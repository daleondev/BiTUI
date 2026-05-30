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

        write(ansi::enter_alternate_screen() + ansi::hide_cursor());
        flush();
    }

    Terminal::~Terminal()
    {
        try {
            write(ansi::leave_alternate_screen() + ansi::show_cursor() + ansi::reset());
            flush();
        } catch (const std::exception& e) {
            std::fprintf(stderr, "Exception occurred during Terminal shutdown: %s", e.what());
        }

        m_pendingInput.clear();
    }

    auto Terminal::write(std::string_view data) -> void { m_backend->write(data); }

    auto Terminal::flush() -> void { m_backend->flush(); }

    auto Terminal::clearScreen() -> void
    {
        std::string clear_sequence{ ansi::clear_screen() };
        ansi::append_move_cursor(clear_sequence, 0, 0);
        write(clear_sequence);
    }

    auto Terminal::getSize() const -> Size { return m_backend->getSize(); }
}
