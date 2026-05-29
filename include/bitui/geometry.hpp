#pragma once

#include <algorithm>
#include <limits>

namespace bitui
{
    namespace detail
    {
        constexpr auto add_saturated(uint16_t value, uint16_t increment) -> uint16_t
        {
            constexpr auto max_value{ std::numeric_limits<uint16_t>::max() };
            if (increment > static_cast<uint16_t>(max_value - value)) {
                return max_value;
            }
            return static_cast<uint16_t>(value + increment);
        }

        constexpr auto distance_or_zero(uint16_t first, uint16_t last) -> uint16_t
        {
            if (last <= first) {
                return 0;
            }
            return static_cast<uint16_t>(last - first);
        }
    }

    struct Size
    {
        uint16_t width{};
        uint16_t height{};

        constexpr auto operator<=>(const Size&) const = default;
    };

    struct Rect
    {
        uint16_t x{};
        uint16_t y{};
        Size size{};

        constexpr auto right() const -> uint16_t { return detail::add_saturated(x, size.width); }

        constexpr auto bottom() const -> uint16_t { return detail::add_saturated(y, size.height); }

        constexpr auto empty() const -> bool
        {
            return size.width == 0 || size.height == 0 || right() <= x || bottom() <= y;
        }

        constexpr auto contains(uint16_t px, uint16_t py) const -> bool
        {
            return px >= x && py >= y && px < right() && py < bottom();
        }

        constexpr auto intersect(const Rect& other) const -> Rect
        {
            auto x1{ std::max(x, other.x) };
            auto y1{ std::max(y, other.y) };
            auto x2{ std::min(right(), other.right()) };
            auto y2{ std::min(bottom(), other.bottom()) };
            return Rect{ .x = x1,
                         .y = y1,
                         .size = bitui::Size{ .width = detail::distance_or_zero(x1, x2),
                                              .height = detail::distance_or_zero(y1, y2) } };
        }

        constexpr auto operator<=>(const Rect&) const = default;
    };
}