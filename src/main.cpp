#include "bitui/bitui.hpp"

auto main() -> int
{
    constexpr bitui::Size demo_screen_size{ .width = 32, .height = 6 };

    bitui::ScreenBuffer screen(demo_screen_size);
    screen.box(
      bitui::Rect{ .x = 0, .y = 0, .size = demo_screen_size }, bitui::BorderStyle::Rounded, bitui::Style{});
    screen.text(2, 2, "Cell buffer works", bitui::Style{});

    bitui::Renderer renderer;

    std::cout << renderer.render(screen) << std::flush;

    screen.box(bitui::Rect{ .x = 0, .y = 0, .size = bitui::Size{ .width = 4, .height = 4 } },
               bitui::BorderStyle::Heavy,
               bitui::Style{});

    std::cout << renderer.render(screen) << std::flush;

    return 0;
}
