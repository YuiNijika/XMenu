#include "BulletAssist.h"
#include "ui/MenuState.h"
#include "features/GameLogic.h"
#include <XBase/BulletAssist.h>
#include "utils/Log.h"
#include "plugin.h"
#include "CWorld.h"
#include "CPools.h"
#include "CPed.h"
#include "CPlayerPed.h"
#include "CVehicle.h"
#include "CSprite.h"
#include "CColPoint.h"
#include "CVector.h"
#include "CEntity.h"
#include "CColModel.h"
#include "CCollisionData.h"
#include "CColBox.h"
#include "CColSphere.h"
#include "CWeapon.h"
#include "CCamera.h"
#include "CPad.h"
#include "CTimer.h"
#include "CModelInfo.h"
#include "ePedBones.h"
#include "ePedState.h"
#include "ePedType.h"
#include "eVehicleType.h"
#include "kiero/minhook/MinHook.h"
#include "imgui/imgui.h"
#include <algorithm>
#include <cmath>
#include <vector>

// SA：与 VC demo 同策略
// - 只在 FireInstantHit* 调用栈内改写 CWorld::ProcessLineOfSight
// - 追踪：范围内 top-N 准星亲和优先；任务中立/敌对可锁，友方不锁
// - 穿墙：仅开火时忽略建筑/物体

namespace {
    using ProcessLineOfSight_t = bool(__cdecl*)(
        CVector const& origin,
        CVector const& target,
        CColPoint& outColPoint,
        CEntity*& outEntity,
        bool buildings,
        bool vehicles,
        bool peds,
        bool objects,
        bool dummies,
        bool doSeeThroughCheck,
        bool doCameraIgnoreCheck,
        bool doShootThroughCheck);

    using FireInstantHit_t = bool(__thiscall*)(
        CWeapon* self,
        CEntity* firingEntity,
        CVector* origin,
        CVector* muzzlePosn,
        CEntity* targetEntity,
        CVector* target,
        CVector* originForDriveBy,
        bool arg6,
        bool muzzle);

    using FireInstantHitFromCar_t = bool(__thiscall*)(CWeapon* self, CVehicle* vehicle, bool leftSide, bool rightSide);

    ProcessLineOfSight_t oProcessLineOfSight = nullptr;
    FireInstantHit_t oFireInstantHit = nullptr;
    FireInstantHitFromCar_t oFireInstantHitFromCar = nullptr;
    bool s_hookAttempted = false;
    int s_bulletFireDepth = 0;

    constexpr float kPi = 3.14159265f;

    struct TrackCandidate {
        CPed* ped = nullptr;          // 行人目标；直升机可为空
        CVehicle* vehicle = nullptr;  // 直升机目标；行人可为空
        CVector pos{};
        float playerDistSq = 0.0f;
        float score = 0.0f; // 越高越优先（准星亲和 + 距离）
    };

    struct LockCache {
        std::vector<CVector> targets;
    };
    LockCache s_lock;
    unsigned s_trackRoundRobin = 0;

    // 一次 FireInstantHit* 只绑定一个最终目标，避免开火改 Dest 与多次 LOS 重复选目标互相打架
    struct ShotTrackState {
        bool hasTarget = false;
        CVector target{};
    };
    ShotTrackState s_shotTrack;

    // 强锁相机：粘住主目标（准星亲和最高），不跟子弹 round-robin 跳
    struct HardLockState {
        CPed* ped = nullptr;
        CVehicle* vehicle = nullptr;
    };
    HardLockState s_hardLock;

    // 第三人称命中点回退高度（部位点失败时）
    constexpr float kAimZFallback = 0.55f;
    constexpr float kTrackRayPast = 0.45f;

    bool InBulletFire() {
        return s_bulletFireDepth > 0;
    }

    float Dist2XY(const CVector& a, const CVector& b) {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        return dx * dx + dy * dy;
    }

    CVector ExtendPast(const CVector& origin, const CVector& aim, float past) {
        const float dx = aim.x - origin.x;
        const float dy = aim.y - origin.y;
        const float dz = aim.z - origin.z;
        const float len = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (len < 0.05f) {
            return aim;
        }
        const float s = (len + past) / len;
        return CVector(origin.x + dx * s, origin.y + dy * s, origin.z + dz * s);
    }

    float LockRange() {
        return MenuState::WeaponBulletLockRange > 10.0f
            ? MenuState::WeaponBulletLockRange
            : 10.0f;
    }

    int MaxTrackTargets() {
        int n = MenuState::WeaponBulletMaxTargets;
        if (n < 1) {
            n = 1;
        }
        if (n > 16) {
            n = 16;
        }
        return n;
    }

    // plugin-sdk / 脚本约定：1=随机，2=任务
    constexpr unsigned char kMissionChar = 2;

    bool IsMissionPed(CPed* ped) {
        return ped && ped->m_nCreatedBy == kMissionChar;
    }

    bool IsMissionVehicle(CVehicle* veh) {
        return veh && veh->m_nCreatedBy == MISSION_VEHICLE;
    }

    bool IsUsablePed(CPed* ped, CPed* player) {
        if (!ped || ped == player) {
            return false;
        }
        if (ped->m_fHealth <= 0.0f) {
            return false;
        }
        if (ped->m_ePedState == PEDSTATE_DEAD || ped->m_ePedState == PEDSTATE_DIE || ped->m_ePedState == PEDSTATE_DIE_BY_STEALTH) {
            return false;
        }
        return true;
    }

    // 子弹追踪 可用行人 + 阵营过滤 见 PassTrackRelation
    // 注意：旧逻辑整表排除任务行人 → 黄色中立任务目标永远锁不上
    bool IsTrackablePed(CPed* ped, CPed* player);

    bool IsUsableVehicle(CVehicle* veh) {
        return veh && veh->m_fHealth > 0.0f;
    }

    bool IsHeliVehicle(CVehicle* veh) {
        if (!veh) {
            return false;
        }
        if (CModelInfo::IsHeliModel(veh->m_nModelIndex)) {
            return true;
        }
        // SA 还有假直升机类
        return veh->m_nVehicleClass == VEHICLE_HELI || veh->m_nVehicleClass == VEHICLE_FHELI;
    }

    bool PedInHeli(CPed* ped) {
        return ped && ped->bInVehicle && ped->m_pVehicle && IsHeliVehicle(ped->m_pVehicle);
    }

