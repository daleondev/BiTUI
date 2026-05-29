#pragma once

#include "Style.hpp"

namespace bitui::ansi
{
    auto reset() -> std::string;
    auto clear_screen() -> std::string;
    auto move_cursor(uint16_t x, uint16_t y) -> std::string;
    auto hide_cursor() -> std::string;
    auto show_cursor() -> std::string;
    auto enter_alternate_screen() -> std::string;
    auto leave_alternate_screen() -> std::string;
    auto style_sequence(const Style& style) -> std::string;
    auto append_move_cursor(std::string& output, uint16_t x, uint16_t y) -> void;
    auto append_style_sequence(std::string& output, const Style& style) -> void;
}