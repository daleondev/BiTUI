#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace bitui
{
    struct Color
    {
        uint8_t r{};
        uint8_t g{};
        uint8_t b{};

        static constexpr auto rgb(uint8_t r, uint8_t g, uint8_t b) -> Color
        {
            return Color{ .r = r, .g = g, .b = b };
        }

        static constexpr auto unpack(uint32_t packed) -> Color
        {
            return Color{
                .r = static_cast<uint8_t>((packed >> 16) & 0xFF),
                .g = static_cast<uint8_t>((packed >> 8) & 0xFF),
                .b = static_cast<uint8_t>(packed & 0xFF),
            };
        }

        constexpr auto pack() -> uint32_t
        {
            return (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) |
                   static_cast<uint32_t>(b);
        }

        constexpr auto blend(const Color& other, float alpha) const -> Color
        {
            auto safe_alpha{ std::clamp(alpha, 0.0f, 1.0f) };
            return Color{
                .r = static_cast<uint8_t>(std::lerp(r, other.r, safe_alpha)),
                .g = static_cast<uint8_t>(std::lerp(g, other.g, safe_alpha)),
                .b = static_cast<uint8_t>(std::lerp(b, other.b, safe_alpha)),
            };
        }

        constexpr auto scale(float factor) const -> Color
        {
            return Color{
                .r = static_cast<uint8_t>(std::clamp(std::round(r * factor), 0.0f, 255.0f)),
                .g = static_cast<uint8_t>(std::clamp(std::round(g * factor), 0.0f, 255.0f)),
                .b = static_cast<uint8_t>(std::clamp(std::round(b * factor), 0.0f, 255.0f)),
            };
        }

        constexpr auto operator<=>(const Color&) const = default;
    };

}