    // 直升机瞄准点：碰撞盒中心，失败再抬高机体
    CVector HeliAimPoint(CVehicle* veh) {
        if (!veh) {
            return CVector{};
        }
        if (CColModel* col = veh->GetColModel()) {
            const CVector mid(
                (col->m_boundBox.m_vecMin.x + col->m_boundBox.m_vecMax.x) * 0.5f,
                (col->m_boundBox.m_vecMin.y + col->m_boundBox.m_vecMax.y) * 0.5f,
                (col->m_boundBox.m_vecMin.z + col->m_boundBox.m_vecMax.z) * 0.5f);
            return veh->TransformFromObjectSpace(mid);
        }
        CVector pos = veh->GetPosition();
        pos.z += 0.6f;
        return pos;
    }

    // 平民保持原色 任务实体按 acquaintance 分友/敌/中立
    enum class EspRelation : unsigned char {
        Civilian = 0,
        Friend,
        Hostile,
        Neutral,
    };

    bool AcqHas(unsigned int mask, int pedType) {
        if (pedType < 0 || pedType > 31) {
            return false;
        }
        return (mask & (1u << static_cast<unsigned>(pedType))) != 0;
    }

    EspRelation ClassifyPedSide(CPed* ped, CPed* player) {
        if (!ped || !player) {
            return EspRelation::Neutral;
        }
        const int playerType = player->m_nPedType;
        const int pedType = ped->m_nPedType;
        const CPedAcquaintance& acq = ped->m_acquaintance;
        const CPedAcquaintance& playerAcq = player->m_acquaintance;

        if (AcqHas(acq.m_nHate, playerType) || AcqHas(acq.m_nDislike, playerType)
            || AcqHas(playerAcq.m_nHate, pedType) || AcqHas(playerAcq.m_nDislike, pedType)) {
            return EspRelation::Hostile;
        }
        if (AcqHas(acq.m_nLike, playerType) || AcqHas(acq.m_nRespect, playerType)
            || AcqHas(playerAcq.m_nLike, pedType) || AcqHas(playerAcq.m_nRespect, pedType)) {
            return EspRelation::Friend;
        }
        if (pedType == PED_TYPE_COP) {
            return EspRelation::Hostile;
        }
        return EspRelation::Neutral;
    }

    EspRelation ClassifyPedEsp(CPed* ped, CPed* player) {
        if (!IsMissionPed(ped)) {
            return EspRelation::Civilian;
        }
        return ClassifyPedSide(ped, player);
    }

    EspRelation ClassifyVehicleEsp(CVehicle* veh, CPed* player) {
        if (!IsMissionVehicle(veh)) {
            return EspRelation::Civilian;
        }
        if (veh->m_pDriver && veh->m_pDriver != player) {
            return ClassifyPedSide(veh->m_pDriver, player);
        }
        for (int i = 0; i < 8; ++i) {
            CPed* pass = veh->m_apPassengers[i];
            if (pass && pass != player) {
                return ClassifyPedSide(pass, player);
            }
        }
        return EspRelation::Neutral;
    }

    bool PassEspFilter(EspRelation rel) {
        // ESP 始终按阵营上色绘制；筛选只作用于子弹追踪
        (void)rel;
        return true;
    }

    // 追踪筛选：多选 其他/友方/敌对/中立（挂在「子弹追踪」下）
    bool PassTrackRelation(EspRelation rel) {
        switch (rel) {
        case EspRelation::Civilian:
            return MenuState::WeaponTrackCivilian;
        case EspRelation::Friend:
            return MenuState::WeaponTrackFriend;
        case EspRelation::Hostile:
            return MenuState::WeaponTrackHostile;
        case EspRelation::Neutral:
            return MenuState::WeaponTrackNeutral;
        default:
            return false;
        }
    }

    bool IsTrackablePed(CPed* ped, CPed* player) {
        if (!IsUsablePed(ped, player)) {
            return false;
        }
        // 机上乘员改锁机体，避免同一直升机占两个名额
        if (PedInHeli(ped)) {
            return false;
        }
        return PassTrackRelation(ClassifyPedEsp(ped, player));
    }

    bool IsTrackableHeli(CVehicle* veh, CPed* player) {
        if (!IsUsableVehicle(veh) || !IsHeliVehicle(veh)) {
            return false;
        }
        if (player && (player->m_pVehicle == veh || veh->m_pDriver == player)) {
            return false;
        }
        return PassTrackRelation(ClassifyVehicleEsp(veh, player));
    }

    ImU32 EspBoundColor(EspRelation rel, ImU32 civilian) {
        switch (rel) {
        case EspRelation::Friend:
            return IM_COL32(70, 170, 255, 230);
        case EspRelation::Hostile:
            return IM_COL32(255, 70, 70, 230);
        case EspRelation::Neutral:
            return IM_COL32(255, 210, 60, 230);
        default:
            return civilian;
        }
    }

    ImU32 EspBoxColor(EspRelation rel, ImU32 civilian) {
        switch (rel) {
        case EspRelation::Friend:
            return IM_COL32(80, 190, 255, 220);
        case EspRelation::Hostile:
            return IM_COL32(255, 100, 100, 220);
        case EspRelation::Neutral:
            return IM_COL32(255, 200, 80, 220);
        default:
            return civilian;
        }
    }

    ImU32 EspSphColor(EspRelation rel, ImU32 civilian) {
        switch (rel) {
        case EspRelation::Friend:
            return IM_COL32(120, 200, 255, 200);
        case EspRelation::Hostile:
            return IM_COL32(255, 140, 140, 200);
        case EspRelation::Neutral:
            return IM_COL32(255, 220, 120, 200);
        default:
            return civilian;
        }
    }

    ImU32 EspSkelColor(EspRelation rel, ImU32 civilian) {
        switch (rel) {
        case EspRelation::Friend:
            return IM_COL32(100, 200, 255, 230);
        case EspRelation::Hostile:
            return IM_COL32(255, 120, 80, 230);
        case EspRelation::Neutral:
            return IM_COL32(255, 220, 90, 230);
        default:
            return civilian;
        }
    }

    CVector BonePos(CPed* ped, ePedBones bone, bool updateSkin = false) {
        RwV3d out{};
        ped->GetBonePosition(out, static_cast<unsigned int>(bone), updateSkin);
        return CVector(out.x, out.y, out.z);
    }

