#pragma once

#include <cstdint>
#include <string>

namespace XBase::Input {

enum class Key : std::uint16_t {
    None,
    Backspace,
    Tab,
    Enter,
    Escape,
    Space,
    PageUp,
    PageDown,
    End,
    Home,
    Left,
    Up,
    Right,
    Down,
    Insert,
    Delete,
    Num0,
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,
    Multiply,
    Add,
    Subtract,
    Decimal,
    Divide,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    Digit0,
    Digit1,
    Digit2,
    Digit3,
    Digit4,
    Digit5,
    Digit6,
    Digit7,
    Digit8,
    Digit9,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
};

enum class Modifier : std::uint8_t {
    None = 0,
    Ctrl = 1u << 0,
    Alt = 1u << 1,
    Shift = 1u << 2,
};

using Modifiers = std::uint8_t;

constexpr Modifiers ModifierBit(Modifier modifier) {
    return static_cast<Modifiers>(modifier);
}

struct Hotkey {
    Key key = Key::None;
    Modifiers modifiers = 0;
};

bool IsDown(Key key);
bool IsModifierDown(Modifier modifier);
bool WasPressed(Key key);
bool IsDown(const Hotkey& hotkey);
bool WasPressed(const Hotkey& hotkey);
bool CapturePressedHotkey(Hotkey& hotkey, bool allowClear = true);
const char* GetKeyName(Key key);
std::string FormatHotkey(const Hotkey& hotkey);
bool ParseHotkey(const std::string& value, Hotkey& hotkey);

} // namespace XBase::Input