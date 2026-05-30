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
            output.reserve(static_cast<size_t>(width) * static_cast<size_t>(height));

            bitui::Style current_style{};
            auto has_style{ false };
            for (auto y : std::views::indices(height)) {
                bitui::ansi::append_move_cursor(output, 0, y);
                for (auto x : std::views::indices(width)) {
                    const auto& cell{ buffer[x, y] };
                    if (!has_style || cell.style != current_style) {
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
        output.reserve(static_cast<size_t>(width) * static_cast<size_t>(height));

        Style current_style{};
        auto has_style{ false };
        auto wrote_cells{ false };

        for (auto y : std::views::indices(height)) {
            uint16_t x{ 0 };
            while (x < width) {
                auto equal{ [&](uint16_t xx) { return buffer[xx, y] == m_previous.value()[xx, y]; } };

                auto tail{ std::views::iota(x, width) };
                auto diff_start{ std::ranges::find_if_not(tail, equal) };

                if (diff_start == std::ranges::end(tail)) {
                    break;
                }

                auto excerpt{ std::ranges::subrange(diff_start, std::ranges::end(tail)) };
                auto diff_end(std::ranges::find_if(excerpt, equal));

                auto start_x{ *diff_start };
                auto end_x{ diff_end == std::ranges::end(excerpt) ? width : *diff_end };

                ansi::append_move_cursor(output, start_x, y);

                for (uint16_t pos{ start_x }; pos < end_x; ++pos) {
                    const auto& cell{ buffer[pos, y] };
                    if (!has_style || cell.style != current_style) {
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
            bitui::ansi::append_move_cursor(output, 0, height);
            output += ansi::reset();
        }
        return output;
    }

    auto Renderer::reset() -> void { m_previous.reset(); }

}