    // 相对行人根位置的合理性：过滤 (0,0,0) 与飞出身体的坏点
    bool BoneNearPed(CPed* ped, const CVector& p) {
        if (!ped || !std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) {
            return false;
        }
        const CVector base = ped->GetPosition();
        const float dx = p.x - base.x;
        const float dy = p.y - base.y;
        const float dz = p.z - base.z;
        const float d2 = dx * dx + dy * dy + dz * dz;
        return d2 > 0.0004f && d2 < 12.25f; // ~0.02–3.5m
    }

    CVector PedAimPoint(CPed* ped) {
        // 0头 1胸 2腹 3腿；失败按胸→腹→头→脚底回退
        const int part = MenuState::WeaponBulletAimPart;

        auto fromBone = [&](ePedBones bone, float zBias, CVector& out) -> bool {
            CVector p = BonePos(ped, bone, false);
            if (!BoneNearPed(ped, p)) {
                p = BonePos(ped, bone, true);
            }
            if (!BoneNearPed(ped, p)) {
                return false;
            }
            out = CVector(p.x, p.y, p.z + zBias);
            return true;
        };

        CVector out{};
        switch (part) {
        case 0: // 头：颅顶略下压到面中
            if (fromBone(BONE_HEAD, -0.18f, out)) {
                return out;
            }
            if (fromBone(BONE_NECK, 0.05f, out)) {
                return out;
            }
            break;
        case 2: // 腹
            if (fromBone(BONE_PELVIS, 0.12f, out)) {
                return out;
            }
            if (fromBone(BONE_SPINE1, -0.15f, out)) {
                return out;
            }
            break;
        case 3: { // 腿：两膝/两髋中点
            CVector lk = BonePos(ped, BONE_LEFTKNEE, false);
            CVector rk = BonePos(ped, BONE_RIGHTKNEE, false);
            if (!BoneNearPed(ped, lk)) {
                lk = BonePos(ped, BONE_LEFTKNEE, true);
            }
            if (!BoneNearPed(ped, rk)) {
                rk = BonePos(ped, BONE_RIGHTKNEE, true);
            }
            if (BoneNearPed(ped, lk) && BoneNearPed(ped, rk)) {
                return CVector(
                    (lk.x + rk.x) * 0.5f,
                    (lk.y + rk.y) * 0.5f,
                    (lk.z + rk.z) * 0.5f + 0.08f);
            }
            CVector lh = BonePos(ped, BONE_LEFTHIP, false);
            CVector rh = BonePos(ped, BONE_RIGHTHIP, false);
            if (!BoneNearPed(ped, lh)) {
                lh = BonePos(ped, BONE_LEFTHIP, true);
            }
            if (!BoneNearPed(ped, rh)) {
                rh = BonePos(ped, BONE_RIGHTHIP, true);
            }
            if (BoneNearPed(ped, lh) && BoneNearPed(ped, rh)) {
                return CVector(
                    (lh.x + rh.x) * 0.5f,
                    (lh.y + rh.y) * 0.5f,
                    (lh.z + rh.z) * 0.5f);
            }
            if (fromBone(BONE_PELVIS, -0.25f, out)) {
                return out;
            }
            break;
        }
        case 1: // 胸（默认）
        default:
            if (fromBone(BONE_SPINE1, -0.05f, out)) {
                return out;
            }
            if (fromBone(BONE_UPPERTORSO, -0.08f, out)) {
                return out;
            }
            if (fromBone(BONE_PELVIS, 0.25f, out)) {
                return out;
            }
            break;
        }

        if (fromBone(BONE_SPINE1, -0.05f, out)) {
            return out;
        }
        if (fromBone(BONE_PELVIS, 0.12f, out)) {
            return out;
        }
        if (fromBone(BONE_HEAD, -0.22f, out)) {
            return out;
        }
        CVector pos = ped->GetPosition();
        pos.z += kAimZFallback;
        return pos;
    }

    // 只读相机前向，不改相机
    bool ReadCamAim(CVector& origin, CVector& frontUnit) {
        const int idx = TheCamera.m_nActiveCam;
        if (idx < 0 || idx > 2) {
            return false;
        }
        const CCam& cam = TheCamera.m_aCams[idx];
        origin = cam.m_vecSource;
        const CVector f = cam.m_vecFront;
        const float len = std::sqrt(f.x * f.x + f.y * f.y + f.z * f.z);
        if (len < 1e-4f || !std::isfinite(len)) {
            return false;
        }
        frontUnit = CVector(f.x / len, f.y / len, f.z / len);
        return true;
    }

    float NormalizeAngle(float a) {
        while (a > kPi) {
            a -= 2.0f * kPi;
        }
        while (a < -kPi) {
            a += 2.0f * kPi;
        }
        return a;
    }

    float LerpAngle(float from, float to, float t) {
        return from + NormalizeAngle(to - from) * t;
    }

    void SetCamFrontFromAngles(CCam& cam) {
        const float cv = std::cos(cam.m_fVerticalAngle);
        const float sv = std::sin(cam.m_fVerticalAngle);
        const float ch = std::cos(cam.m_fHorizontalAngle);
        const float sh = std::sin(cam.m_fHorizontalAngle);
        // SA：Front = (-cos(h)*cos(v), -sin(h)*cos(v), sin(v))
        cam.m_vecFront = CVector(-ch * cv, -sh * cv, sv);
        cam.m_fAlphaSpeed = 0.0f;
        cam.m_fBetaSpeed = 0.0f;
    }

    // 屏幕投影（强锁伺服也要用，须在 ApplyHardLock 前可用）
    bool WorldToScreen(const CVector& world, ImVec2& out) {
        RwV3d in{world.x, world.y, world.z};
        RwV3d scr{};
        float w = 0.0f;
        float h = 0.0f;
        if (!CSprite::CalcScreenCoors(in, &scr, &w, &h, true, true) || w < 1.0f || h < 1.0f) {
            return false;
        }
        out = ImVec2(scr.x, scr.y);
        return true;
    }

    // 屏幕正中心（用户指定第三人称跟目标基准；CHair 偏移会导致「看起来偏高」）
    void ScreenAimPoint(float& outX, float& outY) {
        outX = static_cast<float>(RsGlobal.maximumWidth) * 0.5f;
        outY = static_cast<float>(RsGlobal.maximumHeight) * 0.5f;
    }

