#include "bitui/bitui.hpp"

auto main() -> int
{
    constexpr bitui::Size demo_screen_size{ .width = 32, .height = 6 };

    bitui::ScreenBuffer screen(demo_screen_size);
    screen.box(
      bitui::Rect{ .x = 0, .y = 0, .size = demo_screen_size }, bitui::BorderStyle::Single, bitui::Style{});
    screen.text(2, 2, "Cell buffer works", bitui::Style{});

    auto [width, height] = screen.getSize();
    for (uint16_t y = 0; y < height; ++y) {
        for (uint16_t x = 0; x < width; ++x) {
            std::cout << screen.at(x, y).grapheme;
        }
        std::cout << '\n';
    }

    return 0;
}
