#include "BulletAssist.h"
#include "ui/MenuState.h"
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
#include "CColBox.h"
#include "CColSphere.h"
#include "CMatrix.h"
#include "CWeapon.h"
#include "CCamera.h"
#include "CPad.h"
#include "AnimBlendFrameData.h"
#include "eObjective.h"
#include "ePedType.h"
#include "kiero/minhook/MinHook.h"
#include "imgui/imgui.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <windows.h>

// - 目标点优先 SEH 安全读头/躯干节点，失败再用 GetPosition+固定 z 不调 UpdateRpHAnim
// - Fire + FireInstantHit* + ProcessLineOfSight
// - 追踪：范围内 top-N 准星亲和优先；任务中立/敌对可锁，友方不锁
// - 仅本地玩家开火栈内改 Dest / 软穿墙；相机 LOS 一律不碰

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

    using FireInstantHit_t = bool(__thiscall*)(CWeapon* self, CEntity* firingEntity, CVector* sourcePos);
    using FireInstantHitFromCar_t = bool(__thiscall*)(CWeapon* self, CVehicle* vehicle, bool left, bool right);
    using Fire_t = bool(__thiscall*)(CWeapon* self, CEntity* firingEntity, CVector* sourcePos);

    ProcessLineOfSight_t oProcessLineOfSight = nullptr;
    FireInstantHit_t oFireInstantHit = nullptr;
    FireInstantHitFromCar_t oFireInstantHitFromCar = nullptr;
    Fire_t oFire = nullptr;
    bool s_hookAttempted = false;

    int s_playerBulletDepth = 0;

    constexpr float kPi = 3.14159265f;
    // 脚底→约胸口/下颌；仅作节点失败时的兜底（0.85 会打到头顶外）
    constexpr float kPedAimZFallback = 0.70f;
    // 头节点常在颅顶，略下压到面中/下颌，避免“瞄头偏高”打空
    constexpr float kHeadAimDrop = 0.12f;
    // 终点略过瞄准点，让线段真正穿过 ColSphere 而不是停在表面外
    constexpr float kTrackRayPast = 0.45f;

    // 前置声明：瞄准与绘制共用 SEH 读帧
    bool SafePedNodeWorldPos(CPed* ped, int node, float* ox, float* oy, float* oz);

    enum ePedNode : int {
        PED_NODE_TORSO = 1,
        PED_NODE_HEAD = 2,
        PED_NODE_UPPER_ARM_L = 3,
        PED_NODE_UPPER_ARM_R = 4,
        PED_NODE_HAND_L = 5,
        PED_NODE_HAND_R = 6,
        PED_NODE_LEG_L = 7,
        PED_NODE_LEG_R = 8,
        PED_NODE_FOOT_L = 9,
        PED_NODE_FOOT_R = 10,
    };

    struct TrackCandidate {
        CVector pos{};
        float playerDistSq = 0.0f;
        float score = 0.0f; // 越高越优先
    };

    struct LockCache {
        std::vector<CVector> targets;
    };
    LockCache s_lock;
    unsigned s_trackRoundRobin = 0;

    // 一次 FireInstantHit* 只绑定一个最终目标
    struct ShotTrackState {
        bool hasTarget = false;
        CVector target{};
    };
    ShotTrackState s_shotTrack;

    struct PlayerShotScope {
        bool active = false;
        explicit PlayerShotScope(bool enable);
        ~PlayerShotScope();
    };

    bool InPlayerBullet() {
        return s_playerBulletDepth > 0;
    }

    bool IsLocalPlayerEntity(CEntity* ent) {
        CPed* player = FindPlayerPed();
        if (!player || !ent) {
            return false;
        }
        if (ent == static_cast<CEntity*>(player)) {
            return true;
        }
        // 部分开火路径会把载具当 firingEntity
        if (player->m_pVehicle && ent == static_cast<CEntity*>(player->m_pVehicle)) {
            return true;
        }
        return false;
    }

    float Dist2(const CVector& a, const CVector& b) {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        const float dz = a.z - b.z;
        return dx * dx + dy * dy + dz * dz;
    }

    float Dist2XY(const CVector& a, const CVector& b) {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        return dx * dx + dy * dy;
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

    // VC：m_nPedStatus 1=普通可回收，2=脚本/任务行人
    constexpr unsigned char kScriptPedStatus = 2;

    bool IsMissionPed(CPed* ped) {
        return ped && ped->m_nPedStatus == kScriptPedStatus;
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
        if (ped->m_ePedState == PEDSTATE_DEAD || ped->m_ePedState == PEDSTATE_DIE) {
            return false;
        }
        return true;
    }

    // 子弹追踪：可用行人 + 阵营过滤（见 PassTrackRelation）
    // 旧逻辑整表排除任务行人 → 黄色中立任务目标永远锁不上
    bool IsTrackablePed(CPed* ped, CPed* player);

    bool IsUsableVehicle(CVehicle* veh) {
        return veh && veh->m_fHealth > 0.0f;
    }

    // ESP 阵营：平民原色；任务实体按 ally / objective / threat / pedType
    enum class EspRelation : unsigned char {
        Civilian = 0,
        Friend,
        Hostile,
        Neutral,
    };

    EspRelation ClassifyPedSide(CPed* ped, CPed* player) {
        if (!ped || !player) {
            return EspRelation::Neutral;
        }
        if (ped->bScriptPedIsPlayerAlly) {
            return EspRelation::Friend;
        }
        if (ped->m_pThreatEntity == static_cast<CEntity*>(player)) {
            return EspRelation::Hostile;
        }
        const eObjective obj = ped->m_nObjective;
        if (obj == OBJECTIVE_KILL_CHAR_ON_FOOT
            || obj == OBJECTIVE_KILL_CHAR_ANY_MEANS
            || obj == OBJECTIVE_KILL_CHAR_ON_BOAT
            || obj == OBJECTIVE_GUARD_ATTACK
            || obj == OBJECTIVE_AIM_GUN_AT) {
            if (ped->m_pObjectiveEntity == static_cast<CEntity*>(player)) {
                return EspRelation::Hostile;
            }
            // 任务脚本常不写 objective entity，仍标为敌对更直观
            return EspRelation::Hostile;
        }
        if (ped->m_nPedType == PED_TYPE_COP) {
            return EspRelation::Hostile;
        }
        if (ped->m_nPedType == PED_TYPE_GANG_VERCETTI) {
            return EspRelation::Friend;
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
            CPed* pass = veh->m_passengers[i];
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
        return PassTrackRelation(ClassifyPedEsp(ped, player));
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

    // 优先头/躯干世界坐标（SEH）；失败再用脚底 + 固定高度
    CVector PedAimPoint(CPed* ped) {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        if (SafePedNodeWorldPos(ped, PED_NODE_HEAD, &x, &y, &z)) {
            const CVector base = ped->GetPosition();
            const float dx = x - base.x;
            const float dy = y - base.y;
            const float dz = z - base.z;
            if (dx * dx + dy * dy + dz * dz <= 36.0f
                && std::isfinite(x) && std::isfinite(y) && std::isfinite(z)) {
                return CVector(x, y, z - kHeadAimDrop);
            }
        }
        if (SafePedNodeWorldPos(ped, PED_NODE_TORSO, &x, &y, &z)) {
            const CVector base = ped->GetPosition();
            const float dx = x - base.x;
            const float dy = y - base.y;
            const float dz = z - base.z;
            if (dx * dx + dy * dy + dz * dz <= 36.0f
                && std::isfinite(x) && std::isfinite(y) && std::isfinite(z)) {
                // 躯干中心略抬到上胸，命中体积更大
                return CVector(x, y, z + 0.10f);
            }
        }
        CVector pos = ped->GetPosition();
        pos.z += kPedAimZFallback;
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

    void ApplyHardLockAim(const CVector& worldTarget) {
        if (!MenuState::WeaponBulletHardLock) {
            return;
        }
        const int idx = TheCamera.m_nActiveCam;
        if (idx < 0 || idx > 2) {
            return;
        }
        CCam& cam = TheCamera.m_aCams[idx];
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
        float horiz = std::atan2(nx, ny);
        constexpr float kMaxPitch = 1.35f;
        if (vert > kMaxPitch) {
            vert = kMaxPitch;
        }
        if (vert < -kMaxPitch) {
            vert = -kMaxPitch;
        }
        cam.m_fHorizontalAngle = horiz;
        cam.m_fVerticalAngle = vert;
        const float cv = std::cos(vert);
        const float sv = std::sin(vert);
        const float sh = std::sin(horiz);
        const float ch = std::cos(horiz);
        cam.m_vecFront = CVector(sh * cv, ch * cv, sv);
    }

    bool PlayerWantsHardLockInput() {
        CPad* pad = CPad::GetPad(0);
        if (!pad) {
            return false;
        }
        if (pad->GetTarget()) {
            return true;
        }
        if (pad->GetWeapon() != 0) {
            return true;
        }
        if (pad->WeaponJustDown()) {
            return true;
        }
        if (CPad::NewMouseControllerState.lmb) {
            return true;
        }
        return false;
    }

    float ScoreTrackAim(const CVector& camOrigin, const CVector& camFront, const CVector& aim, float distSq) {
        const float dx = aim.x - camOrigin.x;
        const float dy = aim.y - camOrigin.y;
        const float dz = aim.z - camOrigin.z;
        const float len = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (len < 0.05f || !std::isfinite(len)) {
            return -1.0f;
        }
        const float dot = (dx * camFront.x + dy * camFront.y + dz * camFront.z) / len;
        if (dot < 0.25f) {
            return -1.0f;
        }
        const float range = LockRange();
        const float distNorm = (range > 1.0f) ? (std::sqrt(distSq) / range) : 1.0f;
        return dot * 3.0f - distNorm * 0.35f;
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

    std::vector<TrackCandidate> CollectTrackCandidates() {
        std::vector<TrackCandidate> candidates;
        CPed* player = FindPlayerPed();
        if (!player || !CPools::ms_pPedPool) {
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
            return a.score > b.score;
        };

        for (CPed* ped : CPools::ms_pPedPool) {
            if (!IsTrackablePed(ped, player)) {
                continue;
            }
            const CVector pos = PedAimPoint(ped);
            const float d2 = Dist2(playerPos, pos);
            if (d2 > maxRangeSq) {
                continue;
            }

            float score = 0.0f;
            if (hasCam) {
                score = ScoreTrackAim(camOrigin, camFront, pos, d2);
                if (score < 0.0f) {
                    continue;
                }
            } else {
                score = 1.0f / (1.0f + d2);
            }

            TrackCandidate item{ pos, d2, score };
            if (static_cast<int>(candidates.size()) < maxTargets) {
                candidates.push_back(item);
                std::push_heap(candidates.begin(), candidates.end(), worseScore);
                continue;
            }
            if (score <= candidates.front().score) {
                continue;
            }
            std::pop_heap(candidates.begin(), candidates.end(), worseScore);
            candidates.back() = item;
            std::push_heap(candidates.begin(), candidates.end(), worseScore);
        }

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

    void BeginShotTrack() {
        s_shotTrack.hasTarget = false;
        s_shotTrack.target = CVector{};
        if (!MenuState::WeaponBulletTrack) {
            return;
        }

        const std::vector<TrackCandidate> candidates = CollectTrackCandidates();
        RefreshLockVisuals(candidates);
        if (candidates.empty()) {
            return;
        }

        if (candidates.size() == 1 || MaxTrackTargets() == 1) {
            s_shotTrack.target = candidates[0].pos;
        } else {
            s_shotTrack.target = candidates[s_trackRoundRobin % candidates.size()].pos;
            ++s_trackRoundRobin;
        }
        s_shotTrack.hasTarget = true;
        ApplyHardLockAim(s_shotTrack.target);
    }

    void EndShotTrack() {
        s_shotTrack.hasTarget = false;
    }

    bool ResolveShotTrackTarget(CVector& outTarget) {
        if (!MenuState::WeaponBulletTrack) {
            return false;
        }
        // 开火中若入口未绑上（偶发），LOS 里再绑一次，避免本发整段空追踪
        if (InPlayerBullet() && !s_shotTrack.hasTarget) {
            BeginShotTrack();
        }
        if (InPlayerBullet()) {
            if (!s_shotTrack.hasTarget) {
                return false;
            }
            outTarget = s_shotTrack.target;
            return true;
        }
        const std::vector<TrackCandidate> candidates = CollectTrackCandidates();
        RefreshLockVisuals(candidates);
        if (candidates.empty()) {
            return false;
        }
        outTarget = candidates[0].pos;
        return true;
    }

    PlayerShotScope::PlayerShotScope(bool enable) {
        if (!enable) {
            return;
        }
        active = true;
        ++s_playerBulletDepth;
        if (s_playerBulletDepth == 1) {
            BeginShotTrack();
        }
    }

    PlayerShotScope::~PlayerShotScope() {
        if (!active) {
            return;
        }
        if (s_playerBulletDepth == 1) {
            EndShotTrack();
        }
        --s_playerBulletDepth;
    }

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

        // 相机射线不碰；玩家开火栈内一律可改 Dest（VC 部分武器 flags 组合不稳定）
        if (!InPlayerBullet() || doCameraIgnoreCheck) {
            return oProcessLineOfSight(
                origin, target, outColPoint, outEntity,
                buildings, vehicles, peds, objects, dummies,
                doSeeThroughCheck, doCameraIgnoreCheck, doShootThroughCheck);
        }

        CVector redirected = target;
        bool tracked = false;
        if (MenuState::WeaponBulletTrack) {
            CVector trackPos{};
            if (ResolveShotTrackTarget(trackPos)) {
                // 略延长，避免终点刚好擦过碰撞体
                redirected = ExtendPast(origin, trackPos, kTrackRayPast);
                tracked = true;
            }
        }

        bool useBuildings = buildings;
        bool useObjects = objects;
        bool useDummies = dummies;
        bool usePeds = peds;
        // 追踪成功：保证测行人，并软忽略建筑/物体，避免改 Dest 后仍被墙/道具挡住像“没追踪”
        if (tracked) {
            usePeds = true;
            useBuildings = false;
            useObjects = false;
            useDummies = false;
        }
        if (MenuState::WeaponBulletThroughWalls) {
            useBuildings = false;
            useObjects = false;
            useDummies = false;
        }

        const bool hit = oProcessLineOfSight(
            origin, redirected, outColPoint, outEntity,
            useBuildings, vehicles, usePeds, useObjects, useDummies,
            doSeeThroughCheck, doCameraIgnoreCheck, doShootThroughCheck);

        if (hit && (tracked || MenuState::WeaponBulletThroughWalls) && outEntity
            && outEntity->m_nType == ENTITY_TYPE_VEHICLE) {
            auto* veh = static_cast<CVehicle*>(outEntity);
            if (!veh || veh->m_fHealth <= 0.0f) {
                outEntity = nullptr;
                return false;
            }
        }
        return hit;
    }

    bool __fastcall hkFire(CWeapon* self, void* /*edx*/, CEntity* firingEntity, CVector* sourcePos) {
        // 外层 Fire 建栈，覆盖更多 VC 开火路径
        PlayerShotScope scope(IsLocalPlayerEntity(firingEntity));
        if (!oFire) {
            return false;
        }
        return oFire(self, firingEntity, sourcePos);
    }

    bool __fastcall hkFireInstantHit(CWeapon* self, void* /*edx*/, CEntity* firingEntity, CVector* sourcePos) {
        PlayerShotScope scope(IsLocalPlayerEntity(firingEntity));
        if (!oFireInstantHit) {
            return false;
        }
        return oFireInstantHit(self, firingEntity, sourcePos);
    }

    bool __fastcall hkFireInstantHitFromCar(CWeapon* self, void* /*edx*/, CVehicle* vehicle, bool left, bool right) {
        CPed* player = FindPlayerPed();
        const bool playerShot = player && player->m_pVehicle == vehicle;
        PlayerShotScope scope(playerShot);
        if (!oFireInstantHitFromCar) {
            return false;
        }
        return oFireInstantHitFromCar(self, vehicle, left, right);
    }

    void EnsureHook() {
        if (s_hookAttempted) {
            return;
        }
        s_hookAttempted = true;

        MH_STATUS initStatus = MH_Initialize();
        if (initStatus != MH_OK && initStatus != MH_ERROR_ALREADY_INITIALIZED) {
            Log::Warn("BulletAssist VC: MinHook 初始化失败");
            return;
        }

        // CWeapon::Fire @ 0x5D45E0
        {
            const MH_STATUS st = MH_CreateHook(
                reinterpret_cast<void*>(0x5D45E0),
                reinterpret_cast<void*>(&hkFire),
                reinterpret_cast<void**>(&oFire));
            if (st == MH_OK || st == MH_ERROR_ALREADY_CREATED) {
                MH_EnableHook(reinterpret_cast<void*>(0x5D45E0));
            } else {
                Log::Warn("BulletAssist VC: Fire hook 失败");
            }
        }
        {
            const MH_STATUS st = MH_CreateHook(
                reinterpret_cast<void*>(0x5D1140),
                reinterpret_cast<void*>(&hkFireInstantHit),
                reinterpret_cast<void**>(&oFireInstantHit));
            if (st == MH_OK || st == MH_ERROR_ALREADY_CREATED) {
                MH_EnableHook(reinterpret_cast<void*>(0x5D1140));
            } else {
                Log::Warn("BulletAssist VC: FireInstantHit hook 失败");
            }
        }
        {
            const MH_STATUS st = MH_CreateHook(
                reinterpret_cast<void*>(0x5CB0A0),
                reinterpret_cast<void*>(&hkFireInstantHitFromCar),
                reinterpret_cast<void**>(&oFireInstantHitFromCar));
            if (st == MH_OK || st == MH_ERROR_ALREADY_CREATED) {
                MH_EnableHook(reinterpret_cast<void*>(0x5CB0A0));
            }
        }
        {
            const MH_STATUS st = MH_CreateHook(
                reinterpret_cast<void*>(0x4D92D0),
                reinterpret_cast<void*>(&hkProcessLineOfSight),
                reinterpret_cast<void**>(&oProcessLineOfSight));
            if (st != MH_OK && st != MH_ERROR_ALREADY_CREATED) {
                Log::Warn("BulletAssist VC: ProcessLineOfSight hook 失败");
                return;
            }
            const MH_STATUS en = MH_EnableHook(reinterpret_cast<void*>(0x4D92D0));
            if (en != MH_OK && en != MH_ERROR_ENABLED) {
                Log::Warn("BulletAssist VC: 启用 ProcessLineOfSight hook 失败");
                return;
            }
        }

        Log::Info("BulletAssist VC: Fire/InstantHit/LOS 追踪 hook 已就绪");
    }

    bool WorldToScreen(const CVector& world, ImVec2& out) {
        RwV3d in{};
        in.x = world.x;
        in.y = world.y;
        in.z = world.z;
        RwV3d scr{};
        float w = 0.0f;
        float h = 0.0f;
        if (!CSprite::CalcScreenCoors(in, &scr, &w, &h, true) || w < 1.0f || h < 1.0f) {
            return false;
        }
        out = ImVec2(scr.x, scr.y);
        return true;
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

    void DrawEntityBound(ImDrawList* dl, CEntity* ent, ImU32 color) {
        CColModel* col = ent->GetColModel();
        if (!col) {
            return;
        }
        DrawLocalBoxWire(dl, ent, col->m_boundBox.m_vecMin, col->m_boundBox.m_vecMax, color);
    }

    void DrawEntityColPartial(ImDrawList* dl, CEntity* ent, ImU32 boxColor, ImU32 sphereColor) {
        CColModel* col = ent->GetColModel();
        if (!col) {
            return;
        }
        if (col->m_pBoxes && col->m_nNumBoxes > 0) {
            for (unsigned short i = 0; i < col->m_nNumBoxes; ++i) {
                DrawLocalBoxWire(dl, ent, col->m_pBoxes[i].m_vecMin, col->m_pBoxes[i].m_vecMax, boxColor);
            }
        }
        if (col->m_pSpheres && col->m_nNumSpheres > 0) {
            for (unsigned short i = 0; i < col->m_nNumSpheres; ++i) {
                DrawLocalSphereWire(dl, ent, col->m_pSpheres[i].m_vecCenter, col->m_pSpheres[i].m_fRadius, sphereColor);
            }
        }
    }

    // m_apFrames / RwFrame 可能悬空 必须 SEH，仅 POD 进出
    bool SafePedNodeWorldPos(CPed* ped, int node, float* ox, float* oy, float* oz) {
        if (!ped || !ox || !oy || !oz || node < 0 || node >= 18) {
            return false;
        }
        __try {
            AnimBlendFrameData* data = ped->m_apFrames[node];
            if (!data) {
                return false;
            }
            RwFrame* frame = data->m_pFrame;
            if (!frame) {
                return false;
            }
            RwMatrix* ltm = RwFrameGetLTM(frame);
            if (!ltm) {
                return false;
            }
            *ox = ltm->pos.x;
            *oy = ltm->pos.y;
            *oz = ltm->pos.z;
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    bool FrameWorldPos(CPed* ped, int node, CVector& out) {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        if (!SafePedNodeWorldPos(ped, node, &x, &y, &z)) {
            return false;
        }
        // 过滤远离实体的坏点
        const CVector base = ped->GetPosition();
        const float dx = x - base.x;
        const float dy = y - base.y;
        const float dz = z - base.z;
        if (dx * dx + dy * dy + dz * dz > 36.0f) {
            return false;
        }
        // 过滤 NaN / 极端值
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
            return false;
        }
        out = CVector(x, y, z);
        return true;
    }

    // 帧骨骼不可用时：用 ColModel 球体中心连成“身体骨架”（同样走 SDK 碰撞数据）
    void DrawPedColSphereSkeleton(ImDrawList* dl, CPed* ped, ImU32 color) {
        CColModel* col = ped->GetColModel();
        if (!col || !col->m_pSpheres || col->m_nNumSpheres < 2) {
            return;
        }
        const int n = col->m_nNumSpheres > 16 ? 16 : static_cast<int>(col->m_nNumSpheres);
        CVector pts[16];
        for (int i = 0; i < n; ++i) {
            pts[i] = ped->TransformFromObjectSpace(col->m_pSpheres[i].m_vecCenter);
        }
        // 按 z 排序后相邻连线，近似躯干链
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                if (pts[j].z < pts[i].z) {
                    const CVector tmp = pts[i];
                    pts[i] = pts[j];
                    pts[j] = tmp;
                }
            }
        }
        for (int i = 0; i + 1 < n; ++i) {
            DrawWorldLine(dl, pts[i], pts[i + 1], color, 1.4f);
        }
        for (int i = 0; i < n; ++i) {
            ImVec2 s{};
            if (WorldToScreen(pts[i], s)) {
                dl->AddCircleFilled(s, 2.5f, color, 6);
            }
        }
    }

    void DrawPedSkeleton(ImDrawList* dl, CPed* ped, ImU32 color) {
        if (!ped || !ped->m_pRwClump || !ped->bIsVisible) {
            return;
        }
        // 未进渲染的行人帧层级常无效，禁止碰 RwFrame
        if (!ped->GetIsOnScreen() && !ped->bImBeingRendered) {
            return;
        }

        // 不调用 UpdateRpHAnim：对未就绪 clump 会直接崩进 RW
        static const int kBones[][2] = {
            {PED_NODE_HEAD, PED_NODE_TORSO},
            {PED_NODE_TORSO, PED_NODE_UPPER_ARM_L},
            {PED_NODE_UPPER_ARM_L, PED_NODE_HAND_L},
            {PED_NODE_TORSO, PED_NODE_UPPER_ARM_R},
            {PED_NODE_UPPER_ARM_R, PED_NODE_HAND_R},
            {PED_NODE_TORSO, PED_NODE_LEG_L},
            {PED_NODE_LEG_L, PED_NODE_FOOT_L},
            {PED_NODE_TORSO, PED_NODE_LEG_R},
            {PED_NODE_LEG_R, PED_NODE_FOOT_R},
        };

        int okLines = 0;
        for (const auto& e : kBones) {
            CVector pa{};
            CVector pb{};
            if (FrameWorldPos(ped, e[0], pa) && FrameWorldPos(ped, e[1], pb)) {
                DrawWorldLine(dl, pa, pb, color, 1.6f);
                ++okLines;
            }
        }

        if (okLines == 0) {
            DrawPedColSphereSkeleton(dl, ped, color);
            return;
        }

        static const int kNodes[] = {
            PED_NODE_TORSO, PED_NODE_HEAD,
            PED_NODE_UPPER_ARM_L, PED_NODE_UPPER_ARM_R,
            PED_NODE_HAND_L, PED_NODE_HAND_R,
            PED_NODE_LEG_L, PED_NODE_LEG_R,
            PED_NODE_FOOT_L, PED_NODE_FOOT_R,
        };
        for (int node : kNodes) {
            CVector p{};
            ImVec2 s{};
            if (FrameWorldPos(ped, node, p) && WorldToScreen(p, s)) {
                dl->AddCircleFilled(s, 2.5f, color, 6);
            }
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
            return;
        }
        EnsureHook();
        const std::vector<TrackCandidate> candidates = CollectTrackCandidates();
        RefreshLockVisuals(candidates);
        if (MenuState::WeaponBulletHardLock && PlayerWantsHardLockInput() && !candidates.empty()) {
            if (s_shotTrack.hasTarget) {
                ApplyHardLockAim(s_shotTrack.target);
            } else {
                ApplyHardLockAim(candidates[0].pos);
            }
        }
    }

    void Draw() {
        const bool pedBound = MenuState::WeaponPedEsp;
        const bool pedCol = MenuState::WeaponPedColEsp;
        const bool pedSkel = MenuState::WeaponPedSkeleton;
        const bool vehBound = MenuState::WeaponVehicleEsp;
        const bool vehCol = MenuState::WeaponVehicleColEsp;
        const bool track = MenuState::WeaponBulletTrack;
        if (!pedBound && !pedCol && !pedSkel && !vehBound && !vehCol && !track) {
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

        if ((pedBound || pedCol || pedSkel) && CPools::ms_pPedPool) {
            for (CPed* ped : CPools::ms_pPedPool) {
                if (!IsUsablePed(ped, player)) {
                    continue;
                }
                const EspRelation rel = ClassifyPedEsp(ped, player);
                if (!PassEspFilter(rel)) {
                    continue;
                }
                if (pedBound) {
                    DrawEntityBound(dl, ped, EspBoundColor(rel, pedBoundCiv));
                }
                if (pedCol) {
                    DrawEntityColPartial(dl, ped, EspBoxColor(rel, pedBoxCiv), EspSphColor(rel, pedSphCiv));
                }
                if (pedSkel) {
                    DrawPedSkeleton(dl, ped, EspSkelColor(rel, skelCiv));
                }
            }
        }

        if ((vehBound || vehCol) && CPools::ms_pVehiclePool) {
            for (CVehicle* veh : CPools::ms_pVehiclePool) {
                if (!IsUsableVehicle(veh)) {
                    continue;
                }
                const EspRelation rel = ClassifyVehicleEsp(veh, player);
                if (!PassEspFilter(rel)) {
                    continue;
                }
                if (vehBound) {
                    DrawEntityBound(dl, veh, EspBoundColor(rel, vehBoundCiv));
                }
                if (vehCol) {
                    DrawEntityColPartial(dl, veh, EspBoxColor(rel, vehBoxCiv), EspSphColor(rel, vehSphCiv));
                }
            }
        }
    }
}