    // 强锁：把「子弹追踪目标」压到屏幕准星点（第三人称用 CHair，非纯相机 look-at）
    void ApplyHardLockAim(const CVector& worldTarget) {
        if (!MenuState::WeaponBulletHardLock) {
            return;
        }
        const int idx = TheCamera.m_nActiveCam;
        if (idx < 0 || idx > 2) {
            return;
        }
        CCam& cam = TheCamera.m_aCams[idx];

        float blend = 0.55f;
        if (CTimer::ms_fTimeStep > 0.0f) {
            blend = std::min(1.0f, 0.35f * CTimer::ms_fTimeStep);
        }
        if (blend < 0.22f) {
            blend = 0.22f;
        }

        ImVec2 scr{};
        if (WorldToScreen(worldTarget, scr)) {
            float aimX = 0.0f;
            float aimY = 0.0f;
            ScreenAimPoint(aimX, aimY);

            const float halfW = std::max(1.0f, static_cast<float>(RsGlobal.maximumWidth) * 0.5f);
            const float halfH = std::max(1.0f, static_cast<float>(RsGlobal.maximumHeight) * 0.5f);
            const float errX = scr.x - aimX;
            const float errY = scr.y - aimY;

            float fovDeg = cam.m_fFOV;
            if (fovDeg < 5.0f || fovDeg > 170.0f || !std::isfinite(fovDeg)) {
                fovDeg = 70.0f;
            }
            const float halfFovY = fovDeg * 0.5f * (kPi / 180.0f);
            const float aspect = halfW / halfH;
            const float halfFovX = std::atan(std::tan(halfFovY) * aspect);

            // 目标在准星右侧 → 向右转；目标在准星下方 → 低头
            // 与 SA 鼠标约定一致：水平角减小 ≈ 右转；垂直角减小 ≈ 低头
            const float dH = -(errX / halfW) * halfFovX;
            const float dV = -(errY / halfH) * halfFovY;

            float horiz = cam.m_fHorizontalAngle + dH;
            float vert = cam.m_fVerticalAngle + dV;

            constexpr float kMaxPitchUp = 1.05f;
            constexpr float kMaxPitchDown = 1.49f;
            if (vert > kMaxPitchUp) {
                vert = kMaxPitchUp;
            }
            if (vert < -kMaxPitchDown) {
                vert = -kMaxPitchDown;
            }

            cam.m_fHorizontalAngle = LerpAngle(cam.m_fHorizontalAngle, horiz, blend);
            cam.m_fVerticalAngle = LerpAngle(cam.m_fVerticalAngle, vert, blend);
            SetCamFrontFromAngles(cam);
            return;
        }

        // 屏外回退：几何 look-at（同引擎角约定）
        const float dx = worldTarget.x - cam.m_vecSource.x;
        const float dy = worldTarget.y - cam.m_vecSource.y;
        const float dz = worldTarget.z - cam.m_vecSource.z;
        const float len = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (len < 0.05f || !std::isfinite(len)) {
            return;
        }
        const float inv = 1.0f / len;
        const float nx = dx * inv;
        const float ny = dy * inv;
        const float nz = dz * inv;
        float vert = std::asin(std::max(-1.0f, std::min(1.0f, nz)));
        float horiz = std::atan2(-ny, -nx);
        constexpr float kMaxPitchUp = 1.05f;
        constexpr float kMaxPitchDown = 1.49f;
        if (vert > kMaxPitchUp) {
            vert = kMaxPitchUp;
        }
        if (vert < -kMaxPitchDown) {
            vert = -kMaxPitchDown;
        }
        cam.m_fHorizontalAngle = LerpAngle(cam.m_fHorizontalAngle, horiz, blend);
        cam.m_fVerticalAngle = LerpAngle(cam.m_fVerticalAngle, vert, blend);
        SetCamFrontFromAngles(cam);
    }

    void ClearHardLock() {
        s_hardLock.ped = nullptr;
        s_hardLock.vehicle = nullptr;
    }

    bool PlayerWantsHardLockInput(); // 定义在后

    // 粘性主目标 优先保持当前 ped/直升机，丢失后才换准星最优
    const TrackCandidate* ResolveHardLockCandidate(const std::vector<TrackCandidate>& candidates) {
        if (candidates.empty()) {
            ClearHardLock();
            return nullptr;
        }
        if (s_hardLock.ped || s_hardLock.vehicle) {
            for (const TrackCandidate& c : candidates) {
                if (s_hardLock.ped && c.ped == s_hardLock.ped) {
                    return &c;
                }
                if (s_hardLock.vehicle && c.vehicle == s_hardLock.vehicle) {
                    return &c;
                }
            }
        }
        s_hardLock.ped = candidates[0].ped;
        s_hardLock.vehicle = candidates[0].vehicle;
        return &candidates[0];
    }

    void ApplyHardLockToSticky(const std::vector<TrackCandidate>& candidates) {
        if (!MenuState::WeaponBulletHardLock || !PlayerWantsHardLockInput()) {
            if (!MenuState::WeaponBulletHardLock) {
                ClearHardLock();
            }
            return;
        }
        const TrackCandidate* target = ResolveHardLockCandidate(candidates);
        if (!target) {
            return;
        }
        ApplyHardLockAim(target->pos);
    }

    bool PlayerWantsHardLockInput() {
        CPad* pad = CPad::GetPad(0);
        if (!pad) {
            return false;
        }
        if (pad->GetTarget()) {
            return true;
        }
        if (pad->NewState.ButtonCircle != 0) {
            return true;
        }
        if (CPad::NewMouseControllerState.lmb) {
            return true;
        }
        return false;
    }

    // 准星亲和分：身后直接丢弃；越贴准星越高，略惩罚距离
    float ScoreTrackAim(const CVector& camOrigin, const CVector& camFront, const CVector& aim, float distSq) {
        const float dx = aim.x - camOrigin.x;
        const float dy = aim.y - camOrigin.y;
        const float dz = aim.z - camOrigin.z;
        const float len = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (len < 0.05f || !std::isfinite(len)) {
            return -1.0f;
        }
        const float dot = (dx * camFront.x + dy * camFront.y + dz * camFront.z) / len;
        // ~75° 外不锁，避免身后/侧后方乱飘
        if (dot < 0.25f) {
            return -1.0f;
        }
        const float range = LockRange();
        const float distNorm = (range > 1.0f) ? (std::sqrt(distSq) / range) : 1.0f;
        return dot * 3.0f - distNorm * 0.35f;
    }

