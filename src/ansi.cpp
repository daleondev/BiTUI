#include "bitui/ansi.hpp"

#include <algorithm>
#include <array>
#include <charconv>

namespace bitui::ansi
{
    namespace
    {
        template<std::integral IntType>
        auto append_num(std::string& output, IntType value) -> void
        {
            constexpr size_t buffer_size{ std::numeric_limits<IntType>::digits10 + 1 };
            std::array<char, buffer_size> buffer{};
            auto result{ std::to_chars(buffer.data(), buffer.data() + buffer.size(), value) };
            output.append(buffer.data(), result.ptr);
        }
    } // namespace

    auto reset() -> std::string { return "\x1b[0m"; }

    auto clear_screen() -> std::string { return "\x1b[2J"; }

    auto move_cursor(uint16_t x, uint16_t y) -> std::string
    {
        std::string output;
        constexpr auto reserved_size{ (std::numeric_limits<uint16_t>::digits10 * 2UZ) + 8UZ };
        output.reserve(reserved_size);
        append_move_cursor(output, x, y);
        return output;
    }

    auto hide_cursor() -> std::string { return "\x1b[?25l"; }

    auto show_cursor() -> std::string { return "\x1b[?25h"; }

    auto enter_alternate_screen() -> std::string { return "\x1b[?1049h"; }

    auto leave_alternate_screen() -> std::string { return "\x1b[?1049l"; }

    auto style_sequence(const Style& style) -> std::string
    {
        std::string output;
        constexpr auto reserved_size{ 64UZ };
        output.reserve(reserved_size);
        append_style_sequence(output, style);
        return output;
    }

    auto append_move_cursor(std::string& output, uint16_t x, uint16_t y) -> void
    {
        output += "\x1b[";
        append_num(output, std::max<uint16_t>(0, y) + 1);
        output += ';';
        append_num(output, std::max<uint16_t>(0, x) + 1);
        output += 'H';
    }

    auto append_style_sequence(std::string& output, const Style& style) -> void
    {
        output += "\x1b[0";
        if (style.bold) {
            output += ";1";
        }
        if (style.dim) {
            output += ";2";
        }
        if (style.italic) {
            output += ";3";
        }
        if (style.underline) {
            output += ";4";
        }
        output += ";38;2;";
        append_num(output, style.fg.r);
        output += ';';
        append_num(output, style.fg.g);
        output += ';';
        append_num(output, style.fg.b);
        output += ";48;2;";
        append_num(output, style.bg.r);
        output += ';';
        append_num(output, style.bg.g);
        output += ';';
        append_num(output, style.bg.b);
        output += 'm';
    }
}