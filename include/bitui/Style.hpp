#pragma once

#include "Color.hpp"

#include <string>

namespace bitui
{
    enum class BorderStyle : uint8_t
    {
        Single,
        Rounded,
        Heavy,
    };

    namespace detail
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

        constexpr auto DEFAULT_FOREGROUND_COLOR{ Color::rgb(226, 232, 240) };
        constexpr auto DEFAULT_BACKGROUND_COLOR{ Color::rgb(10, 14, 22) };
    }

    struct Style
    {
        Color fg{ detail::DEFAULT_FOREGROUND_COLOR };
        Color bg{ detail::DEFAULT_BACKGROUND_COLOR };
        bool bold{};
        bool dim{};
        bool italic{};
        bool underline{};
    };
}