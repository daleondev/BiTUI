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

#include "Style.hpp"
#include "geometry.hpp"

namespace bitui
{
    struct Cell
    {
        std::string grapheme{ " " };
        Style style{};
    };

    class ScreenBuffer
    {
      public:
        ScreenBuffer(const Size& size, Style clear_style = {});

        auto resize(const Size& size, Style clear_style) -> void;
        auto clear(Style clear_style) -> void;

        auto put(uint16_t x, uint16_t y, std::string_view glyph, Style style) -> void;
        auto text(uint16_t x, uint16_t y, std::string_view text, Style style) -> void;
        auto fill(Rect rect, std::string_view glyph, Style style) -> void;
        auto box(Rect rect, BorderStyle border, Style style) -> void;

        auto at(uint16_t x, uint16_t y) const -> const Cell&;

        auto getSize() const -> const Size&;
        auto getBounds() const -> const Rect&;

      private:
        auto index(uint16_t x, uint16_t y) const -> size_t;

        Size m_size;
        Rect m_bounds;
        std::vector<Cell> m_cells;
    };
}