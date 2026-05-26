#include "Adapter.h"
#include "utils/Log.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <windows.h>

namespace {
    struct RuntimeLayout {
        GameRuntime::Target target = GameRuntime::Target::Unknown;
        const char* name = "Unknown";
        std::uintptr_t findPlayerPed = 0;
        std::uintptr_t players = 0;
        std::uintptr_t playerInFocus = 0;
        std::size_t playerInfoSize = 0;
        std::size_t moneyOffset = 0;
        std::size_t displayMoneyOffset = 0;
        std::size_t healthOffset = 0;
        std::size_t armourOffset = 0;
        std::size_t wantedPtrOffset = 0;
        std::size_t wantedLevelOffset = 0;
        std::size_t proofByteOffset = 0;
        bool hasProofByte = false;
    };

    bool IsReadable(const void* address, std::size_t size) {
        if (!address || size == 0) {
            return false;
        }

        MEMORY_BASIC_INFORMATION memoryInfo{};
        if (VirtualQuery(address, &memoryInfo, sizeof(memoryInfo)) == 0 || memoryInfo.State != MEM_COMMIT ||
            (memoryInfo.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0) {
            return false;
        }

        const auto start = reinterpret_cast<std::uintptr_t>(address);
        const auto regionStart = reinterpret_cast<std::uintptr_t>(memoryInfo.BaseAddress);
        return start + size <= regionStart + memoryInfo.RegionSize;
    }

    template <typename T>
    bool ReadValue(std::uintptr_t address, T& value) {
        if (!IsReadable(reinterpret_cast<const void*>(address), sizeof(T))) {
            value = T{};
            return false;
        }
        std::memcpy(&value, reinterpret_cast<const void*>(address), sizeof(T));
        return true;
    }

    template <typename T>
    bool WriteValue(std::uintptr_t address, const T& value) {
        if (!IsReadable(reinterpret_cast<const void*>(address), sizeof(T))) {
            return false;
        }

        DWORD oldProtect = 0;
        auto* target = reinterpret_cast<void*>(address);
        if (!VirtualProtect(target, sizeof(T), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            return false;
        }

        std::memcpy(target, &value, sizeof(T));
        DWORD unusedProtect = 0;
        VirtualProtect(target, sizeof(T), oldProtect, &unusedProtect);
        return true;
    }

    int ClampWantedLevel(int level) {
        if (level < 0) {
            return 0;
        }
        if (level > 6) {
            return 6;
        }
        return level;
    }

    RuntimeLayout LayoutFor(GameRuntime::Target target) {
        RuntimeLayout layout;
        layout.target = target;

        switch (target) {
        case GameRuntime::Target::SA:
            layout.name = "GTA San Andreas adapter";
            layout.findPlayerPed = 0x56E210;
            layout.players = 0xB7CD98;
            layout.playerInFocus = 0xB7CD74;
            layout.playerInfoSize = 0x190;
            layout.moneyOffset = 0xB8;
            layout.displayMoneyOffset = 0xBC;
            layout.healthOffset = 0x540;
            layout.armourOffset = 0x548;
            layout.wantedPtrOffset = 0;
            layout.wantedLevelOffset = 0x2C;
            layout.proofByteOffset = 0x42;
            layout.hasProofByte = true;
            break;
        case GameRuntime::Target::VC:
            layout.name = "GTA Vice City adapter";
            layout.findPlayerPed = 0x4BC120;
            layout.players = 0x94AD28;
            layout.playerInFocus = 0xA10AFB;
            layout.playerInfoSize = 0x170;
            layout.moneyOffset = 0xA0;
            layout.displayMoneyOffset = 0xA4;
            layout.healthOffset = 0x354;
            layout.armourOffset = 0x358;
            layout.wantedPtrOffset = 0x5F4;
            layout.wantedLevelOffset = 0x20;
            break;
        case GameRuntime::Target::III:
            layout.name = "GTA III adapter";
            layout.findPlayerPed = 0x4A1150;
            layout.players = 0x9412F0;
            layout.playerInFocus = 0x95CD61;
            layout.playerInfoSize = 0x13C;
            layout.moneyOffset = 0xAC;
            layout.displayMoneyOffset = 0xB0;
            layout.healthOffset = 0x2C0;
            layout.armourOffset = 0x2C4;
            layout.wantedPtrOffset = 0x53C;
            layout.wantedLevelOffset = 0x18;
            break;
        case GameRuntime::Target::Unknown:
        default:
            break;
        }

        return layout;
    }

    std::uintptr_t CallFindPlayerPed(const RuntimeLayout& layout) {
        if (!layout.findPlayerPed || !IsReadable(reinterpret_cast<const void*>(layout.findPlayerPed), 1)) {
            return 0;
        }

        if (layout.target == GameRuntime::Target::SA) {
            using FindPlayerPedSa = void*(__cdecl*)(int);
            return reinterpret_cast<std::uintptr_t>(reinterpret_cast<FindPlayerPedSa>(layout.findPlayerPed)(-1));
        }

        using FindPlayerPed = void*(__cdecl*)();
        return reinterpret_cast<std::uintptr_t>(reinterpret_cast<FindPlayerPed>(layout.findPlayerPed)());
    }

    std::uintptr_t PlayerInfoAddress(const RuntimeLayout& layout) {
        if (!layout.players || !layout.playerInFocus || !layout.playerInfoSize) {
            return 0;
        }

        unsigned char focus = 0;
        if (!ReadValue(layout.playerInFocus, focus)) {
            return 0;
        }

        return layout.players + static_cast<std::uintptr_t>(focus) * layout.playerInfoSize;
    }

    std::uintptr_t WantedAddress(const RuntimeLayout& layout, std::uintptr_t player) {
        if (!player) {
            return 0;
        }

        if (layout.target == GameRuntime::Target::SA) {
            std::uintptr_t playerData = 0;
            if (!ReadValue(player + 0x480, playerData) || !playerData) {
                return 0;
            }

            std::uintptr_t wanted = 0;
            if (!ReadValue(playerData, wanted)) {
                return 0;
            }
            return wanted;
        }

        std::uintptr_t wanted = 0;
        if (!layout.wantedPtrOffset || !ReadValue(player + layout.wantedPtrOffset, wanted)) {
            return 0;
        }
        return wanted;
    }

    GameTypes::ProofState ReadProofByte(unsigned char value) {
        GameTypes::ProofState state;
        state.bullet = (value & (1u << 2)) != 0;
        state.fire = (value & (1u << 3)) != 0;
        state.collision = (value & (1u << 4)) != 0;
        state.melee = (value & (1u << 5)) != 0;
        state.explosion = (value & (1u << 7)) != 0;
        return state;
    }

    unsigned char WriteProofByte(unsigned char value, const GameTypes::ProofState& state) {
        auto setBit = [&value](int bit, bool enabled) {
            const unsigned char mask = static_cast<unsigned char>(1u << bit);
            value = enabled ? static_cast<unsigned char>(value | mask) : static_cast<unsigned char>(value & ~mask);
        };

        setBit(2, state.bullet);
        setBit(3, state.fire);
        setBit(4, state.collision);
        setBit(5, state.melee);
        setBit(7, state.explosion);
        return value;
    }

    class RuntimeGameAdapter final : public GameAdapter::IGameAdapter {
    public:
        explicit RuntimeGameAdapter(RuntimeLayout layout) : layout_(layout) {}

        const char* Name() const override { return layout_.name; }
        GameRuntime::Target Target() const override { return layout_.target; }

        GameAdapter::Capabilities GetCapabilities() const override {
            GameAdapter::Capabilities capabilities;
            capabilities.player = layout_.target != GameRuntime::Target::Unknown;
            return capabilities;
        }

        bool Attach() override {
            Log::Info(std::string("已绑定运行时适配器: ") + layout_.name);
            return layout_.target != GameRuntime::Target::Unknown;
        }

        void Detach() override {
            Log::Info(std::string("运行时适配器已解绑: ") + layout_.name);
        }

        void OnGameInit() override {}
        void OnProcess() override {}

        GameTypes::PlayerState GetPlayerState() override {
            GameTypes::PlayerState state;
            const std::uintptr_t player = CallFindPlayerPed(layout_);
            const std::uintptr_t playerInfo = PlayerInfoAddress(layout_);
            state.available = player != 0;

            if (player) {
                ReadValue(player + layout_.healthOffset, state.health);
                ReadValue(player + layout_.armourOffset, state.armour);
                state.dead = state.health <= 0.0f;
                state.proofs = GetPlayerProofState();
            }

            if (playerInfo) {
                ReadValue(playerInfo + layout_.moneyOffset, state.money);
            }

            const std::uintptr_t wanted = WantedAddress(layout_, player);
            if (wanted) {
                int wantedLevel = 0;
                if (ReadValue(wanted + layout_.wantedLevelOffset, wantedLevel)) {
                    state.wantedLevel = ClampWantedLevel(wantedLevel);
                }
            }

            return state;
        }

        GameTypes::ProofState GetPlayerProofState() override {
            const std::uintptr_t player = CallFindPlayerPed(layout_);
            if (!player || !layout_.hasProofByte) {
                return GameTypes::ProofState{};
            }

            unsigned char value = 0;
            if (!ReadValue(player + layout_.proofByteOffset, value)) {
                return GameTypes::ProofState{};
            }
            return ReadProofByte(value);
        }

        void SetPlayerProofState(const GameTypes::ProofState& state) override {
            const std::uintptr_t player = CallFindPlayerPed(layout_);
            if (!player || !layout_.hasProofByte) {
                return;
            }

            unsigned char value = 0;
            if (ReadValue(player + layout_.proofByteOffset, value)) {
                value = WriteProofByte(value, state);
                WriteValue(player + layout_.proofByteOffset, value);
            }
        }

        void SetPlayerInvincible(bool enable) override {
            GameTypes::ProofState state;
            state.bullet = enable;
            state.collision = enable;
            state.explosion = enable;
            state.fire = enable;
            state.melee = enable;
            state.nonPlayer = enable;
            SetPlayerProofState(state);
        }

        void SetPlayerHealth(float value) override {
            const std::uintptr_t player = CallFindPlayerPed(layout_);
            if (player) {
                WriteValue(player + layout_.healthOffset, value);
            }
        }

        void SetPlayerArmour(float value) override {
            const std::uintptr_t player = CallFindPlayerPed(layout_);
            if (player) {
                WriteValue(player + layout_.armourOffset, value);
            }
        }

        void SetWantedLevel(int level) override {
            const std::uintptr_t player = CallFindPlayerPed(layout_);
            const std::uintptr_t wanted = WantedAddress(layout_, player);
            if (!wanted) {
                return;
            }

            const int clamped = ClampWantedLevel(level);
            WriteValue(wanted + layout_.wantedLevelOffset, clamped);
        }

        void GiveMoney(int amount) override {
            const GameTypes::PlayerState state = GetPlayerState();
            SetMoney(state.money + amount);
        }

        void SetMoney(int amount) override {
            const std::uintptr_t playerInfo = PlayerInfoAddress(layout_);
            if (!playerInfo) {
                return;
            }

            WriteValue(playerInfo + layout_.moneyOffset, amount);
            WriteValue(playerInfo + layout_.displayMoneyOffset, amount);
        }

        void KillPlayer() override {
            SetPlayerHealth(0.0f);
        }

    private:
        RuntimeLayout layout_;
    };

    RuntimeGameAdapter saAdapter(LayoutFor(GameRuntime::Target::SA));
    RuntimeGameAdapter vcAdapter(LayoutFor(GameRuntime::Target::VC));
    RuntimeGameAdapter iiiAdapter(LayoutFor(GameRuntime::Target::III));

    GameAdapter::IGameAdapter* activeAdapter = nullptr;
    GameAdapter::Capabilities activeCapabilities;

    GameAdapter::IGameAdapter* AdapterFor(GameRuntime::Target target) {
        switch (target) {
        case GameRuntime::Target::SA: return &saAdapter;
        case GameRuntime::Target::VC: return &vcAdapter;
        case GameRuntime::Target::III: return &iiiAdapter;
        case GameRuntime::Target::Unknown:
        default: return nullptr;
        }
    }
}

namespace GameAdapter {
    bool Bind(GameRuntime::Target target) {
        Detach();

        activeAdapter = AdapterFor(target);
        if (!activeAdapter) {
            Log::Error("无法绑定运行时适配器：未知游戏");
            return false;
        }

        if (!activeAdapter->Attach()) {
            activeAdapter = nullptr;
            activeCapabilities = Capabilities{};
            return false;
        }

        activeCapabilities = activeAdapter->GetCapabilities();
        return true;
    }

    IGameAdapter* Active() {
        return activeAdapter;
    }

    const Capabilities& ActiveCapabilities() {
        return activeCapabilities;
    }

    void Detach() {
        if (activeAdapter) {
            activeAdapter->Detach();
        }
        activeAdapter = nullptr;
        activeCapabilities = Capabilities{};
    }
}