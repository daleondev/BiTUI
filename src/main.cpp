#include <algorithm>
#include <cmath>
#include <iostream>
#include <print>
#include <ranges>
#include <vector>

struct Color
{
    uint8_t r{};
    uint8_t g{};
    uint8_t b{};
};

constexpr Color rgb(uint8_t r, uint8_t g, uint8_t b) { return Color{ r, g, b }; }

struct Style
{
    Color fg{ rgb(226, 232, 240) };
    Color bg{ rgb(10, 14, 22) };
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

    friend constexpr auto operator<=>(const Size&, const Size&) = default;
};

struct Rect
{
    uint16_t x{};
    uint16_t y{};
    Size size{};

    constexpr auto right() const -> uint16_t { return x + size.width; }
    constexpr auto bottom() const -> uint16_t { return y + size.height; }
    constexpr auto empty() const -> bool { return size.width <= 0 || size.height <= 0; }

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
                     .size = { .width = std::max<uint16_t>(0, x2 - x1),
                               .height = std::max<uint16_t>(0, y2 - y1) } };
    }

    friend constexpr auto operator<=>(const Rect&, const Rect&) = default;
};

enum class BorderStyle
{
    Single,
    Rounded,
    Heavy,
};

namespace
{

    constexpr auto utf8_glyph_length(unsigned char lead) -> size_t
    {
        if ((lead & 0b1000'0000U) == 0U) {
            return 1;
        }
        if ((lead & 0b1110'0000U) == 0b1100'0000U) {
            return 2;
        }
        if ((lead & 0b1111'0000U) == 0b1110'0000U) {
            return 3;
        }
        if ((lead & 0b1111'1000U) == 0b1111'0000U) {
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
                return { "┏", "┓", "┗", "┛", "━", "┃" };
            case BorderStyle::Single:
                return { "┌", "┐", "└", "┘", "─", "│" };
            case BorderStyle::Rounded:
            default:
                return { "╭", "╮", "╰", "╯", "─", "│" };
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
        auto cell_count{ static_cast<std::size_t>(size.width) * static_cast<std::size_t>(size.height) };
        m_cells.assign(cell_count, Cell{ .grapheme = " ", .style = clear_style });
    }

    constexpr auto clear(Style clear_style)
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
        for (auto i{ 0UZ }; i < text.size();) {
            auto lead{ static_cast<unsigned char>(text[i]) };
            if (text[i] == '\n') {
                ++y;
                x = start_x;
                ++i;
                continue;
            }
            auto glyph_len{ std::min(utf8_glyph_length(lead), text.size() - i) };
            if (y >= m_size.height) {
                break;
            }
            if (y >= 0 && x >= 0 && x < m_size.width) {
                auto& cell{ m_cells[index(x, y)] };
                cell.grapheme.assign(text.substr(i, glyph_len));
                cell.style = style;
            }
            ++x;
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

        auto glyphs{ glyphs_for(border) };
        auto left{ rect.x };
        auto right{ rect.right() - 1 };
        auto top{ rect.y };
        auto bottom{ rect.bottom() - 1 };

        auto horizontal_start{ left + 1 };
        auto horizontal_end{ m_size.width - 1 };
        if (horizontal_start <= horizontal_end) {
            for (auto xx{ horizontal_start }; xx <= horizontal_end; ++xx) {
                put(xx, top, glyphs.horizontal, style);
                put(xx, bottom, glyphs.horizontal, style);
            }
        }

        auto vertical_start{ top + 1 };
        auto vertical_end{ m_size.height - 1 };
        if (vertical_start <= vertical_end) {
            for (auto yy{ vertical_start }; yy <= vertical_end; ++yy) {
                put(left, yy, glyphs.vertical, style);
                put(right, yy, glyphs.vertical, style);
            }
        }

        put(left, top, glyphs.top_left, style);
        put(right, top, glyphs.top_right, style);
        put(left, bottom, glyphs.bottom_left, style);
        put(right, bottom, glyphs.bottom_right, style);
    }

    auto at(uint16_t x, uint16_t y) const -> const Cell&
    {
        if (!m_bounds.contains(x, y)) {
            throw std::out_of_range("ScreenBuffer::at coordinates out of range");
        }
        return m_cells[index(x, y)];
    }

    constexpr auto getSize() -> const Size& { return m_size; }
    constexpr auto getBounds() -> Rect { return Rect{ .x = 0, .y = 0, .size = m_size }; }

  private:
    constexpr auto index(uint16_t x, uint16_t y) const -> size_t
    {
        return static_cast<std::size_t>(y * m_size.width + x);
    }

    Size m_size;
    Rect m_bounds;
    std::vector<Cell> m_cells;
};

auto main() -> int
{
    ScreenBuffer screen({ 32, 6 });
    screen.box({ 0, 0, { 32, 6 } }, BorderStyle::Rounded, Style{});
    screen.text(2, 2, "Cell buffer works", Style{});

    auto [width, height] = screen.getSize();
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            std::cout << screen.at(x, y).grapheme;
        }
        std::cout << '\n';
    }

    return 0;
}