    std::vector<TrackCandidate> CollectTrackCandidates() {
        std::vector<TrackCandidate> candidates;
        CPed* player = FindPlayerPed();
        if (!player) {
            return candidates;
        }

        const CVector playerPos = player->GetPosition();
        const float maxRangeSq = LockRange() * LockRange();
        const int maxTargets = MaxTrackTargets();
        candidates.reserve(static_cast<std::size_t>(maxTargets));

        CVector camOrigin{};
        CVector camFront{};
        const bool hasCam = ReadCamAim(camOrigin, camFront);

        auto worseScore = [](const TrackCandidate& a, const TrackCandidate& b) {
            return a.score > b.score; // 最小堆：堆顶是 top-N 里分最低的
        };

        auto consider = [&](TrackCandidate item) {
            if (item.playerDistSq > maxRangeSq) {
                return;
            }
            float score = 0.0f;
            if (hasCam) {
                score = ScoreTrackAim(camOrigin, camFront, item.pos, item.playerDistSq);
                if (score < 0.0f) {
                    return;
                }
            } else {
                score = 1.0f / (1.0f + item.playerDistSq);
            }
            item.score = score;

            if (static_cast<int>(candidates.size()) < maxTargets) {
                candidates.push_back(item);
                std::push_heap(candidates.begin(), candidates.end(), worseScore);
                return;
            }
            if (score <= candidates.front().score) {
                return;
            }
            std::pop_heap(candidates.begin(), candidates.end(), worseScore);
            candidates.back() = item;
            std::push_heap(candidates.begin(), candidates.end(), worseScore);
        };

        if (CPools::ms_pPedPool) {
            for (CPed* ped : CPools::ms_pPedPool) {
                if (!IsTrackablePed(ped, player)) {
                    continue;
                }
                const CVector pos = PedAimPoint(ped);
                const float d2 = Dist2XY(playerPos, pos) + (pos.z - playerPos.z) * (pos.z - playerPos.z);
                consider(TrackCandidate{ ped, nullptr, pos, d2, 0.0f });
            }
        }

        // 直升机：整机作为目标（乘员已在行人循环跳过）
        if (CPools::ms_pVehiclePool) {
            for (CVehicle* veh : CPools::ms_pVehiclePool) {
                if (!IsTrackableHeli(veh, player)) {
                    continue;
                }
                const CVector pos = HeliAimPoint(veh);
                const float d2 = Dist2XY(playerPos, pos) + (pos.z - playerPos.z) * (pos.z - playerPos.z);
                consider(TrackCandidate{ nullptr, veh, pos, d2, 0.0f });
            }
        }

        // 分高在前；同分近者优先
        std::sort(candidates.begin(), candidates.end(), [](const TrackCandidate& a, const TrackCandidate& b) {
            if (a.score != b.score) {
                return a.score > b.score;
            }
            return a.playerDistSq < b.playerDistSq;
        });
        return candidates;
    }

    void RefreshLockVisuals(const std::vector<TrackCandidate>& candidates) {
        s_lock.targets.clear();
        s_lock.targets.reserve(candidates.size());
        for (const TrackCandidate& item : candidates) {
            s_lock.targets.push_back(item.pos);
        }
    }

    // 本发射击绑定：单目标=准星最优；多目标在 top-N 内轮询
    // 强锁相机独立：粘主目标，不跟本发 round-robin
    void BeginShotTrack() {
        s_shotTrack.hasTarget = false;
        s_shotTrack.target = CVector{};
        if (!MenuState::WeaponBulletTrack) {
            return;
        }

        const std::vector<TrackCandidate> candidates = CollectTrackCandidates();
        RefreshLockVisuals(candidates);
        if (candidates.empty()) {
            ClearHardLock();
            return;
        }

        if (candidates.size() == 1 || MaxTrackTargets() == 1) {
            s_shotTrack.target = candidates[0].pos;
        } else {
            s_shotTrack.target = candidates[s_trackRoundRobin % candidates.size()].pos;
            ++s_trackRoundRobin;
        }
        s_shotTrack.hasTarget = true;
        ApplyHardLockToSticky(candidates);
    }

    void EndShotTrack() {
        s_shotTrack.hasTarget = false;
    }

    bool ResolveShotTrackTarget(CVector& outTarget) {
        if (!MenuState::WeaponBulletTrack) {
            return false;
        }
        // 开火栈内：固定使用本发已绑定目标，保证每一发都稳定落到同一点
        if (InBulletFire()) {
            if (!s_shotTrack.hasTarget) {
                return false;
            }
            outTarget = s_shotTrack.target;
            return true;
        }
        // 兜底（不应常走到）：即时选一个
        const std::vector<TrackCandidate> candidates = CollectTrackCandidates();
        RefreshLockVisuals(candidates);
        if (candidates.empty()) {
            return false;
        }
        outTarget = candidates[0].pos;
        return true;
    }

    struct PlayerShotScope {
        bool active = false;
        explicit PlayerShotScope(bool enable) {
            if (!enable) {
                return;
            }
            active = true;
            ++s_bulletFireDepth;
            if (s_bulletFireDepth == 1) {
                BeginShotTrack();
            }
        }
        ~PlayerShotScope() {
            if (!active) {
                return;
            }
            if (s_bulletFireDepth == 1) {
                EndShotTrack();
            }
            --s_bulletFireDepth;
        }
    };

