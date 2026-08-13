#pragma once

#include <functional>
#include <string>
#include <vector>
#include <cstdint>

namespace XBase::Hotkey {

enum class Key : uint16_t {
    None = 0,
    LButton = 0x01, RButton = 0x02, MButton = 0x04,
    Back = 0x08, Tab = 0x09,
    Clear = 0x0C, Return = 0x0D,
    Shift = 0x10, Control = 0x11, Menu = 0x12, Alt = 0x12,
    Pause = 0x13, Capital = 0x14, Escape = 0x1B,
    Space = 0x20, Prior = 0x21, Next = 0x22,
    End = 0x23, Home = 0x24, Left = 0x25, Up = 0x26, Right = 0x27, Down = 0x28,
    Select = 0x29, Print = 0x2A, Execute = 0x2B, Snapshot = 0x2C, Insert = 0x2D, Delete = 0x2E, Help = 0x2F,
    D0 = 0x30, D1 = 0x31, D2 = 0x32, D3 = 0x33, D4 = 0x34, D5 = 0x35, D6 = 0x36, D7 = 0x37, D8 = 0x38, D9 = 0x39,
    A = 0x41, B = 0x42, C = 0x43, D = 0x44, E = 0x45, F = 0x46, G = 0x47, H = 0x48, I = 0x49, J = 0x4A, K = 0x4B, L = 0x4C, M = 0x4D, N = 0x4E, O = 0x4F, P = 0x50, Q = 0x51, R = 0x52, S = 0x53, T = 0x54, U = 0x55, V = 0x56, W = 0x57, X = 0x58, Y = 0x59, Z = 0x5A,
    LWin = 0x5B, RWin = 0x5C, Apps = 0x5D,
    Sleep = 0x5F,
    Numpad0 = 0x60, Numpad1 = 0x61, Numpad2 = 0x62, Numpad3 = 0x63, Numpad4 = 0x64, Numpad5 = 0x65, Numpad6 = 0x66, Numpad7 = 0x67, Numpad8 = 0x68, Numpad9 = 0x69,
    Multiply = 0x6A, Add = 0x6B, Separator = 0x6C, Subtract = 0x6D, Decimal = 0x6E, Divide = 0x6F,
    F1 = 0x70, F2 = 0x71, F3 = 0x72, F4 = 0x73, F5 = 0x74, F6 = 0x75, F7 = 0x76, F8 = 0x77, F9 = 0x78, F10 = 0x79, F11 = 0x7A, F12 = 0x7B,
    F13 = 0x7C, F14 = 0x7D, F15 = 0x7E, F16 = 0x7F, F17 = 0x80, F18 = 0x81, F19 = 0x82, F20 = 0x83, F21 = 0x84, F22 = 0x85, F23 = 0x86, F24 = 0x87,
    NumLock = 0x90, Scroll = 0x91,
    LShift = 0xA0, RShift = 0xA1, LControl = 0xA2, RControl = 0xA3, LMenu = 0xA4, RMenu = 0xA5,
    BrowserBack = 0xA6, BrowserForward = 0xA7, BrowserRefresh = 0xA8, BrowserStop = 0xA9, BrowserSearch = 0xAA, BrowserFavorites = 0xAB, BrowserHome = 0xAC,
    VolumeMute = 0xAD, VolumeDown = 0xAE, VolumeUp = 0xAF, MediaNext = 0xB0, MediaPrev = 0xB1, MediaStop = 0xB2, MediaPlayPause = 0xB3,
    Oem1 = 0xBA, OemPlus = 0xBB, OemComma = 0xBC, OemMinus = 0xBD, OemPeriod = 0xBE, Oem2 = 0xBF, Oem3 = 0xC0, Oem4 = 0xDB, Oem5 = 0xDC, Oem6 = 0xDD, Oem7 = 0xDE, Oem8 = 0xDF,
    Oem102 = 0xE2, ProcessKey = 0xE5, Packet = 0xE7, Attn = 0xF6, CrSel = 0xF7, ExSel = 0xF8, EraseEof = 0xF9, Play = 0xFA, Zoom = 0xFB, NoName = 0xFC, Pa1 = 0xFD, OemClear = 0xFE,
};

struct Modifiers {
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
    bool win = false;

    bool operator==(const Modifiers& other) const {
        return ctrl == other.ctrl && shift == other.shift && alt == other.alt && win == other.win;
    }
};

enum class Mode {
    Toggle,   // 按下切换状态
    Hold,     // 按住生效，松开失效
    Once      // 仅触发一次
};

struct Binding {
    std::string id;
    Key key = Key::None;
    Modifiers mods;
    Mode mode = Mode::Toggle;
    std::function<void(bool)> onChange; // true=激活/按下, false=释放/关闭
    bool state = false;
    bool prevPressed = false;
};

void Init();
void Process();

bool Register(const std::string& id, Key key, Modifiers mods, Mode mode, std::function<void(bool)> callback);
bool Unregister(const std::string& id);
Binding* Get(const std::string& id);
std::vector<Binding*> GetAll();

bool IsPressed(Key key);
bool IsKeyDown(Key key);
bool IsToggled(Key key);

void SaveBindings(const std::string& filePath = "");
void LoadBindings(const std::string& filePath = "");

std::string KeyToString(Key key);
Key StringToKey(const std::string& str);

} // namespace XBase::Hotkey
