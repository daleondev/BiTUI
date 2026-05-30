#include "bitui/Terminal.hpp"

#include "bitui/ansi.hpp"

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <cerrno>
#include <cstring>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace bitui
{
    struct Terminal::State
    {
#if defined(_WIN32) || defined(_WIN64)
        HANDLE input_handle{ nullptr };
        HANDLE output_handle{ nullptr };
        DWORD original_input_mode{};
        DWORD original_output_mode{};
        std::deque<TerminalEvent> queued_events;
#else
        int input_fd{ -1 };
        int output_fd{ -1 };
        termios original_termios{};
#endif
    };

    Terminal::Terminal()
      : m_state{ std::make_unique<State>() }
    {
        using namespace std::string_literals;

        m_state->input_fd = STDIN_FILENO;
        m_state->output_fd = STDOUT_FILENO;

        if (::isatty(m_state->input_fd) != 1 || ::isatty(m_state->output_fd) != 1) {
            throw std::runtime_error("Raw terminal input requires an interactive TTY");
        }

        if (::tcgetattr(m_state->input_fd, &m_state->original_termios) != 0) {
            throw std::runtime_error("Failed to read terminal attributes: "s + std::strerror(errno));
        }

        auto termios{ m_state->original_termios };
        ::cfmakeraw(&termios);
        termios.c_cc[VMIN] = 0;
        termios.c_cc[VTIME] = 0;

        if (::tcsetattr(m_state->input_fd, TCSAFLUSH, &termios) != 0) {
            throw std::runtime_error("Failed to enable raw terminal input: "s + std::strerror(errno));
        }

        m_pendingInput.clear();

        write(ansi::enter_alternate_screen() + ansi::hide_cursor());
        flush();
    }

    Terminal::~Terminal()
    {
        try {
            write(ansi::enter_alternate_screen() + ansi::hide_cursor() + ansi::reset());
            flush();
        } catch (const std::exception& e) {
            std::fprintf(stderr, "Exception occured during Terminal shutdown: %s", e.what());
        }

        ::tcsetattr(m_state->input_fd, TCSAFLUSH, &m_state->original_termios);
        m_pendingInput.clear();
    }

    auto Terminal::write(std::string_view data) -> void
    {
        if (std::fwrite(data.data(), sizeof(char), data.size(), stdout) != data.size()) {
            throw std::runtime_error("Failed to write terminal output");
        }
    }

    auto Terminal::flush() -> void
    {
        if (std::fflush(stdout) != 0) {
            throw std::runtime_error("Failed to flush terminal output");
        }
    }

    auto Terminal::clearScreen() -> void
    {
        std::string clear_sequence{ ansi::clear_screen() };
        ansi::append_move_cursor(clear_sequence, 0, 0);
        write(clear_sequence);
    }
}