    bool __cdecl hkProcessLineOfSight(
        CVector const& origin,
        CVector const& target,
        CColPoint& outColPoint,
        CEntity*& outEntity,
        bool buildings,
        bool vehicles,
        bool peds,
        bool objects,
        bool dummies,
        bool doSeeThroughCheck,
        bool doCameraIgnoreCheck,
        bool doShootThroughCheck) {
        if (!oProcessLineOfSight) {
            return false;
        }

        // 玩家开火栈内：放宽 LOS 特征，避免部分武器 buildings/peds 组合漏改 Dest
        if (!InBulletFire() || doCameraIgnoreCheck) {
            return oProcessLineOfSight(
                origin, target, outColPoint, outEntity,
                buildings, vehicles, peds, objects, dummies,
                doSeeThroughCheck, doCameraIgnoreCheck, doShootThroughCheck);
        }

        CVector redirected = target;
        bool useBuildings = buildings;
        bool useVehicles = vehicles;
        bool useObjects = objects;
        bool useDummies = dummies;
        bool usePeds = peds;
        bool tracked = false;

        if (MenuState::WeaponBulletTrack) {
            CVector trackPos{};
            if (ResolveShotTrackTarget(trackPos)) {
                redirected = ExtendPast(origin, trackPos, kTrackRayPast);
                tracked = true;
            }
        }

        if (tracked) {
            usePeds = true;
            // 直升机目标 确保测载具碰撞
            useVehicles = true;
        }

        if (MenuState::WeaponBulletThroughWalls) {
            useBuildings = false;
            useObjects = false;
            useDummies = false;
        }

        return oProcessLineOfSight(
            origin, redirected, outColPoint, outEntity,
            useBuildings, useVehicles, usePeds, useObjects, useDummies,
            doSeeThroughCheck, doCameraIgnoreCheck, doShootThroughCheck);
    }

    bool IsLocalPlayerEntity(CEntity* ent) {
        CPed* player = FindPlayerPed();
        return player && ent == static_cast<CEntity*>(player);
    }

    bool __fastcall hkFireInstantHit(
        CWeapon* self,
        void* /*edx*/,
        CEntity* firingEntity,
        CVector* origin,
        CVector* muzzlePosn,
        CEntity* targetEntity,
        CVector* target,
        CVector* originForDriveBy,
        bool arg6,
        bool muzzle) {
        PlayerShotScope shotScope(IsLocalPlayerEntity(firingEntity));
        if (!oFireInstantHit) {
            return false;
        }

        // PedsNoFire: 阻止非玩家 ped 按筛选开火（不改库存、不记录 trace）
        if (firingEntity && !IsLocalPlayerEntity(firingEntity)) {
            CPed* firer = static_cast<CPed*>(firingEntity);
            const int firerId = firer ? CPools::GetPedRef(firer) : -1;
            if (XBase::BulletAssist::ShouldSuppressPedFire(
                    XBase::PedId{firerId >= 0 ? static_cast<std::uint32_t>(firerId) + 1u : 0u})) {
                return false;
            }
        }

        // 开枪前改 Dest；与 LOS 共用本发绑定目标，保证每一发都追踪
        CVector saved{};
        bool patched = false;
        if (shotScope.active && MenuState::WeaponBulletTrack && target) {
            CVector trackPos{};
            if (ResolveShotTrackTarget(trackPos)) {
                saved = *target;
                *target = trackPos;
                patched = true;
            }
        }

        const bool result = oFireInstantHit(
            self, firingEntity, origin, muzzlePosn, targetEntity, target, originForDriveBy, arg6, muzzle);

        if (patched) {
            *target = saved;
        }
        return result;
    }

    bool __fastcall hkFireInstantHitFromCar(CWeapon* self, void* /*edx*/, CVehicle* vehicle, bool leftSide, bool rightSide) {
        CPed* player = FindPlayerPed();
        const bool playerShot = player && player->m_pVehicle == vehicle;
        PlayerShotScope shotScope(playerShot);
        if (!oFireInstantHitFromCar) {
            return false;
        }
        if (vehicle && vehicle->m_pDriver && vehicle->m_pDriver != player) {
            const int driverId = vehicle->m_pDriver ? CPools::GetPedRef(vehicle->m_pDriver) : -1;
            if (XBase::BulletAssist::ShouldSuppressPedFire(
                    XBase::PedId{driverId >= 0 ? static_cast<std::uint32_t>(driverId) + 1u : 0u})) {
                return false;
            }
        }
        return oFireInstantHitFromCar(self, vehicle, leftSide, rightSide);
    }

    void EnsureHook() {
        if (s_hookAttempted) {
            return;
        }
        s_hookAttempted = true;

        MH_STATUS initStatus = MH_Initialize();
        if (initStatus != MH_OK && initStatus != MH_ERROR_ALREADY_INITIALIZED) {
            Log::Warn("BulletAssist SA: MinHook 初始化失败");
            return;
        }

        // FireInstantHit @ 0x73FB10
        {
            const MH_STATUS st = MH_CreateHook(
                reinterpret_cast<void*>(0x73FB10),
                reinterpret_cast<void*>(&hkFireInstantHit),
                reinterpret_cast<void**>(&oFireInstantHit));
            if (st == MH_OK || st == MH_ERROR_ALREADY_CREATED) {
                MH_EnableHook(reinterpret_cast<void*>(0x73FB10));
            } else {
                Log::Warn("BulletAssist SA: FireInstantHit hook 失败");
            }
        }
        // FireInstantHitFromCar @ 0x73EC40
        {
            const MH_STATUS st = MH_CreateHook(
                reinterpret_cast<void*>(0x73EC40),
                reinterpret_cast<void*>(&hkFireInstantHitFromCar),
                reinterpret_cast<void**>(&oFireInstantHitFromCar));
            if (st == MH_OK || st == MH_ERROR_ALREADY_CREATED) {
                MH_EnableHook(reinterpret_cast<void*>(0x73EC40));
            }
        }

        // CWorld::ProcessLineOfSight @ 0x56BA00 — 仅 InBulletFire
        {
            const MH_STATUS st = MH_CreateHook(
                reinterpret_cast<void*>(0x56BA00),
                reinterpret_cast<void*>(&hkProcessLineOfSight),
                reinterpret_cast<void**>(&oProcessLineOfSight));
            if (st != MH_OK && st != MH_ERROR_ALREADY_CREATED) {
                Log::Warn("BulletAssist SA: CWorld::PLOS hook 失败");
                return;
            }
            const MH_STATUS en = MH_EnableHook(reinterpret_cast<void*>(0x56BA00));
            if (en != MH_OK && en != MH_ERROR_ENABLED) {
                Log::Warn("BulletAssist SA: 启用 CWorld::PLOS hook 失败");
                return;
            }
        }

        Log::Info("BulletAssist SA: 开火上下文 + LOS hook 已就绪");
    }

    void DrawWorldLine(ImDrawList* dl, const CVector& a, const CVector& b, ImU32 color, float thickness = 1.25f) {
        ImVec2 sa{};
        ImVec2 sb{};
        if (!WorldToScreen(a, sa) || !WorldToScreen(b, sb)) {
            return;
        }
        dl->AddLine(sa, sb, color, thickness);
    }

