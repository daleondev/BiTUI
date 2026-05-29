#include "bitui/Renderer.hpp"

#include "bitui/ansi.hpp"

namespace bitui
{
    namespace
    {
        auto render_full(const ScreenBuffer& buffer) -> std::string
        {
            std::string output;
            auto [width, height] = buffer.getSize();
            output.reserve(static_cast<std::size_t>(width * height * 8));

            bitui::Style current_style{};
            auto has_style{ false };
            for (uint16_t y{ 0 }; y < height; ++y) {
                bitui::ansi::append_move_cursor(output, 0, static_cast<int>(y));
                for (uint16_t x{ 0 }; x < width; ++x) {
                    const auto& cell{ buffer[x, y] };
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
    }

    auto Renderer::render(const ScreenBuffer& buffer) -> std::string
    {
        if (!m_previous.has_value() || m_previous->getSize() != buffer.getSize()) {
            std::string output{ ansi::clear_screen() + render_full(buffer) };
            m_previous.emplace(buffer);
            return output;
        }

        auto [width, height] = buffer.getSize();
        std::string output;
        output.reserve(static_cast<std::size_t>(width * height));

        Style current_style{};
        auto has_style{ false };
        auto wrote_cells{ false };

        for (uint16_t y{ 0 }; y < height; ++y) {
            uint16_t x{ 0 };
            while (x < width) {
                while (x < width && buffer[x, y] == m_previous.value()[x, y]) {
                    ++x;
                }

                if (x == width) {
                    break;
                }

                auto start_x{ x };
                while (x < width && !(buffer[x, y] == m_previous.value()[x, y])) {
                    ++x;
                }
                auto end_x{ x };

                ansi::append_move_cursor(output, start_x, y);

                for (uint16_t pos{ start_x }; pos < end_x; ++pos) {
                    const auto& cell{ buffer[pos, y] };
                    if (!has_style || !(cell.style == current_style)) {
                        ansi::append_style_sequence(output, cell.style);
                        current_style = cell.style;
                        has_style = true;
                    }
                    output += cell.grapheme;
                    m_previous.value()[pos, y] = cell;
                    wrote_cells = true;
                }
            }
        }

        if (wrote_cells) {
            output += ansi::reset();
        }
        return output;
    }

    auto Renderer::reset() -> void { m_previous.reset(); }

}