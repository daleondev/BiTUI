#include "bitui/Terminal.hpp"

#include "bitui/ansi.hpp"
#include "terminal/TerminalBackend.hpp"

#include <cstdio>
#include <exception>

namespace bitui
{
    Terminal::Terminal()
      : m_backend{ detail::create_terminal_backend() }
    {
        m_pendingInput.clear();

        useAlternateScreen(true);
        setCursorVisible(false);
        flush();
    }

    Terminal::~Terminal()
    {
        try {
            useAlternateScreen(false);
            setCursorVisible(true);
            write(ansi::reset());
            flush();
        } catch (const std::exception& e) {
            std::fprintf(stderr, "Exception occurred during Terminal shutdown: %s", e.what());
        }

        m_pendingInput.clear();
    }

    auto Terminal::write(std::string_view data) -> void
    {
        m_backend->write(data);
    }

    auto Terminal::flush() -> void
    {
        m_backend->flush();
    }

    auto Terminal::clearScreen() -> void
    {
        std::string clear_sequence{ ansi::clear_screen() };
        ansi::append_move_cursor(clear_sequence, 0, 0);
        write(clear_sequence);
    }

    auto Terminal::setCursorVisible(bool visible) -> void
    {
        write(visible ? ansi::show_cursor() : ansi::hide_cursor());
    }

    auto Terminal::useAlternateScreen(bool enabled) -> void
    {
        write(enabled ? ansi::enter_alternate_screen() : ansi::leave_alternate_screen());
    }
}
