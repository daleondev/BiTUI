#include "bitui/Application.hpp"

#include "bitui/Renderer.hpp"

#include <atomic>
#include <format>
#include <thread>

namespace bitui
{
    Application::Application(Options options)
      : m_options{ options }
    {
    }

    auto Application::run() -> int
    {
        Terminal terminal;

        Renderer renderer;
        auto next_cycle{ std::chrono::steady_clock::now() };
        ScreenBuffer buffer(Size{}, m_options.clear_style);
        std::chrono::steady_clock::duration cycle_time{};

        std::atomic_bool running{ true };
        while (running) {
            auto start{ std::chrono::steady_clock::now() };

            next_cycle += m_options.tick_rate;

            auto size{ terminal.getSize() };
            if (buffer.getSize() != size) {
                buffer.resize(size, m_options.clear_style);
            }
            else {
                buffer.clear(m_options.clear_style);
            }

            buffer.box(bitui::Rect{ .x = 0, .y = 0, .size = { .width = 10, .height = 6 } },
                       bitui::BorderStyle::Rounded,
                       bitui::Style{});
            buffer.text(
              2,
              2,
              std::format("Cycle Time: {} ms",
                          std::chrono::duration_cast<std::chrono::milliseconds>(cycle_time).count()),
              bitui::Style{});

            terminal.write(renderer.render(buffer));
            terminal.flush();

            std::this_thread::sleep_until(next_cycle);

            auto end{ std::chrono::steady_clock::now() };
            cycle_time = end - start;
        }

        return 0;
    }
}