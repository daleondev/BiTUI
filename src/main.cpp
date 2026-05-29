#include <algorithm>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace bitui::detail
{
    [[maybe_unused]]
    inline const auto CONFIGURED_TERMINAL_HANDLE{ [] {
        const auto fail{ [] {
            std::fputs("Error: Failed to configure terminal\n", stderr);
            std::fflush(stderr);
            std::exit(EXIT_FAILURE);
        } };

        HANDLE hOut{ GetStdHandle(STD_OUTPUT_HANDLE) };
        if (hOut == INVALID_HANDLE_VALUE || !hOut) {
            fail();
        }

        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

        DWORD dwMode{};
        if (GetConsoleMode(hOut, &dwMode) == FALSE) {
            fail();
        }

        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (SetConsoleMode(hOut, dwMode) == FALSE) {
            fail();
        }

        return hOut;
    }() };
}
#endif

namespace
{
    constexpr auto uint16_max() -> uint16_t { return std::numeric_limits<uint16_t>::max(); }

    constexpr auto add_saturated(uint16_t value, uint16_t increment) -> uint16_t
    {
        if (increment > static_cast<uint16_t>(uint16_max() - value)) {
            return uint16_max();
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

struct Color
{
    uint8_t r{};
    uint8_t g{};
    uint8_t b{};
};

constexpr auto rgb(uint8_t r, uint8_t g, uint8_t b) -> Color { return Color{ .r = r, .g = g, .b = b }; }

namespace
{
    constexpr Color DEFAULT_FOREGROUND_COLOR{ .r = 226, .g = 232, .b = 240 };
    constexpr Color DEFAULT_BACKGROUND_COLOR{ .r = 10, .g = 14, .b = 22 };
}

struct Style
{
    Color fg{ DEFAULT_FOREGROUND_COLOR };
    Color bg{ DEFAULT_BACKGROUND_COLOR };
    bool bold{};
    bool dim{};
    bool italic{};
    bool underline{};
};

struct Cell
{
    std::string grapheme{ " " };
    Style style{};
};

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

    constexpr auto right() const -> uint16_t { return add_saturated(x, size.width); }
    constexpr auto bottom() const -> uint16_t { return add_saturated(y, size.height); }
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
                     .size = Size{ .width = distance_or_zero(x1, x2), .height = distance_or_zero(y1, y2) } };
    }

    constexpr auto operator<=>(const Rect&) const = default;
};

enum class BorderStyle : uint8_t
{
    Single,
    Rounded,
    Heavy,
};

