#include "GameLogic.h"
#include "utils/Log.h"
#include "resources/ResourceData.h"
#include "CPlayerPed.h"
#include "CWorld.h"
#include "CPools.h"
#include "CStreaming.h"
#include "CVehicle.h"
#include "CAutomobile.h"
#include "CPed.h"
#include "CModelInfo.h"
#include "CWeaponInfo.h"
#include "CPickups.h"
#include "extensions/ScriptCommands.h"
#include "Patch.h"
#include "CTimer.h"
#include "CClock.h"
#include "CHud.h"
#include "CWeather.h"
#include "CTheScripts.h"
#include "CCutsceneMgr.h"
#include "CMenuManager.h"
#include "ui/MenuState.h"
#include "ePedType.h"
#include "utils/StdExtras.h"
#include "rw/skeleton.h"
#include "CMatrix.h"
#include "Events.h"
#include <cmath>
#include <cstdint>
#include <windows.h>
#include <string>
#include <ctime>
#include <string>

namespace {
std::vector<int> g_trackedPickupHandles;
bool g_weaponTweaksNeedReapply = true;

bool PatchByteIfExpected(const char* label, std::uintptr_t address, unsigned char original, unsigned char patched, bool enable) {
    auto* target = reinterpret_cast<unsigned char*>(address);

    MEMORY_BASIC_INFORMATION memoryInfo{};
    if (VirtualQuery(target, &memoryInfo, sizeof(memoryInfo)) == 0 || memoryInfo.State != MEM_COMMIT ||
        (memoryInfo.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0 ||
        reinterpret_cast<std::uintptr_t>(target) + 1 > reinterpret_cast<std::uintptr_t>(memoryInfo.BaseAddress) + memoryInfo.RegionSize) {
        Log::Warn(std::string(label) + " 回放补丁地址不可用，已跳过");
        return false;
    }

    const unsigned char desired = enable ? patched : original;
    if (*target == desired) {
        return true;
    }

    const unsigned char acceptableCurrent = enable ? original : patched;
    if (*target != acceptableCurrent) {
        Log::Warn(std::string(label) + " 回放补丁位置与预期不符，已跳过");
        return false;
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(target, 1, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Log::Warn(std::string(label) + " 回放补丁无法修改内存权限，已跳过");
        return false;
    }

    *target = desired;
    FlushInstructionCache(GetCurrentProcess(), target, 1);

    DWORD unusedProtect = 0;
    VirtualProtect(target, 1, oldProtect, &unusedProtect);
    return true;
}

bool PatchBytesIfExpected(const char* label, std::uintptr_t address, const unsigned char* original, const unsigned char* patched, std::size_t size, bool enable) {
    auto* target = reinterpret_cast<unsigned char*>(address);

    MEMORY_BASIC_INFORMATION memoryInfo{};
    if (VirtualQuery(target, &memoryInfo, sizeof(memoryInfo)) == 0 || memoryInfo.State != MEM_COMMIT ||
        (memoryInfo.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0 ||
        reinterpret_cast<std::uintptr_t>(target) + size > reinterpret_cast<std::uintptr_t>(memoryInfo.BaseAddress) + memoryInfo.RegionSize) {
        Log::Warn(std::string(label) + " 补丁地址不可用，已跳过");
        return false;
    }

    const unsigned char* desired = enable ? patched : original;
    if (std::memcmp(target, desired, size) == 0) {
        return true;
    }

    const unsigned char* acceptableCurrent = enable ? original : patched;
    if (std::memcmp(target, acceptableCurrent, size) != 0) {
        Log::Warn(std::string(label) + " 补丁位置与预期不符，已跳过");
        return false;
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(target, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Log::Warn(std::string(label) + " 补丁无法修改内存权限，已跳过");
        return false;
    }

    std::memcpy(target, desired, size);
    FlushInstructionCache(GetCurrentProcess(), target, size);

    DWORD unusedProtect = 0;
    VirtualProtect(target, size, oldProtect, &unusedProtect);
    return true;
}

    bool IsSpawnPositionClear(const CVector& pos) {
        for (CVehicle* vehicle : CPools::ms_pVehiclePool) {
            if (!vehicle) {
                continue;
            }

            const CVector other = vehicle->GetPosition();
            const float dx = other.x - pos.x;
            const float dy = other.y - pos.y;
            const float dz = other.z - pos.z;
            if (dx * dx + dy * dy + dz * dz < 64.0f) {
                return false;
            }
        }

        return !plugin::Command<plugin::Commands::IS_POINT_OBSCURED_BY_A_MISSION_ENTITY>(
            pos.x - 3.0f, pos.y - 3.0f, pos.z - 1.0f,
            pos.x + 3.0f, pos.y + 3.0f, pos.z + 3.0f
        );
    }

    CVector FindSideSpawnPosition(CPlayerPed* player, const CVector& origin) {
        if (!player) {
            return origin;
        }

        const float heading = player->GetHeading() * 0.01745329252f;
        const float forwardX = std::sin(heading);
        const float forwardY = std::cos(heading);
        const float rightX = forwardY;
        const float rightY = -forwardX;

        const float offsets[][2] = {
            { 8.0f, 2.0f },
            { -8.0f, 2.0f },
            { 10.0f, -2.0f },
            { -10.0f, -2.0f },
            { 0.0f, 12.0f },
            { 0.0f, -12.0f },
            { 14.0f, 0.0f },
            { -14.0f, 0.0f }
        };

        for (const auto& offset : offsets) {
            CVector candidate(
                origin.x + rightX * offset[0] + forwardX * offset[1],
                origin.y + rightY * offset[0] + forwardY * offset[1],
                origin.z + 3.0f
            );

            float groundZ = candidate.z;
            if (plugin::Command<plugin::Commands::GET_GROUND_Z_FOR_3D_COORD>(candidate.x, candidate.y, candidate.z + 20.0f, &groundZ)) {
                candidate.z = groundZ + 1.0f;
            }

            if (IsSpawnPositionClear(candidate)) {
                return candidate;
            }
        }

        return CVector(origin.x + rightX * 14.0f, origin.y + rightY * 14.0f, origin.z + 1.0f);
    }
}

namespace GameLogic {

namespace {
    int g_worldReadyStreak = 0;
    int g_initGeneration = 0;
    int g_readyGeneration = -1;
    bool g_logicHooksInstalled = false;
    bool g_loggedCleo = false;
    int g_keepStuffApplied = -1;
    int g_framesSinceInitGame = 0;
    bool g_addressProbeUnsafe = false;   // 一旦SEH或明显坏指针，III本会话不再尝试就绪
    int g_postTransitionCooldown = 0;       // 过场/大模型流结束后的短冷却

    bool IsCleoPresent() {
        static int cached = -1;
        if (cached < 0) {
            cached = (GetModuleHandleA("III.CLEO.asi")
                || GetModuleHandleA("CLEO.asi")
                || GetModuleHandleA("cleo.asi")
                || GetModuleHandleA("III.CLEO.dll")) ? 1 : 0;
        }
        return cached == 1;
    }

    int RequiredReadyStreak() {
        // ~0.5s / ~2.5s @60fps；有 CLEO 时 Init 后还要 LoadCustomScripts
        return IsCleoPresent() ? 150 : 30;
    }

    // 任务过场加载会同步抽 TXD/Clump（崩溃点 0x5ABD80 落在 RW 流附近，读 target=0xD）
    // sticky-ready 不能盖过这段窗口：ped/脚本表面仍“活着”，但 RW/流未稳
    bool IsCutsceneOrStreamBusy() {
#if defined(GTA3) || !(defined(GTASA) || defined(GTAVC))
        if (g_addressProbeUnsafe) return true;
        __try {
#endif
            if (CCutsceneMgr::ms_cutsceneProcessing) return true;
            if (CCutsceneMgr::ms_running) return true;
            // loadStatus：0 空闲；非 0 表示正在装/刚装完未进入 running 的过渡
            if (CCutsceneMgr::ms_cutsceneLoadStatus != 0 && !CCutsceneMgr::ms_loaded) return true;
            if (CStreaming::ms_bLoadingBigModel) return true;
            if (CTimer::m_CodePause) return true;
            if (FrontEndMenuManager.m_bMenuActive) return true;
            if (FrontEndMenuManager.m_bGameNotLoaded) return true;
            return false;
#if defined(GTA3) || !(defined(GTASA) || defined(GTAVC))
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            g_addressProbeUnsafe = true;
            Log::Error("III 过场/流状态探测异常，已锁定未就绪");
            return true;
        }
#endif
    }

    void MarkWorldUnsettled() {
        g_worldReadyStreak = 0;
        g_readyGeneration = -1;
    }

    bool ArePoolsAlive() {
#if defined(GTA3) || !(defined(GTASA) || defined(GTAVC))
        if (g_addressProbeUnsafe) return false;
        __try {
#endif
            auto* pedPool = CPools::ms_pPedPool;
            auto* vehPool = CPools::ms_pVehiclePool;
            if (reinterpret_cast<uintptr_t>(pedPool) < 0x10000) return false;
            if (reinterpret_cast<uintptr_t>(vehPool) < 0x10000) return false;
            if (!pedPool || !pedPool->m_pObjects || !pedPool->m_byteMap || pedPool->m_nSize <= 0) {
                return false;
            }
            if (!vehPool || !vehPool->m_pObjects || !vehPool->m_byteMap || vehPool->m_nSize <= 0) {
                return false;
            }
            return true;
#if defined(GTA3) || !(defined(GTASA) || defined(GTAVC))
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            g_addressProbeUnsafe = true;
            Log::Error("III 池指针探测异常（疑似EXE布局与plugin-sdk不匹配或CLEO冲突），已锁定未就绪");
            return false;
        }
#endif
    }

    // 崩溃点 0x438FE4 = CTheScripts::Init → AddScriptToList 读坏链表
    // Init 中途 pActiveScripts 可能已非空；必须等 ProcessScripts 真正跑过
    // ScriptsUpdated / CommandsExecuted 只在脚本循环里增长，Init 窗口保持 0
    // 额外：若plugin-sdk硬编码地址在当前EXE下无效，SEH会捕获并永久未就绪
    bool AreScriptsSettled() {
#if defined(GTA3) || !(defined(GTASA) || defined(GTAVC))
        if (g_addressProbeUnsafe) return false;
        __try {
#endif
            // 只读全局计数与头指针，不调 ProcessOneCommand / 不遍历链表节点
            CRunningScript* pa = CTheScripts::pActiveScripts;
            if (pa == nullptr) return false;
            if (reinterpret_cast<uintptr_t>(pa) < 0x10000) return false;
            // 计数器若地址错会读到随机；此处保守：两个都为0才认为未跑过
            if (CTheScripts::ScriptsUpdated == 0 && CTheScripts::CommandsExecuted == 0) {
                return false;
            }
            return true;
#if defined(GTA3) || !(defined(GTASA) || defined(GTAVC))
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            g_addressProbeUnsafe = true;
            Log::Error("III 脚本计数探测异常，已锁定未就绪以保护CTheScripts::Init");
            return false;
        }
#endif
    }

    CPlayerPed* TryGetLocalPlayerPed() {
#if defined(GTA3) || !(defined(GTASA) || defined(GTAVC))
        if (g_addressProbeUnsafe) return nullptr;
        __try {
#endif
            if (!ArePoolsAlive()) {
                return nullptr;
            }
            if (!CWorld::Players) {
                return nullptr;
            }
            if (reinterpret_cast<uintptr_t>(CWorld::Players) < 0x10000) {
                return nullptr;
            }
            // 不走 FindPlayerPed，避免半初始化时进游戏函数
            const unsigned char focus = CWorld::PlayerInFocus;
            if (focus > 1) {
                return nullptr;
            }
            CPlayerPed* ped = CWorld::Players[focus].m_pPed;
            if (!ped) {
                return nullptr;
            }

            auto* pool = CPools::ms_pPedPool;
            if (reinterpret_cast<uintptr_t>(pool) < 0x10000) return nullptr;
            const int index = pool->GetIndex(ped);
            if (index < 0 || index >= pool->m_nSize) {
                return nullptr;
            }
            if (pool->IsFreeSlotAtIndex(index)) {
                return nullptr;
            }
            return ped;
#if defined(GTA3) || !(defined(GTASA) || defined(GTAVC))
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            g_addressProbeUnsafe = true;
            Log::Error("III 玩家ped探测异常，已锁定未就绪");
            return nullptr;
        }
#endif
    }
}

void NotifyGameInit() {
    ++g_initGeneration;
    g_worldReadyStreak = 0;
    g_readyGeneration = -1;
    g_framesSinceInitGame = 0;
    g_postTransitionCooldown = 0;
    g_keepStuffApplied = -1; // 新游戏/读档代码段可能被还原，下次按需重打补丁
    // g_addressProbeUnsafe 保留：布局不随读档变，保持锁定更安全
    if (!g_loggedCleo && IsCleoPresent()) {
        g_loggedCleo = true;
        Log::Info("检测到 III.CLEO：延长 world-ready 冷却，避开 CTheScripts::Init/AddScriptToList 窗口");
    }
}

bool IsWorldReady() {
    ++g_framesSinceInitGame;

    if (g_addressProbeUnsafe) {
        return false;
    }

    // initGame 刚发生后给游戏/CLEO 更多帧完成脚本链表构建
    const int minCd = IsCleoPresent() ? 90 : 15;
    if (g_framesSinceInitGame < minCd) {
        return false;
    }

    // 必须先于 sticky-ready：过场加载时 ped 仍在，旧逻辑会一直 true 并继续跑菜单逻辑
    if (IsCutsceneOrStreamBusy()) {
        MarkWorldUnsettled();
        g_postTransitionCooldown = IsCleoPresent() ? 45 : 20;
        return false;
    }

    if (g_postTransitionCooldown > 0) {
        --g_postTransitionCooldown;
        MarkWorldUnsettled();
        return false;
    }

    if (!ArePoolsAlive() || !AreScriptsSettled() || !TryGetLocalPlayerPed()) {
        MarkWorldUnsettled();
        return false;
    }

    // 本代 init 已就绪则保持 true（避免 streak 在边界抖动）
    if (g_readyGeneration == g_initGeneration) {
        return true;
    }

    const int need = RequiredReadyStreak();
    if (g_worldReadyStreak < need) {
        ++g_worldReadyStreak;
        return false;
    }
    g_readyGeneration = g_initGeneration;
    return true;
}

void Init() {
    // 新游戏/读档会再次走 bootstrap，钩子只装一次
    // 注意：不要 hook initScriptsEvent（0x48C26B 等），CLEO 已 RedirectCall 同一批地址
    if (g_logicHooksInstalled) {
        return;
    }
    g_logicHooksInstalled = true;

    // III BigHead：对齐 Cheat-Menu 的 pre-render 钩子（非脚本路径）
    // 过场/流窗口禁止改 RwFrame；此处不调 IsWorldReady()（其会 ++ 帧计数）
    static plugin::CdeclEvent<plugin::AddressList<0x4CFE12, plugin::H_CALL>, plugin::PRIORITY_AFTER, plugin::ArgPickN<CPed*, 0>, void(CPed*)> onPreRender;
    onPreRender += [](CPed* ped) {
        if (!MenuState::BigHeadMode || !ped) {
            return;
        }
        if (g_addressProbeUnsafe || g_readyGeneration != g_initGeneration || g_postTransitionCooldown > 0) {
            return;
        }
        if (IsCutsceneOrStreamBusy()) {
            return;
        }
        if (!ped->m_pRwObject || !ped->m_apFrames[2]) {
            return;
        }
        RwFrame* frame = ped->m_apFrames[2]->m_pFrame;
        if (!frame) {
            return;
        }
        RwMatrix* headMatrix = RwFrameGetMatrix(frame);
        if (!headMatrix) {
            return;
        }
        CMatrix mat;
        mat.m_pAttachMatrix = nullptr;
        mat.Attach(headMatrix, false);
        mat.SetScale(3.0f);
        mat.pos.x = 0.4f;
        mat.pos.y = 0.0f;
        mat.pos.z = 0.0f;
        mat.UpdateRW();
    };
}

ProofState GetPlayerProofState(CPlayerPed* player) {
    ProofState state;
    if (!player) return state;
    state.bullet = player->bBulletProof;
    state.collision = player->bCollisionProof;
    state.explosion = player->bExplosionProof;
    state.fire = player->bFireProof;
    state.melee = player->bMeleeProof;
    state.nonPlayer = player->bOnlyDamagedByPlayer;
    return state;
}

void SetPlayerProofState(CPlayerPed* player, const ProofState& state) {
    if (!player) return;
    player->bBulletProof = state.bullet;
    player->bCollisionProof = state.collision;
    player->bExplosionProof = state.explosion;
    player->bFireProof = state.fire;
    player->bMeleeProof = state.melee;
    player->bOnlyDamagedByPlayer = state.nonPlayer;
}

void ApplyGodMode(CPlayerPed* player, bool enable) {
    if (!enable || !player) return;
    player->bBulletProof = enable;
    player->bCollisionProof = enable;
    player->bExplosionProof = enable;
    player->bFireProof = enable;
    player->bMeleeProof = enable;
    player->bOnlyDamagedByPlayer = enable;
}

void SetInfiniteSprint(bool enable) {
    CPlayerInfo* pInfo = &CWorld::Players[CWorld::PlayerInFocus];
    pInfo->m_bInfiniteSprint = enable;
}

void SetFreeHealthcare(bool enable) {
    CPlayerInfo* pInfo = &CWorld::Players[CWorld::PlayerInFocus];
    pInfo->m_bGetOutOfHospitalFree = enable;
}

bool GetFreeHealthcare() {
    CPlayerInfo* pInfo = &CWorld::Players[CWorld::PlayerInFocus];
    return pInfo->m_bGetOutOfHospitalFree;
}

void SetFreeJail(bool enable) {
    CPlayerInfo* pInfo = &CWorld::Players[CWorld::PlayerInFocus];
    pInfo->m_bGetOutOfJailFree = enable;
}

bool GetFreeJail() {
    CPlayerInfo* pInfo = &CWorld::Players[CWorld::PlayerInFocus];
    return pInfo->m_bGetOutOfJailFree;
}

void GiveMoney(int amount) {
    CPlayerInfo* pInfo = &CWorld::Players[CWorld::PlayerInFocus];
    pInfo->m_nMoney += amount;
    pInfo->m_nDisplayMoney += amount;
}

int GetMoney() {
    CPlayerInfo* pInfo = &CWorld::Players[CWorld::PlayerInFocus];
    return pInfo->m_nMoney;
}

void SetMoney(int amount) {
    CPlayerInfo* pInfo = &CWorld::Players[CWorld::PlayerInFocus];
    pInfo->m_nMoney = amount;
    pInfo->m_nDisplayMoney = amount;
}

float GetHealth(CPlayerPed* player) {
    return player ? player->m_fHealth : 0.0f;
}

void SetHealth(CPlayerPed* player, float value) {
    if (player) player->m_fHealth = value;
}

float GetArmour(CPlayerPed* player) {
    return player ? player->m_fArmour : 0.0f;
}

void SetArmour(CPlayerPed* player, float value) {
    if (player) player->m_fArmour = value;
}

CVector GetPlayerPosition(CPlayerPed* player) {
    return player ? player->GetPosition() : CVector(0.0f, 0.0f, 0.0f);
}

void MovePlayerRelative(CPlayerPed* player, float forward, float right, float up) {
    if (!player) {
        return;
    }

    CVector pos = player->GetPosition();
    const float angle = player->m_fHeadingCurrent;
    const float forwardX = -std::sin(angle);
    const float forwardY = std::cos(angle);
    const float rightX = forwardY;
    const float rightY = -forwardX;
    pos.x += forwardX * forward + rightX * right;
    pos.y += forwardY * forward + rightY * right;
    pos.z += up;
    TeleportPlayer(pos);
}

bool IsPlayerDead(CPlayerPed* player) {
    return player ? player->m_fHealth <= 0.0f : false;
}

void SetKeepStuff(bool enable) {
    // 每帧重写会与其它 ASI（含 CLEO 加载窗口）抢代码页；仅在状态变化时打补丁
    const int want = enable ? 1 : 0;
    if (g_keepStuffApplied == want) {
        return;
    }
    if (enable) {
        plugin::patch::Nop(0x421507, 7);
        plugin::patch::Nop(0x421724, 7);
        plugin::patch::Nop(0x4217F8, 8);
    } else {
        plugin::patch::SetRaw(0x421507, (void*)"\x8B\x0B\xE8\x62\xE6\x0A\x00", 7);
        plugin::patch::SetRaw(0x421724, (void*)"\x8B\x0B\xE8\x45\xE4\x0A\x00", 7);
        plugin::patch::SetRaw(0x4217F8, (void*)"\x83\xC4\x14\xE8\x73\xE3\x0A\x00", 8);
    }
    g_keepStuffApplied = want;
}

void HealPlayer(CPlayerPed* player) {
    if (player) player->m_fHealth = 100.0f;
}

void GiveArmour(CPlayerPed* player) {
    if (player) player->m_fArmour = 100.0f;
}

void ProcessAutoHeal(CPlayerPed* player, bool enable) {
    if (!enable || !player) return;
    static unsigned int lastTimer = 0;
    unsigned int timer = CTimer::m_snTimeInMilliseconds;
    if (timer - lastTimer > 1000) {
        if (player->m_fHealth < 100.0f) {
            player->m_fHealth += 2.0f;
            if (player->m_fHealth > 100.0f) player->m_fHealth = 100.0f;
        } else if (player->m_fArmour < 100.0f && player->m_fArmour > 0.0f) {
            player->m_fArmour += 2.0f;
            if (player->m_fArmour > 100.0f) player->m_fArmour = 100.0f;
        }
        lastTimer = timer;
    }
}

void SetWantedLevel(CPlayerPed* player, int level) {
    if (player) player->SetWantedLevel(level);
}

int GetWantedLevel(CPlayerPed* player) {
    return player ? player->m_pWanted->m_nWantedLevel : 0;
}

void ProcessHardMode(CPlayerPed* player, bool enable) {
    if (!enable || !player) return;
    if (player->m_fHealth > 50.0f) player->m_fHealth = 50.0f;
    player->m_fArmour = 0.0f;
}

void ProcessRespawnAtDeathPosition(CPlayerPed* player, bool enable) {
    static CVector deathPos(0.0f, 0.0f, 0.0f);
    static bool hasDeathPos = false;
    if (!enable || !player) {
        hasDeathPos = false;
        return;
    }

    if (IsPlayerDead(player)) {
        deathPos = player->GetPosition();
        hasDeathPos = true;
        return;
    }

    const CVector current = player->GetPosition();
    if (hasDeathPos && deathPos.x != 0.0f && deathPos.y != 0.0f && (deathPos.x != current.x || deathPos.y != current.y)) {
        player->Teleport(deathPos);
        hasDeathPos = false;
    }
}

void ProcessFreezeWantedLevel(CPlayerPed* player, bool enable, int level) {
    if (!enable || !player) return;
    SetWantedLevel(player, level);
}

void SetManualPlayerProof(CPlayerPed* player, const ProofState& state) {
    SetPlayerProofState(player, state);
}

bool RequestSaveGame() {
    plugin::Command<plugin::Commands::ACTIVATE_SAVE_MENU>();
    Log::Info("已请求打开存档菜单");
    return true;
}

ProofState GetVehicleProofState(CVehicle* vehicle) {
    ProofState state;
    if (!vehicle) return state;
    state.bullet = vehicle->bBulletProof;
    state.collision = vehicle->bCollisionProof;
    state.explosion = vehicle->bExplosionProof;
    state.fire = vehicle->bFireProof;
    state.melee = vehicle->bMeleeProof;
    state.nonPlayer = vehicle->bOnlyDamagedByPlayer;
    return state;
}

void SetVehicleProofState(CVehicle* vehicle, const ProofState& state) {
    if (!vehicle) return;
    vehicle->bBulletProof = state.bullet;
    vehicle->bCollisionProof = state.collision;
    vehicle->bExplosionProof = state.explosion;
    vehicle->bFireProof = state.fire;
    vehicle->bMeleeProof = state.melee;
    vehicle->bOnlyDamagedByPlayer = state.nonPlayer;
}

void RepairVehicle(CVehicle* vehicle) {
    if (!vehicle) return;
    if (vehicle->m_nVehicleClass == 0) {
        reinterpret_cast<CAutomobile*>(vehicle)->Fix();
    }
    vehicle->m_fHealth = 1000.0f;
}

void StopVehicle(CVehicle* vehicle) {
    if (!vehicle) return;
    vehicle->m_vecMoveSpeed = CVector(0.0f, 0.0f, 0.0f);
    vehicle->m_vecTurnSpeed = CVector(0.0f, 0.0f, 0.0f);
}

void UnflipVehicle(CVehicle* vehicle) {
    if (!vehicle || !vehicle->IsUpsideDown()) return;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    vehicle->GetOrientation(x, y, z);
    vehicle->SetOrientation(x, y + 135.0f, z);
    StopVehicle(vehicle);
}

void SetVehicleForwardSpeed(CVehicle* vehicle, float speed) {
    if (!vehicle) return;

    CPlayerPed* player = FindPlayerPed();
    if (!player || player->m_pVehicle != vehicle) return;

    const CVector forward = vehicle->GetForward();
    const float velocity = speed / 50.0f;
    vehicle->m_vecMoveSpeed = CVector(forward.x * velocity, forward.y * velocity, forward.z * velocity);
}

void SetVehicleSpeedLock(CVehicle* vehicle, bool enable, float speed) {
    if (!enable || !vehicle) return;
    
    CPad* pad = CPad::GetPad(0);
    if (!pad || pad->GetAccelerate() <= 0) return;

    SetVehicleForwardSpeed(vehicle, speed);
}

void SetVehicleEngine(CVehicle* vehicle, bool enable) {
    if (vehicle) vehicle->bEngineOn = enable;
}

void SetVehicleInvincible(CVehicle* vehicle, bool enable) {
    if (!enable || !vehicle) return;
    vehicle->bBulletProof = enable;
    vehicle->bExplosionProof = enable;
    vehicle->bFireProof = enable;
    vehicle->bCollisionProof = enable;
    vehicle->bMeleeProof = enable;
}

void SetVehicleHeavy(CVehicle* vehicle, bool enable) {
    if (!vehicle) return;
    plugin::Command<plugin::Commands::SET_CAR_HEAVY>(CPools::GetVehicleRef(vehicle), enable);
}

void SetVehicleWatertight(CVehicle* vehicle, bool enable) {
    if (!vehicle) return;
    plugin::Command<plugin::Commands::SET_CAR_WATERTIGHT>(CPools::GetVehicleRef(vehicle), enable);
}

float GetVehicleHealth(CVehicle* vehicle) {
    return vehicle ? vehicle->m_fHealth : 0.0f;
}

void SetVehicleHealth(CVehicle* vehicle, float health) {
    if (!vehicle) return;
    vehicle->m_fHealth = health;
}

bool GetVehicleLights(CVehicle* vehicle) {
    return vehicle ? vehicle->bLightsOn : false;
}

void SetVehicleLights(CVehicle* vehicle, bool enable) {
    if (!vehicle) return;
    vehicle->bLightsOn = enable;
}

bool GetVehicleLocked(CVehicle* vehicle) {
    return vehicle ? vehicle->m_eDoorLock == CARLOCK_LOCKED_PLAYER_INSIDE : false;
}

void SetVehicleLocked(CVehicle* vehicle, bool enable) {
    if (!vehicle) return;
    vehicle->m_eDoorLock = enable ? CARLOCK_LOCKED_PLAYER_INSIDE : CARLOCK_UNLOCKED;
}

bool GetVehicleVisible(CVehicle* vehicle) {
    return vehicle ? vehicle->bIsVisible : false;
}

void SetVehicleVisible(CVehicle* vehicle, bool enable) {
    if (!vehicle) return;
    vehicle->bIsVisible = enable;
}

bool GetVehicleAlwaysSkidMarks(CVehicle*) {
    return false;
}

void SetVehicleAlwaysSkidMarks(CVehicle*, bool) {
}

bool GetVehicleDisableParticles(CVehicle*) {
    return false;
}

void SetVehicleDisableParticles(CVehicle*, bool) {
}

bool GetVehicleDriverTargetable(CVehicle*) {
    return false;
}

void SetVehicleDriverTargetable(CVehicle*, bool) {
}

bool GetVehicleHeatSeekingTargetable(CVehicle*) {
    return false;
}

void SetVehicleHeatSeekingTargetable(CVehicle*, bool) {
}

bool GetVehiclePetrolTankWeakPoint(CVehicle*) {
    return false;
}

void SetVehiclePetrolTankWeakPoint(CVehicle*, bool) {
}

bool GetVehicleSirenOrAlarm(CVehicle*) {
    return false;
}

void SetVehicleSirenOrAlarm(CVehicle*, bool) {
}

bool GetVehicleTakeLessDamage(CVehicle*) {
    return false;
}

void SetVehicleTakeLessDamage(CVehicle*, bool) {
}

void BlowUpAllVehicles() {
    CPlayerPed* player = FindPlayerPed();
    if (!player) return;

    for (CVehicle* vehicle : CPools::ms_pVehiclePool) {
        if (vehicle) {
            vehicle->BlowUpCar(player);
        }
    }
}

void DeleteVehicle(CVehicle* vehicle) {
    if (!vehicle) return;

    CPlayerPed* player = FindPlayerPed();
    if (player) {
        const int hplayer = CPools::GetPedRef(player);
        if (plugin::Command<plugin::Commands::IS_CHAR_IN_ANY_CAR>(hplayer) && player->m_pVehicle == vehicle) {
            Log::Warn("III 载具清理跳过：玩家正在使用目标载具");
            return;
        }
    }

    const int hveh = CPools::GetVehicleRef(vehicle);
    if (hveh == 0) return;

    plugin::Command<plugin::Commands::MARK_CAR_AS_NO_LONGER_NEEDED>(hveh);
    plugin::Command<plugin::Commands::DELETE_CAR>(hveh);
}

bool IsAircraftModel(int model) {
    return CModelInfo::IsHeliModel(model) || CModelInfo::IsPlaneModel(model)
        || model == 125 || model == 126 || model == 140 || model == 141 || model == 147;
}

bool IsValidVehicleModel(unsigned int modelId) {
    if (!Resources::IsKnownVehicleModel(modelId)) {
        return false;
    }

    const int model = static_cast<int>(modelId);
    switch (model) {
    case 124: // Train
    case 125: // Chopper
    case 126: // Dodo
    case 140: // Airtrain
    case 141: // Dead Dodo
    case 147: // Escape helicopter
    case 150: // Ghost
        Log::Warn("III 载具生成被拒绝：模型 ID " + std::to_string(model) + " 不适合通过普通刷车入口生成");
        return false;
    default:
        break;
    }

    return model >= 90 && model <= 150 && CModelInfo::IsVehicleModel(model);
}

bool IsValidPedModel(unsigned int modelId) {
    const int model = static_cast<int>(modelId);
    return model >= 0 && model < CModelInfo::ms_modelInfoCount && CModelInfo::IsPedModel(model);
}

bool SetPlayerSkin(unsigned int modelId) {
    Log::Warn("III 不支持安全玩家皮肤切换，已跳过模型 ID " + std::to_string(modelId));
    return false;
}

bool ApplyPlayerClothes(int textureId, int modelId, int bodyPart) {
    Log::Warn("III 不支持 CJ 衣服接口，已跳过");
    return false;
}

bool SetPlayerCustomSkin(const char* name) {
    return false;
}

bool SetPlayerStat(int statId, float value) {
    Log::Warn("III 不支持 SA stats 接口，已跳过 stat " + std::to_string(statId));
    return false;
}

CVehicle* SpawnVehicle(unsigned int modelId, const SpawnVehicleOptions& options) {
    CPlayerPed* player = FindPlayerPed();
    if (!player) return nullptr;

    const int model = static_cast<int>(modelId);
    if (!IsValidVehicleModel(modelId)) {
        Log::Warn("III 载具生成被拒绝：无效模型 ID " + std::to_string(model));
        return nullptr;
    }

    const int hplayer = CPools::GetPedRef(player);
    CVector pos = player->GetPosition();
    float speed = 0.0f;

    if (options.asDriver && options.cleanupPrevious && plugin::Command<plugin::Commands::IS_CHAR_IN_ANY_CAR>(hplayer)) {
        CVehicle* currentVehicle = player->m_pVehicle;
        if (currentVehicle) {
            const int hveh = CPools::GetVehicleRef(currentVehicle);
            pos = currentVehicle->GetPosition();
            plugin::Command<plugin::Commands::GET_CAR_SPEED>(hveh, &speed);
            plugin::Command<plugin::Commands::WARP_CHAR_FROM_CAR_TO_COORD>(hplayer, pos.x, pos.y, pos.z);
            plugin::Command<plugin::Commands::MARK_CAR_AS_NO_LONGER_NEEDED>(hveh);
        }
    } else if (!options.cleanupPrevious) {
        pos = FindSideSpawnPosition(player, pos);
    }

    if (options.aircraftInAir && IsAircraftModel(model)) {
        pos.z = 400.0f;
    } else if (options.cleanupPrevious) {
        pos.z -= 5.0f;
    }

    CStreaming::RequestModel(model, PRIORITY_REQUEST);
    CStreaming::LoadAllRequestedModels(false);

    int hveh = 0;
    if (options.asDriver) {
        plugin::Command<plugin::Commands::CREATE_CAR>(model, pos.x, pos.y, pos.z + (options.cleanupPrevious ? 4.0f : 1.0f), &hveh);
    } else {
        if (options.cleanupPrevious) {
            pos = player->TransformFromObjectSpace(CVector(0.0f, 10.0f, 0.0f));
        }
        plugin::Command<plugin::Commands::CREATE_CAR>(model, pos.x, pos.y, pos.z + 1.0f, &hveh);
    }

    CVehicle* vehicle = CPools::GetVehicle(hveh);
    if (!vehicle) {
        CStreaming::SetModelIsDeletable(model);
        Log::Error("III 载具生成失败：脚本命令 CREATE_CAR 未返回有效载具，模型 ID " + std::to_string(model));
        return nullptr;
    }

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    player->GetOrientation(x, y, z);
    vehicle->SetOrientation(x, y, z);
    vehicle->m_eDoorLock = CARLOCK_UNLOCKED;

    if (options.asDriver) {
        player->SetObjective(OBJECTIVE_ENTER_CAR_AS_DRIVER);
        player->WarpPedIntoCar(vehicle);
        SetVehicleForwardSpeed(vehicle, speed);
    }

    plugin::Command<plugin::Commands::MARK_CAR_AS_NO_LONGER_NEEDED>(hveh);
    CStreaming::SetModelIsDeletable(model);
    plugin::Command<plugin::Commands::RESTORE_CAMERA_JUMPCUT>();
    return vehicle;
}

void TeleportPlayer(CVector pos, int interiorID) {
    CPlayerPed* pPlayer = FindPlayerPed();
    if (!pPlayer) return;
    CVehicle* pVeh = pPlayer->m_pVehicle;

    CStreaming::LoadScene(pos);
    CStreaming::LoadAllRequestedModels(false);

    if (pVeh && pPlayer->m_bInVehicle) {
        pVeh->Teleport(pos);
    } else {
        pPlayer->Teleport(pos);
    }
}

void TeleportMapPosition(CVector pos, bool spawnUnderwater) {
    TeleportPlayer(pos);
}

bool TeleportMarker(bool spawnUnderwater) {
    return false;
}

void ApplyVehicleAppearance(CVehicle* vehicle, const VehicleAppearanceOptions& options) {
    if (!vehicle) return;
    plugin::Command<plugin::Commands::CHANGE_CAR_COLOUR>(CPools::GetVehicleRef(vehicle), options.primaryColor, options.secondaryColor);
}

void OpenVehicleDoor(CVehicle*, int) {
    Log::Warn("III 不支持安全车门打开接口，已跳过");
}

void PopVehicleDoor(CVehicle*, int) {
    Log::Warn("III 不支持安全车门拆卸接口，已跳过");
}

void WarpPlayerToVehicleSeat(CVehicle* vehicle, int seatIndex) {
    CPlayerPed* player = FindPlayerPed();
    if (!player || !vehicle) return;
    if (seatIndex <= 0) {
        player->SetObjective(OBJECTIVE_ENTER_CAR_AS_DRIVER);
        player->WarpPedIntoCar(vehicle);
    } else {
        plugin::Command<plugin::Commands::WARP_CHAR_INTO_CAR_AS_PASSENGER>(CPools::GetPedRef(player), CPools::GetVehicleRef(vehicle), seatIndex - 1);
    }
}

void ProcessAutoDrive(CVehicle*, bool enable, float) {
    if (enable) Log::Warn("III 不支持 AutoDrive，已安全降级");
}

void SetTrafficDensity(float density) {
    (void)density;
    // III 未接交通密度；勿每帧打日志
}

void SetFlyingCars(bool enable) {
    plugin::patch::Set<bool>(0x95CD75, enable, false);
}

void SetVehicleNoDerail(bool) {}
void SetVehicleFlipNoBurn(bool) {}
void ProcessVehicleCheats(CVehicle*) {
    plugin::patch::Set<bool>(0x95CD75, MenuState::VehicleFlyingCars, false);
    plugin::patch::Set<bool>(0x95CD66, MenuState::VehiclePerfectHandling, false);
}

CPed* SpawnPedNearPlayer(const PedSpawnOptions& options) {
    CPlayerPed* player = FindPlayerPed();
    if (!player || !IsValidPedModel(options.modelId)) return nullptr;
    const int model = static_cast<int>(options.modelId);
    CVector pos = player->TransformFromObjectSpace(CVector(0.0f, 3.0f, 0.0f));
    CStreaming::RequestModel(model, PRIORITY_REQUEST);
    CStreaming::LoadAllRequestedModels(false);
    int hped = 0;
    plugin::Command<plugin::Commands::CREATE_CHAR>(options.pedType, model, pos.x, pos.y, pos.z, &hped);
    CPed* ped = CPools::GetPed(hped);
    ApplyPedOptions(ped, options);
    plugin::Command<plugin::Commands::MARK_MODEL_AS_NO_LONGER_NEEDED>(model);
    return ped;
}

CPed* SpawnPedAtMarker(const PedSpawnOptions&) {
    Log::Warn("III 暂不支持标记点生成 Ped，已安全降级");
    return nullptr;
}

void DeletePed(CPed* ped) {
    if (!ped) return;
    plugin::Command<plugin::Commands::DELETE_CHAR>(CPools::GetPedRef(ped));
}

void ApplyPedOptions(CPed* ped, const PedSpawnOptions& options) {
    if (!ped) return;
    const int hped = CPools::GetPedRef(ped);
    plugin::Command<plugin::Commands::SET_CHAR_HEALTH>(hped, static_cast<int>(options.health));
    plugin::Command<plugin::Commands::SET_CHAR_ARMOUR>(hped, static_cast<int>(options.armour));
    plugin::Command<plugin::Commands::FREEZE_CHAR_POSITION>(hped, options.freeze);
    plugin::Command<plugin::Commands::SET_CHAR_PROOFS>(hped, false, false, false, false, false);
    if (options.weaponModel > 0) {
        plugin::Command<plugin::Commands::GIVE_WEAPON_TO_CHAR>(hped, options.weaponModel, 9999);
    }
}

void DestroyAllPeds() {
    CPlayerPed* player = FindPlayerPed();
    for (CPed* ped : CPools::ms_pPedPool) {
        if (!ped || ped == static_cast<CPed*>(player) || IsMissionPed(ped)) continue;
        DeletePed(ped);
    }
}

void SetElvisEverywhere(bool enable) { }
void SetEveryoneArmed(bool enable) { plugin::patch::Set<bool>(0x95CCF6, enable, false); }
void SetPedsMayhem(bool enable) { if(enable) plugin::Call<0x4911C0>(); }
void SetPedsAtkRocket(bool enable) { }
void SetPedsRiot(bool enable) { if(enable) plugin::Call<0x491270>(); }
void SetSlutMagnet(bool enable) { }
void SetGangsControl(bool enable) { }
void SetGangsEverywhere(bool enable) { }
void SetPedNoProstitutes(bool) {}
void SetPedNastyLimbs(bool enable) { plugin::patch::Set<bool>(0x95CD44, enable, false); }
void SetGangWarsActive(bool) {}
void StartGangWar(bool) {}
void EndGangWar() {}
int GetGangZoneDensity(int) { return 0; }
void SetGangZoneDensity(int, int) {}
unsigned int GetGangMemberModel(unsigned int, unsigned int) { return 0; }
void SetGangMemberModel(unsigned int, unsigned int, unsigned int) {}
void ResetGangModels() {}
void SetGangWeapons(unsigned int, int, int, int) {}

bool IsCopPed(const CPed* ped) {
    return ped && ped->m_ePedType == PED_TYPE_COP;
}
bool IsGangPed(const CPed* ped) {
    if (!ped) return false;
    int t = ped->m_ePedType;
    // III has limited gangs; treat any non-cop non-mission special as gang-ish if desired
    // Conservative: only explicit known if any; else few gangs in III base.
    return t >= 7 && t <= 12; // safe range guess
}

void SetPedsNoFire(bool /*enable*/) {}

void ClearPedAiming(CPed* ped) {
    if (!ped) return;
#ifdef GTA3
    ped->ClearObjective();
#else
    ped->ClearLookFlag();
#endif
}

bool ShouldSuppressPedFire(CPed* ped) {
    if (!ped) return false;
    CPlayerPed* player = FindPlayerPed();
    if (ped == static_cast<CPed*>(player)) return false;

    if (!MenuState::PedsNoFire) return false;

    // III: try createdBy; many scripts use it
    const bool isMission = (ped->m_nCharCreatedBy == 2);
    const bool isCop = IsCopPed(ped);
    const bool isGang = IsGangPed(ped);
    const bool isCiv = !isMission && !isCop && !isGang;

    if (isMission && MenuState::PedsNoFireMission) return true;
    if (isCop && MenuState::PedsNoFirePolice) return true;
    if (isGang && MenuState::PedsNoFireGangs) return true;
    if (isCiv && MenuState::PedsNoFireCivilians) return true;
    return false;
}

bool IsMissionPed(CPed* ped) {
    if (!ped) return false;
    return ped->m_nCharCreatedBy == 2;  // MISSION_CHAR
}

bool PlayPlayerAnimation(const char*, const char*, bool) {
    Log::Warn("III 暂不支持动画页播放，已安全降级");
    return false;
}

bool PlayAnimationEx(const char* group, const char* name, bool loop, bool, bool) {
    return PlayPlayerAnimation(group, name, loop);
}

void StopPlayerAnimation() {
}

void ProcessSmokingEffect(CPlayerPed*, bool) {}

void ProcessFliesEffect(CPlayerPed*, bool) {}

void ProcessVisualExtras() {}
void MaxWeaponSkills() {}
void MaxVehicleSkills() {}
void ApplyAimSkinChanger() {}
void ProcessWeaponAutoAim(bool) {}

void DisplayHud(bool enable) {
    CHud::m_Wants_To_Draw_Hud = enable;
}

void DisplayRadar(bool enable) {
    plugin::Command<plugin::Commands::DISPLAY_RADAR>(enable);
}

void SetVisualFilter(bool enable, int filterId, float) {
    if (!enable) {
        return;
    }

    if (filterId < 0) {
        filterId = 0;
    } else if (filterId > 3) {
        filterId = 3;
    }

    CWeather::OldWeatherType = static_cast<short>(filterId);
    CWeather::NewWeatherType = static_cast<short>(filterId);
}

void TeleportForward(float distance) {
    CPlayerPed* player = FindPlayerPed();
    if (!player) return;
    CVector pos = player->GetPosition();
    float angle = player->m_fHeadingCurrent;
    pos.x -= sin(angle) * distance;
    pos.y += cos(angle) * distance;
    TeleportPlayer(pos);
}

namespace {
    int GetWeaponModel(eWeaponType weaponType) {
        return plugin::CallAndReturnDynGlobal<int, int>(0x430690, static_cast<int>(weaponType));
    }

    eWeaponType GetWeaponTypeFromModel(int model) {
        for (int i = 0; i < 37; ++i) {
            const auto weaponType = static_cast<eWeaponType>(i);
            if (GetWeaponModel(weaponType) == model) {
                return weaponType;
            }
        }
        return WEAPONTYPE_UNARMED;
    }

    void ClearPlayerWeapon(CPlayerPed* player, eWeaponType weaponType) {
        if (!player) return;
        const int weaponSlot = player->GetWeaponSlot(weaponType);
        if (weaponSlot == -1) return;

        CWeapon* weapon = &player->m_aWeapons[weaponSlot];
        if (weapon->m_eWeaponType != weaponType) return;

        if (player->m_nCurrentWeapon == weaponSlot) {
            plugin::Command<plugin::Commands::SET_CURRENT_PLAYER_WEAPON>(0, WEAPONTYPE_UNARMED);
        }

        weapon->m_eWeaponState = WEAPONSTATE_OUT_OF_AMMO;
        weapon->m_nAmmoTotal = 0;
        weapon->m_nAmmoInClip = 0;
    }
}

void GiveAllWeapons(CPlayerPed* player) {
    if (!player) return;
    const unsigned int models[] = { 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183 };
    for (const unsigned int model : models) {
        GiveWeaponModel(player, model, 99999);
    }
}

void GiveWeapon(CPlayerPed* player, unsigned int weaponType, unsigned int ammo) {
    if (!player) return;
    GiveWeaponModel(player, static_cast<unsigned int>(GetWeaponModel(static_cast<eWeaponType>(weaponType))), ammo);
}

void GiveWeaponModel(CPlayerPed* player, unsigned int weaponModel, unsigned int ammo) {
    if (!player) return;
    const int hplayer = CPools::GetPedRef(player);
    const int model = static_cast<int>(weaponModel);
    CStreaming::RequestModel(model, PRIORITY_REQUEST);
    CStreaming::LoadAllRequestedModels(false);

    const eWeaponType weaponType = GetWeaponTypeFromModel(model);
    if (weaponType == WEAPONTYPE_UNARMED) {
        Log::Warn("III 武器发放被拒绝：模型未映射到有效武器类型，模型 ID " + std::to_string(model));
        plugin::Command<plugin::Commands::MARK_MODEL_AS_NO_LONGER_NEEDED>(model);
        return;
    }
    plugin::Command<plugin::Commands::GIVE_WEAPON_TO_CHAR>(hplayer, weaponType, ammo);
    plugin::Command<plugin::Commands::MARK_MODEL_AS_NO_LONGER_NEEDED>(model);
    plugin::Command<plugin::Commands::SET_CURRENT_PLAYER_WEAPON>(0, weaponType);
}

void DropWeapon(CPlayerPed* player) {
    if (!player) return;
    const int hplayer = CPools::GetPedRef(player);
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    plugin::Command<plugin::Commands::GET_OFFSET_FROM_CHAR_IN_WORLD_COORDS>(hplayer, 0.0f, 3.0f, 0.0f, &x, &y, &z);

    const eWeaponType weaponType = player->m_aWeapons[player->m_nSelectedWepSlot].m_eWeaponType;
    if (weaponType == WEAPONTYPE_UNARMED) return;

    const int model = GetWeaponModel(weaponType);
    int pickup = 0;
    plugin::Command<plugin::Commands::CREATE_PICKUP_WITH_AMMO>(model, 3, 999, x, y, z, &pickup);
    ClearPlayerWeapon(player, weaponType);
}

void DropCurrentWeapon(CPlayerPed* player) {
    if (!player) return;
    ClearPlayerWeapon(player, player->m_aWeapons[player->m_nCurrentWeapon].m_eWeaponType);
}

void ClearWeapons(CPlayerPed* player) {
    if (player) player->ClearWeapons();
}

int RemoveTrackedPickups() {
    int removed = 0;
    for (int handle : g_trackedPickupHandles) {
        if (handle < 0) {
            continue;
        }
        CPickups::RemovePickUp(handle);
        ++removed;
    }
    g_trackedPickupHandles.clear();
    return removed;
}

void ProcessInfiniteAmmo(CPlayerPed* player, bool enable) {
    if (!enable || !player) return;
    for (int i = 0; i < 13; i++) {
        player->m_aWeapons[i].m_nAmmoTotal = 9999;
    }
}

void SetFastReload(CPlayerPed* player, bool enable) {
    if (!player) return;
    plugin::Command<plugin::Commands::SET_PLAYER_FAST_RELOAD>(CPools::GetPedRef(player), enable);
}

void ProcessWeaponTweaks(CPlayerPed* player, bool hugeDamage, bool longRange, bool rapidFire, bool dualWield, bool moveAim, bool moveFire, bool noSpread, bool customFireRate, float fireRate) {
    (void)rapidFire;
    (void)dualWield;
    (void)moveAim;
    (void)moveFire;

    auto scaleAnimLoop = [](float& start, float& end, float& fire, float rate) {
        if (rate < 0.1f) {
            rate = 0.1f;
        }
        if (rate > 20.0f) {
            rate = 20.0f;
        }
        if (std::fabs(rate - 1.0f) < 0.001f || end <= start) {
            return;
        }
        const float oldStart = start;
        const float oldEnd = end;
        const float oldFire = fire;
        end = oldStart + (oldEnd - oldStart) / rate;
        fire = oldStart + (oldFire - oldStart) / rate;
        if (fire < start) {
            fire = start;
        }
        if (fire > end) {
            fire = end;
        }
    };

    const float effectiveRate = (customFireRate && fireRate > 0.1f) ? fireRate : 1.0f;
    const bool fireRateActive = customFireRate && std::fabs(effectiveRate - 1.0f) >= 0.001f;
    const bool any = hugeDamage || longRange || noSpread || fireRateActive;
    static bool s_wasAny = false;
    static int s_lastType = -1;
    static unsigned char s_lastMask = 0;
    static int s_lastRateQ = 100;

    const unsigned char mask =
        static_cast<unsigned char>((hugeDamage ? 1 : 0) |
                                   (longRange ? 2 : 0) |
                                   (noSpread ? 4 : 0) |
                                   (fireRateActive ? 8 : 0));
    const int rateQ = fireRateActive ? static_cast<int>(effectiveRate * 100.0f + 0.5f) : 100;

    if (!any) {
        if (s_wasAny) {
            CWeaponInfo::LoadWeaponData();
            s_wasAny = false;
            s_lastType = -1;
            s_lastMask = 0;
            s_lastRateQ = 100;
        }
        return;
    }
    if (!player) {
        return;
    }

    CWeapon& weapon = player->m_aWeapons[player->m_nCurrentWeapon];
    const int type = static_cast<int>(weapon.m_eWeaponType);
    if (s_wasAny && !g_weaponTweaksNeedReapply && s_lastType == type && s_lastMask == mask && s_lastRateQ == rateQ) {
        return;
    }

    if (s_wasAny || g_weaponTweaksNeedReapply) {
        CWeaponInfo::LoadWeaponData();
    }

    CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(weapon.m_eWeaponType);
    if (!info) {
        return;
    }

    if (hugeDamage) {
        info->m_nDamage = 1000;
    }
    if (longRange) {
        info->m_fRange = 1000.0f;
    }
    if (noSpread) {
        info->m_fSpread = 0.0f;
    }
    if (fireRateActive) {
        if (info->m_nFiringRate > 0) {
            const float scaled = static_cast<float>(info->m_nFiringRate) / effectiveRate;
            info->m_nFiringRate = scaled < 1.0f ? 1u : static_cast<unsigned int>(scaled);
        }
        // III 只有一组主循环 + 第二开火帧
        scaleAnimLoop(info->m_fAnimLoopStart, info->m_fAnimLoopEnd, info->m_fAnimFrameFire, effectiveRate);
        if (info->m_fAnim2FrameFire > 0.0f) {
            info->m_fAnim2FrameFire = info->m_fAnim2FrameFire / effectiveRate;
        }
    }

    s_wasAny = true;
    s_lastType = type;
    s_lastMask = mask;
    s_lastRateQ = rateQ;
    g_weaponTweaksNeedReapply = false;
}

void ResetWeaponStats() {
    CWeaponInfo::LoadWeaponData();
    g_weaponTweaksNeedReapply = true;
}

void SetTime(int hour, int minute) {
    CClock::SetGameClock(hour, minute);
}

void GetTime(int& hour, int& minute) {
    hour = CClock::ms_nGameClockHours;
    minute = CClock::ms_nGameClockMinutes;
}

void SyncTimeWithSystemClock() {
    const std::time_t nowTime = std::time(nullptr);
    const std::tm* now = std::localtime(&nowTime);
    if (!now) return;

    CClock::SetGameClock(now->tm_hour, now->tm_min);
}

void SetGameSpeed(float speed) {
    CTimer::ms_fTimeScale = speed;
}

float GetGameSpeed() {
    return CTimer::ms_fTimeScale;
}

int GetFpsLimit() {
    return RsGlobal.maxFPS;
}

void SetFpsLimit(int limit) {
    RsGlobal.maxFPS = limit;
}

void SetDisableReplay(bool enable) {
    PatchByteIfExpected("III", 0x593170, 0x80, 0xC3, enable);
}

void SetDisableCheats(bool enable) {
    static const unsigned char originalFirst[] = { 0x88, 0xD8, 0x89, 0xF1, 0x50 };
    static const unsigned char patchedFirst[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
    static const unsigned char originalSecond[] = { 0xE8, 0x84, 0xE2, 0xF0, 0xFF };
    static const unsigned char patchedSecond[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };

    const bool firstOk = PatchBytesIfExpected("III 禁用作弊码", 0x5841B8, originalFirst, patchedFirst, sizeof(originalFirst), enable);
    const bool secondOk = PatchBytesIfExpected("III 禁用作弊码", 0x5841C7, originalSecond, patchedSecond, sizeof(originalSecond), enable);
    if (!firstOk || !secondOk) {
        Log::Warn("III 禁用作弊码补丁未完整应用，当前游戏版本可能不匹配");
    }
}

void SetForbiddenAreaWanted(bool enable) {
    Log::Warn(std::string("III 不支持禁止通缉区域开关：") + (enable ? "开启" : "关闭"));
}

void SetFreePayNSpray(bool enable) {
    Log::Warn(std::string("III 不支持免费喷漆店开关：") + (enable ? "开启" : "关闭"));
}

void SetFasterClock(bool enable) {
    plugin::patch::Set<bool>(0x95CDBB, enable, false);
}

void ProcessSolidWater(CPlayerPed* player, bool enable) {
}

void SetNoWaterPhysics(bool enable) {
}

void SetFreezeTime(bool enable) {
    Log::Info(std::string("III 冻结时间使用控制器锁定：") + (enable ? "开启" : "关闭"));
}

int GetDaysPassed() {
    return *reinterpret_cast<int*>(0x8F2BB8);
}

void SetDaysPassed(int days) {
    *reinterpret_cast<int*>(0x8F2BB8) = days;
}

float GetGravity() {
    return *reinterpret_cast<float*>(0x5F68D4);
}

void SetGravity(float gravity) {
    *reinterpret_cast<float*>(0x5F68D4) = gravity;
}

namespace {
    int lastXMenuPickupHandle = -1;
    CVector lastXMenuPickupPosition;

    unsigned char NormalizePickupType(unsigned int type) {
        if (type > PICKUP_NUMOFTYPES) {
            return PICKUP_ONCE;
        }
        return static_cast<unsigned char>(type);
    }

    int CreateXMenuPickup(const GameLogic::PickupOptions& options, const CVector& pos) {
        const int model = static_cast<int>(options.modelId);
        if (model <= 0) {
            return -1;
        }

        CStreaming::RequestModel(model, PRIORITY_REQUEST);
        CStreaming::LoadAllRequestedModels(false);
        const int handle = CPickups::GenerateNewOne(
            pos,
            static_cast<unsigned int>(model),
            NormalizePickupType(options.type),
            options.quantity
        );
        plugin::Command<plugin::Commands::MARK_MODEL_AS_NO_LONGER_NEEDED>(model);
        if (handle >= 0) {
            g_trackedPickupHandles.push_back(handle);
        }
        return handle;
    }
}

int SpawnPickupNearPlayer(const PickupOptions& options) {
    CPlayerPed* player = FindPlayerPed();
    if (!player) {
        return -1;
    }

    const int hplayer = CPools::GetPedRef(player);
    CVector pos;
    plugin::Command<plugin::Commands::GET_OFFSET_FROM_CHAR_IN_WORLD_COORDS>(hplayer, 0.0f, 2.0f, 0.0f, &pos.x, &pos.y, &pos.z);
    pos.z += 0.2f;

    const int handle = CreateXMenuPickup(options, pos);
    if (handle >= 0) {
        lastXMenuPickupHandle = handle;
        lastXMenuPickupPosition = pos;
    }
    return handle;
}

bool UpdateLastPickup(const PickupOptions& options) {
    if (lastXMenuPickupHandle < 0) {
        return false;
    }

    CPickups::RemovePickUp(lastXMenuPickupHandle);
    for (auto it = g_trackedPickupHandles.begin(); it != g_trackedPickupHandles.end(); ++it) {
        if (*it == lastXMenuPickupHandle) {
            g_trackedPickupHandles.erase(it);
            break;
        }
    }
    const int handle = CreateXMenuPickup(options, lastXMenuPickupPosition);
    if (handle < 0) {
        lastXMenuPickupHandle = -1;
        return false;
    }

    lastXMenuPickupHandle = handle;
    return true;
}

bool RemoveLastPickup() {
    if (lastXMenuPickupHandle < 0) {
        return false;
    }

    CPickups::RemovePickUp(lastXMenuPickupHandle);
    for (auto it = g_trackedPickupHandles.begin(); it != g_trackedPickupHandles.end(); ++it) {
        if (*it == lastXMenuPickupHandle) {
            g_trackedPickupHandles.erase(it);
            break;
        }
    }
    lastXMenuPickupHandle = -1;
    return true;
}

} // namespace GameLogic
