#include "bitui/bitui.hpp"

auto render_full(const bitui::ScreenBuffer& buffer) -> std::string
{
    std::string output;
    auto [width, height] = buffer.getSize();
    output.reserve(static_cast<std::size_t>(width * height * 8));

    bitui::Style current_style{};
    bool has_style = false;
    for (std::size_t y = 0; y < height; ++y) {
        bitui::ansi::append_move_cursor(output, 0, static_cast<int>(y));
        for (std::size_t x = 0; x < width; ++x) {
            const bitui::Cell& cell = buffer[x, y];
            if (!has_style || !(cell.style == current_style)) {
                bitui::ansi::append_style_sequence(output, cell.style);
                current_style = cell.style;
                has_style = true;
            }
            output += cell.grapheme;
        }
    }
    output += bitui::ansi::reset();
    return output;
}

auto main() -> int
{
    constexpr bitui::Size demo_screen_size{ .width = 32, .height = 6 };

    bitui::ScreenBuffer screen(demo_screen_size);
    screen.box(
      bitui::Rect{ .x = 0, .y = 0, .size = demo_screen_size }, bitui::BorderStyle::Heavy, bitui::Style{});
    screen.text(2, 2, "Cell buffer works", bitui::Style{});

    auto output{ render_full(screen) };
    std::cout << output;

    // std::string output;

    // auto [width, height] = screen.getSize();
    // for (uint16_t y = 0; y < height; ++y) {
    //     bitui::ansi::append_move_cursor(output, 0, static_cast<int>(y));
    //     for (uint16_t x = 0; x < width; ++x) {
    //         std::fputs(screen.at(x, y).grapheme.data(), stdout);
    //     }
    //     std::fputc('\n', stdout);
    // }

    return 0;
}
