
#pragma once

#include <algorithm>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <mdspan>
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

        constexpr auto operator<=>(const Cell&) const = default;
    };

    using CellGrid = std::mdspan<Cell, std::dextents<std::size_t, 2>, std::layout_right>;
    using ConstCellGrid = std::mdspan<const Cell, std::dextents<std::size_t, 2>, std::layout_right>;

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

        template<typename Self>
        constexpr auto at(this Self&& self, uint16_t x, uint16_t y)
          -> std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const Cell&, Cell&>
        {
            return self.grid().at(y, x);
        }

        template<typename Self>
        constexpr auto operator[](this Self&& self, auto... coords)
          -> std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const Cell&, Cell&>
        {
            static_assert(sizeof...(coords) == 2, "ScreenBuffer::operator[] requires exactly 2 coordinates");
            static_assert((std::integral<decltype(coords)> && ...),
                          "ScreenBuffer::operator[] coordinates must be integral");

            auto [x, y] = std::tuple(static_cast<uint16_t>(coords)...);
            return self.grid()[y, x];
        }

        constexpr auto getSize() const -> const Size& { return m_size; }
        constexpr auto getBounds() const -> const Rect& { return m_bounds; }

      private:
        template<typename Self>
        constexpr auto grid(this Self&& self)
          -> std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, ConstCellGrid, CellGrid>
        {
            if constexpr (std::is_const_v<std::remove_reference_t<Self>>) {
                return ConstCellGrid(self.m_cells.data(), self.m_size.height, self.m_size.width);
            }
            else {
                return CellGrid(self.m_cells.data(), self.m_size.height, self.m_size.width);
            }
        }

        Size m_size;
        Rect m_bounds;
        std::vector<Cell> m_cells;
    };
}