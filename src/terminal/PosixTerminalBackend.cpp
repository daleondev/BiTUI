#include "terminal/TerminalBackend.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace bitui::detail
{
    namespace
    {
        auto fail(std::string_view message) -> std::runtime_error
        {
            throw std::runtime_error(std::string(message) + ": " + std::strerror(errno));
        }

        class PosixTerminalBackend final : public TerminalBackend
        {
          public:
            PosixTerminalBackend()
            {
                try {
                    configureTerminal();
                } catch (...) {
                    restoreTerminal();
                    throw;
                }
            }

            ~PosixTerminalBackend() override { restoreTerminal(); }

            auto write(std::string_view data) -> void override
            {
                if (std::fwrite(data.data(), sizeof(char), data.size(), stdout) != data.size()) {
                    throw std::runtime_error("Failed to write terminal output");
                }
            }

            auto flush() -> void override
            {
                if (std::fflush(stdout) != 0) {
                    throw std::runtime_error("Failed to flush terminal output");
                }
            }

            auto getSize() const -> Size override
            {
                winsize ws{};
                if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
                    return Size{ .width = ws.ws_col, .height = ws.ws_row };
                }
                return Size{};
            }

          private:
            auto configureTerminal() -> void
            {
                if (::isatty(m_inputFd) != 1 || ::isatty(m_outputFd) != 1) {
                    fail("Raw terminal input requires an interactive TTY");
                }

                termios termios{};
                if (::tcgetattr(m_inputFd, &termios) != 0) {
                    fail("Failed to read terminal attributes");
                }

                m_originalTermios.emplace(termios);

                ::cfmakeraw(&termios);
                termios.c_cc[VMIN] = 0;
                termios.c_cc[VTIME] = 0;

                if (::tcsetattr(m_inputFd, TCSAFLUSH, &termios) != 0) {
                    fail("Failed to enable raw terminal input");
                }
            }

            auto restoreTerminal() noexcept -> void
            {
                if (m_originalTermios.has_value()) {
                    (void)::tcsetattr(m_inputFd, TCSAFLUSH, &m_originalTermios.value());
                    m_originalTermios.reset();
                }
            }

            int m_inputFd{ STDIN_FILENO };
            int m_outputFd{ STDOUT_FILENO };
            std::optional<termios> m_originalTermios;
        };
    }

    auto create_terminal_backend() -> std::unique_ptr<TerminalBackend>
    {
        return std::make_unique<PosixTerminalBackend>();
    }
}
