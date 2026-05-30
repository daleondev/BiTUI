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
        termios original_termios{};
        int input_fd{ -1 };
        int output_fd{ -1 };
#endif
    };

    Terminal::Terminal()
      : m_state{ std::make_unique<State>() }
    {
        using namespace std::string_literals;

#if defined(_WIN32) || defined(_WIN64)
        auto fail{ [](std::string_view msg) {
            throw std::runtime_error(msg + ": "s + std::system_category().message(GetLastError()));
        } };

        m_state->input_handle = GetStdHandle(STD_INPUT_HANDLE);
        m_state->output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (m_state->input_handle == INVALID_HANDLE_VALUE || m_state->output_handle == INVALID_HANDLE_VALUE) {
            throw fail("Failed to acquire standard console handles");
        }

        if (GetConsoleMode(m_state->input_handle, &m_state->original_input_mode) == FALSE ||
            GetConsoleMode(m_state->output_handle, &m_state->original_output_mode) == FALSE) {
            throw fail("Failed to get console mode");
        }

        if (SetConsoleOutputCP(CP_UTF8) == FALSE || SetConsoleCP(CP_UTF8) == FALSE) {
            throw fail("Failed to configure console UTF-8 code pages");
        }

        auto configured_output_mode{ m_state->original_output_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING };
        if (SetConsoleMode(m_state->output_handle, configured_output_mode) == FALSE) {
            throw fail("Failed to enable virtual terminal output");
        }

        auto configured_input_mode{ m_state->original_input_mode };
        configured_input_mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
        configured_input_mode |= ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS;
        if (SetConsoleMode(m_state->input_handle, configured_input_mode) == FALSE) {
            throw fail("Failed to enable raw console input");
        }

        m_state->queued_events.clear();
#else
        auto fail{ [](std::string_view msg) {
            throw std::runtime_error(msg + ": "s + std::strerror(errno));
        } };

        m_state->input_fd = STDIN_FILENO;
        m_state->output_fd = STDOUT_FILENO;

        if (::isatty(m_state->input_fd) != 1 || ::isatty(m_state->output_fd) != 1) {
            fail("Raw terminal input requires an interactive TTY");
        }

        if (::tcgetattr(m_state->input_fd, &m_state->original_termios) != 0) {
            fail("Failed to read terminal attributes");
        }

        auto termios{ m_state->original_termios };
        ::cfmakeraw(&termios);
        termios.c_cc[VMIN] = 0;
        termios.c_cc[VTIME] = 0;

        if (::tcsetattr(m_state->input_fd, TCSAFLUSH, &termios) != 0) {
            fail("Failed to enable raw terminal input");
        }
#endif

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

#if defined(_WIN32) || defined(_WIN64)
        SetConsoleMode(m_state->input_handle, m_state->original_input_mode);
        SetConsoleMode(m_state->output_handle, m_state->original_output_mode);

        m_state->queued_events.clear();
#else
        ::tcsetattr(m_state->input_fd, TCSAFLUSH, &m_state->original_termios);
#endif

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