    void DrawLocalBoxWire(ImDrawList* dl, CEntity* ent, const CVector& mn, const CVector& mx, ImU32 color) {
        if (!ent) {
            return;
        }
        CVector c[8];
        c[0] = ent->TransformFromObjectSpace(CVector(mn.x, mn.y, mn.z));
        c[1] = ent->TransformFromObjectSpace(CVector(mx.x, mn.y, mn.z));
        c[2] = ent->TransformFromObjectSpace(CVector(mx.x, mx.y, mn.z));
        c[3] = ent->TransformFromObjectSpace(CVector(mn.x, mx.y, mn.z));
        c[4] = ent->TransformFromObjectSpace(CVector(mn.x, mn.y, mx.z));
        c[5] = ent->TransformFromObjectSpace(CVector(mx.x, mn.y, mx.z));
        c[6] = ent->TransformFromObjectSpace(CVector(mx.x, mx.y, mx.z));
        c[7] = ent->TransformFromObjectSpace(CVector(mn.x, mx.y, mx.z));
        static const int edges[12][2] = {
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            {4, 5}, {5, 6}, {6, 7}, {7, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7},
        };
        for (const auto& e : edges) {
            DrawWorldLine(dl, c[e[0]], c[e[1]], color);
        }
    }

    void DrawLocalSphereWire(ImDrawList* dl, CEntity* ent, const CVector& center, float radius, ImU32 color) {
        if (!ent || radius < 0.01f) {
            return;
        }
        constexpr int kSeg = 12;
        CVector prevXY{};
        CVector prevXZ{};
        CVector prevYZ{};
        for (int i = 0; i <= kSeg; ++i) {
            const float ang = (kPi * 2.0f) * static_cast<float>(i) / static_cast<float>(kSeg);
            const float ca = std::cos(ang);
            const float sa = std::sin(ang);
            const CVector pXY = ent->TransformFromObjectSpace(CVector(center.x + radius * ca, center.y + radius * sa, center.z));
            const CVector pXZ = ent->TransformFromObjectSpace(CVector(center.x + radius * ca, center.y, center.z + radius * sa));
            const CVector pYZ = ent->TransformFromObjectSpace(CVector(center.x, center.y + radius * ca, center.z + radius * sa));
            if (i > 0) {
                DrawWorldLine(dl, prevXY, pXY, color);
                DrawWorldLine(dl, prevXZ, pXZ, color);
                DrawWorldLine(dl, prevYZ, pYZ, color);
            }
            prevXY = pXY;
            prevXZ = pXZ;
            prevYZ = pYZ;
        }
    }

    void DrawEntityDebugBound(ImDrawList* dl, CEntity* ent, ImU32 color) {
        CColModel* col = ent->GetColModel();
        if (!col) {
            return;
        }
        DrawLocalBoxWire(dl, ent, col->m_boundBox.m_vecMin, col->m_boundBox.m_vecMax, color);
    }

    void DrawEntityColPartial(ImDrawList* dl, CEntity* ent, ImU32 boxColor, ImU32 sphereColor) {
        CColModel* col = ent->GetColModel();
        if (!col || !col->m_pColData) {
            return;
        }
        CCollisionData* data = col->m_pColData;
        if (data->m_pBoxes && data->m_nNumBoxes > 0) {
            for (unsigned short i = 0; i < data->m_nNumBoxes; ++i) {
                DrawLocalBoxWire(dl, ent, data->m_pBoxes[i].m_vecMin, data->m_pBoxes[i].m_vecMax, boxColor);
            }
        }
        if (data->m_pSpheres && data->m_nNumSpheres > 0) {
            for (unsigned short i = 0; i < data->m_nNumSpheres; ++i) {
                DrawLocalSphereWire(dl, ent, data->m_pSpheres[i].m_vecCenter, data->m_pSpheres[i].m_fRadius, sphereColor);
            }
        }
    }

    // 骨骼失败时 用实体局部坐标画简化火柴人
    void DrawPedStickFallback(ImDrawList* dl, CPed* ped, ImU32 color) {
        if (!ped) {
            return;
        }
        const CVector head = ped->TransformFromObjectSpace(CVector(0.0f, 0.0f, 0.90f));
        const CVector neck = ped->TransformFromObjectSpace(CVector(0.0f, 0.0f, 0.72f));
        const CVector pelvis = ped->TransformFromObjectSpace(CVector(0.0f, 0.0f, 0.38f));
        const CVector lHand = ped->TransformFromObjectSpace(CVector(-0.42f, 0.0f, 0.62f));
        const CVector rHand = ped->TransformFromObjectSpace(CVector(0.42f, 0.0f, 0.62f));
        const CVector lFoot = ped->TransformFromObjectSpace(CVector(-0.12f, 0.0f, 0.05f));
        const CVector rFoot = ped->TransformFromObjectSpace(CVector(0.12f, 0.0f, 0.05f));
        DrawWorldLine(dl, head, neck, color, 1.5f);
        DrawWorldLine(dl, neck, pelvis, color, 1.5f);
        DrawWorldLine(dl, neck, lHand, color, 1.4f);
        DrawWorldLine(dl, neck, rHand, color, 1.4f);
        DrawWorldLine(dl, pelvis, lFoot, color, 1.4f);
        DrawWorldLine(dl, pelvis, rFoot, color, 1.4f);
    }