namespace
{
    constexpr unsigned char UTF8_ONE_BYTE_MASK{ 0b1000'0000U };
    constexpr unsigned char UTF8_TWO_BYTE_MASK{ 0b1110'0000U };
    constexpr unsigned char UTF8_TWO_BYTE_PREFIX{ 0b1100'0000U };
    constexpr unsigned char UTF8_THREE_BYTE_MASK{ 0b1111'0000U };
    constexpr unsigned char UTF8_THREE_BYTE_PREFIX{ 0b1110'0000U };
    constexpr unsigned char UTF8_FOUR_BYTE_MASK{ 0b1111'1000U };
    constexpr unsigned char UTF8_FOUR_BYTE_PREFIX{ 0b1111'0000U };

    constexpr auto utf8_glyph_length(unsigned char lead) -> size_t
    {
        if ((lead & UTF8_ONE_BYTE_MASK) == 0U) {
            return 1;
        }
        if ((lead & UTF8_TWO_BYTE_MASK) == UTF8_TWO_BYTE_PREFIX) {
            return 2;
        }
        if ((lead & UTF8_THREE_BYTE_MASK) == UTF8_THREE_BYTE_PREFIX) {
            return 3;
        }
        if ((lead & UTF8_FOUR_BYTE_MASK) == UTF8_FOUR_BYTE_PREFIX) {
            return 4;
        }
        return 1;
    }

    struct BorderGlyphs
    {
        std::string_view top_left;
        std::string_view top_right;
        std::string_view bottom_left;
        std::string_view bottom_right;
        std::string_view horizontal;
        std::string_view vertical;
    };

    constexpr auto glyphs_for(BorderStyle border) -> BorderGlyphs
    {
        switch (border) {
            case BorderStyle::Heavy:
                return BorderGlyphs{ .top_left = "┏",
                                     .top_right = "┓",
                                     .bottom_left = "┗",
                                     .bottom_right = "┛",
                                     .horizontal = "━",
                                     .vertical = "┃" };
            case BorderStyle::Single:
                return BorderGlyphs{ .top_left = "┌",
                                     .top_right = "┐",
                                     .bottom_left = "└",
                                     .bottom_right = "┘",
                                     .horizontal = "─",
                                     .vertical = "│" };
            case BorderStyle::Rounded:
            default:
                return BorderGlyphs{ .top_left = "╭",
                                     .top_right = "╮",
                                     .bottom_left = "╰",
                                     .bottom_right = "╯",
                                     .horizontal = "─",
                                     .vertical = "│" };
        }
    }

}

class ScreenBuffer
{
  public:
    constexpr ScreenBuffer(const Size& size, Style clear_style = {}) { resize(size, clear_style); }

    constexpr auto resize(const Size& size, Style clear_style) -> void
    {
        m_size = size;
        m_bounds = { .x = 0, .y = 0, .size = size };
        auto cell_count{ static_cast<size_t>(size.width) * static_cast<size_t>(size.height) };
        m_cells.assign(cell_count, Cell{ .grapheme = " ", .style = clear_style });
    }

    constexpr auto clear(Style clear_style) -> void
    {
        std::ranges::fill(m_cells, Cell{ .grapheme = " ", .style = clear_style });
    }

    constexpr auto put(uint16_t x, uint16_t y, std::string_view glyph, Style style) -> void
    {
        if (glyph.empty() || !getBounds().contains(x, y)) {
            return;
        }

        auto& cell = m_cells[index(x, y)];
        cell.grapheme.assign(glyph);
        cell.style = style;
    }

    constexpr auto text(uint16_t x, uint16_t y, std::string_view text, Style style) -> void
    {
        auto start_x{ x };
        auto line_saturated{ false };
        for (auto i{ 0UZ }; i < text.size();) {
            auto lead{ static_cast<unsigned char>(text[i]) };
            if (text[i] == '\n') {
                if (y == uint16_max()) {
                    break;
                }
                ++y;
                x = start_x;
                line_saturated = false;
                ++i;
                continue;
            }
            auto glyph_len{ std::min(utf8_glyph_length(lead), text.size() - i) };
            if (y >= m_size.height) {
                break;
            }
            if (!line_saturated && x < m_size.width) {
                auto& cell{ m_cells[index(x, y)] };
                cell.grapheme.assign(text.substr(i, glyph_len));
                cell.style = style;
            }
            if (x == uint16_max()) {
                line_saturated = true;
            }
            else {
                ++x;
            }
            i += glyph_len;
        }
    }

    constexpr auto fill(Rect rect, std::string_view glyph, Style style) -> void
    {
        Rect clipped{ rect.intersect(m_bounds) };
        if (clipped.empty() || glyph.empty()) {
            return;
        }
        for (auto yy{ clipped.y }; yy < clipped.bottom(); ++yy) {
            auto* row{ &m_cells[index(clipped.x, yy)] };
            for (auto xx{ clipped.x }; xx < clipped.right(); ++xx) {
                row->grapheme = std::string(glyph);
                row->style = style;
                ++row;
            }
        }
    }

    constexpr auto box(Rect rect, BorderStyle border, Style style) -> void
    {
        if (rect.size.width < 2 || rect.size.height < 2) {
            return;
        }

        auto rect_right{ rect.right() };
        auto rect_bottom{ rect.bottom() };
        if (distance_or_zero(rect.x, rect_right) < 2 || distance_or_zero(rect.y, rect_bottom) < 2) {
            return;
        }

        auto glyphs{ glyphs_for(border) };
        auto left{ rect.x };
        auto right{ static_cast<uint16_t>(rect_right - 1) };
        auto top{ rect.y };
        auto bottom{ static_cast<uint16_t>(rect_bottom - 1) };

        for (auto xx{ add_saturated(left, 1) }; xx < right; ++xx) {
            put(xx, top, glyphs.horizontal, style);
            put(xx, bottom, glyphs.horizontal, style);
        }

        for (auto yy{ add_saturated(top, 1) }; yy < bottom; ++yy) {
            put(left, yy, glyphs.vertical, style);
            put(right, yy, glyphs.vertical, style);
        }

        put(left, top, glyphs.top_left, style);
        put(right, top, glyphs.top_right, style);
        put(left, bottom, glyphs.bottom_left, style);
        put(right, bottom, glyphs.bottom_right, style);
    }

    constexpr auto at(uint16_t x, uint16_t y) const -> const Cell&
    {
        if (!m_bounds.contains(x, y)) {
            throw std::out_of_range("ScreenBuffer::at coordinates out of range");
        }
        return m_cells[index(x, y)];
    }

    constexpr auto getSize() const -> const Size& { return m_size; }
    constexpr auto getBounds() const -> Rect { return Rect{ .x = 0, .y = 0, .size = m_size }; }

  private:
    constexpr auto index(uint16_t x, uint16_t y) const -> size_t
    {
        return (static_cast<size_t>(y) * static_cast<size_t>(m_size.width)) + static_cast<size_t>(x);
    }

    Size m_size;
    Rect m_bounds;
    std::vector<Cell> m_cells;
};

auto main() -> int
{
    constexpr Size demo_screen_size{ .width = 32, .height = 6 };

    ScreenBuffer screen(demo_screen_size);
    screen.box(Rect{ .x = 0, .y = 0, .size = demo_screen_size }, BorderStyle::Rounded, Style{});
    screen.text(2, 2, "Cell buffer works", Style{});

    auto [width, height] = screen.getSize();
    for (uint16_t y = 0; y < height; ++y) {
        for (uint16_t x = 0; x < width; ++x) {
            std::cout << screen.at(x, y).grapheme;
        }
        std::cout << '\n';
    }

    return 0;
}
