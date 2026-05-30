#include "bitui/bitui.hpp"

auto main() -> int
{
    bitui::Terminal terminal(bitui::TerminalOptions{
      .raw_input = false,
      .use_alternate_screen = false,
      .hide_cursor = false,
    });

    constexpr bitui::Size demo_screen_size{ .width = 32, .height = 6 };

    bitui::ScreenBuffer screen(demo_screen_size);
    screen.box(
      bitui::Rect{ .x = 0, .y = 0, .size = demo_screen_size }, bitui::BorderStyle::Rounded, bitui::Style{});
    screen.text(2, 2, "Cell buffer works", bitui::Style{});

    bitui::Renderer renderer;

    terminal.write(renderer.render(screen));
    terminal.flush();

    screen.box(bitui::Rect{ .x = 0, .y = 0, .size = bitui::Size{ .width = 4, .height = 4 } },
               bitui::BorderStyle::Heavy,
               bitui::Style{});

    terminal.write(renderer.render(screen));
    terminal.flush();

    return 0;
}