    void DrawPedSkeleton(ImDrawList* dl, CPed* ped, ImU32 color) {
        if (!ped || !ped->m_pRwClump) {
            return;
        }
        // 不可见且不在屏内：HAnim 常未更新，画出来是乱线
        if (!ped->GetIsOnScreen() && !ped->bIsVisible) {
            return;
        }

        // 一次刷新皮肤矩阵，避免 GetBonePosition(false) 全落根点
        ped->UpdateRpHAnim();

        const ePedBones links[][2] = {
            {BONE_PELVIS, BONE_SPINE1},
            {BONE_SPINE1, BONE_NECK},
            {BONE_NECK, BONE_HEAD},
            {BONE_NECK, BONE_LEFTSHOULDER},
            {BONE_LEFTSHOULDER, BONE_LEFTELBOW},
            {BONE_LEFTELBOW, BONE_LEFTHAND},
            {BONE_NECK, BONE_RIGHTSHOULDER},
            {BONE_RIGHTSHOULDER, BONE_RIGHTELBOW},
            {BONE_RIGHTELBOW, BONE_RIGHTHAND},
            {BONE_PELVIS, BONE_LEFTHIP},
            {BONE_LEFTHIP, BONE_LEFTKNEE},
            {BONE_LEFTKNEE, BONE_LEFTFOOT},
            {BONE_PELVIS, BONE_RIGHTHIP},
            {BONE_RIGHTHIP, BONE_RIGHTKNEE},
            {BONE_RIGHTKNEE, BONE_RIGHTFOOT},
        };

        int ok = 0;
        for (const auto& e : links) {
            CVector pa = BonePos(ped, e[0], false);
            if (!BoneNearPed(ped, pa)) {
                pa = BonePos(ped, e[0], true);
            }
            CVector pb = BonePos(ped, e[1], false);
            if (!BoneNearPed(ped, pb)) {
                pb = BonePos(ped, e[1], true);
            }
            if (!BoneNearPed(ped, pa) || !BoneNearPed(ped, pb)) {
                continue;
            }
            const float dx = pa.x - pb.x;
            const float dy = pa.y - pb.y;
            const float dz = pa.z - pb.z;
            const float d2 = dx * dx + dy * dy + dz * dz;
            if (d2 < 0.0004f || d2 > 2.56f) {
                continue;
            }
            DrawWorldLine(dl, pa, pb, color, 1.6f);
            ++ok;
        }

        if (ok < 4) {
            DrawPedStickFallback(dl, ped, color);
        }
    }

    void DrawTrackRange(ImDrawList* dl, CPed* player) {
        const float range = LockRange();
        const CVector center = player->GetPosition();
        const ImU32 rangeColor = IM_COL32(255, 80, 80, 180);
        const ImU32 lockColor = IM_COL32(255, 40, 40, 255);

        constexpr int kSeg = 48;
        ImVec2 prev{};
        bool hasPrev = false;
        for (int i = 0; i <= kSeg; ++i) {
            const float ang = (kPi * 2.0f) * static_cast<float>(i) / static_cast<float>(kSeg);
            const CVector p(center.x + std::cos(ang) * range, center.y + std::sin(ang) * range, center.z);
            ImVec2 s{};
            if (WorldToScreen(p, s)) {
                if (hasPrev) {
                    dl->AddLine(prev, s, rangeColor, 1.5f);
                }
                prev = s;
                hasPrev = true;
            } else {
                hasPrev = false;
            }
        }

        for (const CVector& lockPos : s_lock.targets) {
            ImVec2 s{};
            if (WorldToScreen(lockPos, s)) {
                dl->AddCircle(s, 12.0f, lockColor, 16, 2.0f);
                dl->AddCircleFilled(s, 3.0f, lockColor, 8);
            }
            DrawWorldLine(dl, center, lockPos, lockColor, 1.4f);
        }
    }
}

namespace Controllers::BulletAssist {
    void Init() {
        EnsureHook();
    }

    void Process() {
        s_lock.targets.clear();
        if (!MenuState::WeaponBulletTrack) {
            ClearHardLock();
            return;
        }
        EnsureHook();
        const std::vector<TrackCandidate> candidates = CollectTrackCandidates();
        RefreshLockVisuals(candidates);
        // 按住开火/瞄准：强锁粘住主目标，不跟多目标轮询跳视角
        ApplyHardLockToSticky(candidates);
    }

    void Draw() {
        const bool pedDebug = MenuState::WeaponPedEsp;
        const bool pedCol = MenuState::WeaponPedColEsp;
        const bool pedSkel = MenuState::WeaponPedSkeleton;
        const bool vehDebug = MenuState::WeaponVehicleEsp;
        const bool vehCol = MenuState::WeaponVehicleColEsp;
        const bool track = MenuState::WeaponBulletTrack;
        if (!pedDebug && !pedCol && !pedSkel && !vehDebug && !vehCol && !track) {
            return;
        }

        CPed* player = FindPlayerPed();
        if (!player) {
            return;
        }

        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        if (!dl) {
            return;
        }

        if (track) {
            DrawTrackRange(dl, player);
        }

        // 平民默认色；任务实体由 Esp*Color 按阵营改写
        const ImU32 pedBoundCiv = IM_COL32(80, 220, 120, 230);
        const ImU32 pedBoxCiv = IM_COL32(60, 180, 255, 220);
        const ImU32 pedSphCiv = IM_COL32(120, 200, 255, 200);
        const ImU32 skelCiv = IM_COL32(255, 200, 60, 230);
        const ImU32 vehBoundCiv = IM_COL32(255, 140, 60, 230);
        const ImU32 vehBoxCiv = IM_COL32(255, 90, 90, 220);
        const ImU32 vehSphCiv = IM_COL32(255, 160, 120, 200);

        if ((pedDebug || pedCol || pedSkel) && CPools::ms_pPedPool) {
            for (CPed* ped : CPools::ms_pPedPool) {
                if (!IsUsablePed(ped, player)) {
                    continue;
                }
                const EspRelation rel = ClassifyPedEsp(ped, player);
                if (!PassEspFilter(rel)) {
                    continue;
                }
                if (pedDebug) {
                    DrawEntityDebugBound(dl, ped, EspBoundColor(rel, pedBoundCiv));
                }
                if (pedCol) {
                    DrawEntityColPartial(dl, ped, EspBoxColor(rel, pedBoxCiv), EspSphColor(rel, pedSphCiv));
                }
                if (pedSkel) {
                    DrawPedSkeleton(dl, ped, EspSkelColor(rel, skelCiv));
                }
            }
        }

        if ((vehDebug || vehCol) && CPools::ms_pVehiclePool) {
            for (CVehicle* veh : CPools::ms_pVehiclePool) {
                if (!IsUsableVehicle(veh)) {
                    continue;
                }
                const EspRelation rel = ClassifyVehicleEsp(veh, player);
                if (!PassEspFilter(rel)) {
                    continue;
                }
                if (vehDebug) {
                    DrawEntityDebugBound(dl, veh, EspBoundColor(rel, vehBoundCiv));
                }
                if (vehCol) {
                    DrawEntityColPartial(dl, veh, EspBoxColor(rel, vehBoxCiv), EspSphColor(rel, vehSphCiv));
                }
            }
        }
    }
}