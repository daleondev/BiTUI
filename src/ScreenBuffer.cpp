#include "bitui/ScreenBuffer.hpp"

namespace bitui
{
    ScreenBuffer::ScreenBuffer(const Size& size, Style clear_style) { resize(size, clear_style); }

    auto ScreenBuffer::resize(const Size& size, Style clear_style) -> void
    {
        m_size = size;
        m_bounds = { .x = 0, .y = 0, .size = size };
        auto cell_count{ static_cast<size_t>(size.width) * static_cast<size_t>(size.height) };
        m_cells.assign(cell_count, Cell{ .grapheme = " ", .style = clear_style });
    }

    auto ScreenBuffer::clear(Style clear_style) -> void
    {
        std::ranges::fill(m_cells, Cell{ .grapheme = " ", .style = clear_style });
    }

    auto ScreenBuffer::put(uint16_t x, uint16_t y, std::string_view glyph, Style style) -> void
    {
        if (glyph.empty() || !getBounds().contains(x, y)) {
            return;
        }

        auto& cell = m_cells[index(x, y)];
        cell.grapheme.assign(glyph);
        cell.style = style;
    }

    auto ScreenBuffer::text(uint16_t x, uint16_t y, std::string_view text, Style style) -> void
    {
        auto start_x{ x };
        auto line_saturated{ false };
        for (auto i{ 0UZ }; i < text.size();) {
            auto lead{ static_cast<unsigned char>(text[i]) };
            if (text[i] == '\n') {
                if (y == std::numeric_limits<uint16_t>::max()) {
                    break;
                }
                ++y;
                x = start_x;
                line_saturated = false;
                ++i;
                continue;
            }
            auto glyph_len{ std::min(detail::utf8_glyph_length(lead), text.size() - i) };
            if (y >= m_size.height) {
                break;
            }
            if (!line_saturated && x < m_size.width) {
                auto& cell{ m_cells[index(x, y)] };
                cell.grapheme.assign(text.substr(i, glyph_len));
                cell.style = style;
            }
            if (x == std::numeric_limits<uint16_t>::max()) {
                line_saturated = true;
            }
            else {
                ++x;
            }
            i += glyph_len;
        }
    }

    auto ScreenBuffer::fill(Rect rect, std::string_view glyph, Style style) -> void
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

    auto ScreenBuffer::box(Rect rect, BorderStyle border, Style style) -> void
    {
        if (rect.size.width < 2 || rect.size.height < 2) {
            return;
        }

        auto rect_right{ rect.right() };
        auto rect_bottom{ rect.bottom() };
        if (detail::distance_or_zero(rect.x, rect_right) < 2 ||
            detail::distance_or_zero(rect.y, rect_bottom) < 2) {
            return;
        }

        auto glyphs{ detail::glyphs_for(border) };
        auto left{ rect.x };
        auto right{ static_cast<uint16_t>(rect_right - 1) };
        auto top{ rect.y };
        auto bottom{ static_cast<uint16_t>(rect_bottom - 1) };

        for (auto xx{ detail::add_saturated(left, 1) }; xx < right; ++xx) {
            put(xx, top, glyphs.horizontal, style);
            put(xx, bottom, glyphs.horizontal, style);
        }

        for (auto yy{ detail::add_saturated(top, 1) }; yy < bottom; ++yy) {
            put(left, yy, glyphs.vertical, style);
            put(right, yy, glyphs.vertical, style);
        }

        put(left, top, glyphs.top_left, style);
        put(right, top, glyphs.top_right, style);
        put(left, bottom, glyphs.bottom_left, style);
        put(right, bottom, glyphs.bottom_right, style);
    }

    auto ScreenBuffer::at(uint16_t x, uint16_t y) const -> const Cell&
    {
        if (!m_bounds.contains(x, y)) {
            throw std::out_of_range("ScreenBuffer::at coordinates out of range");
        }
        return m_cells[index(x, y)];
    }

    auto ScreenBuffer::getSize() const -> const Size& { return m_size; }

    auto ScreenBuffer::getBounds() const -> const Rect& { return Rect{ .x = 0, .y = 0, .size = m_size }; }

    auto ScreenBuffer::index(uint16_t x, uint16_t y) const -> size_t
    {
        return (static_cast<size_t>(y) * static_cast<size_t>(m_size.width)) + static_cast<size_t>(x);
    }
}