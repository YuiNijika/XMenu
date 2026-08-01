#pragma once

#include <cstdint>

namespace XBase {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    constexpr Vec2() = default;
    constexpr Vec2(float xValue, float yValue) : x(xValue), y(yValue) {}
};

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Rect {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
};

struct Color {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;
};

struct ColorF {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    constexpr ColorF() = default;
    constexpr ColorF(float red, float green, float blue, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}
};

struct FontId {
    std::uint32_t value = 0;

    constexpr explicit operator bool() const {
        return value != 0;
    }
};

struct TextureId {
    std::uint64_t value = 0;

    constexpr explicit operator bool() const {
        return value != 0;
    }
};

struct EntityId {
    std::uint32_t value = 0;

    constexpr explicit operator bool() const {
        return value != 0;
    }
};

struct PedId {
    std::uint32_t value = 0;

    constexpr explicit operator bool() const {
        return value != 0;
    }
};

struct VehicleId {
    std::uint32_t value = 0;

    constexpr explicit operator bool() const {
        return value != 0;
    }
};

} // namespace XBase