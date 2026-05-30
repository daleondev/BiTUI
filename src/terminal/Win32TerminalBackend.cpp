#include "terminal/TerminalBackend.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <cstdio>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace bitui::detail
{
    namespace
    {
        auto fail(std::string_view message) -> std::runtime_error
        {
            throw std::runtime_error(std::string(message) + ": " +
                                     std::system_category().message(GetLastError()));
        }

        class Win32TerminalBackend final : public TerminalBackend
        {
          public:
            Win32TerminalBackend()
            {
                try {
                    configureConsole();
                } catch (...) {
                    restoreConsole();
                    throw;
                }
            }

            ~Win32TerminalBackend() override { restoreConsole(); }

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

          private:
            auto configureConsole() -> void
            {
                m_inputHandle = GetStdHandle(STD_INPUT_HANDLE);
                m_outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
                if (m_inputHandle == INVALID_HANDLE_VALUE || m_inputHandle == nullptr ||
                    m_outputHandle == INVALID_HANDLE_VALUE || m_outputHandle == nullptr) {
                    throw fail("Failed to acquire standard console handles");
                }

                if (auto inputMode{}; GetConsoleMode(m_inputHandle, &inputMode) == TRUE) {
                    m_originalInputMode.emplace(inputMode);
                }
                else {
                    throw fail("Failed to get console input mode");
                }

                if (auto outputMode{}; GetConsoleMode(m_outputHandle, &outputMode) == TRUE) {
                    m_originalOutputMode.emplace(outputMode);
                }
                else {
                    throw fail("Failed to get console output mode");
                }

                m_originalInputCodePage.emplace(GetConsoleCP());
                m_originalOutputCodePage.emplace(GetConsoleOutputCP());

                if (SetConsoleOutputCP(CP_UTF8) != TRUE || SetConsoleCP(CP_UTF8) != TRUE) {
                    throw fail("Failed to configure console UTF-8 code pages");
                }

                auto configured_output_mode{ m_originalOutputMode.value() |
                                             ENABLE_VIRTUAL_TERMINAL_PROCESSING };
                if (SetConsoleMode(m_outputHandle, configured_output_mode) == FALSE) {
                    throw fail("Failed to enable virtual terminal output");
                }

                auto configured_input_mode{ m_originalInputMode.value() };
                configured_input_mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
                configured_input_mode |= ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS;
                if (SetConsoleMode(m_inputHandle, configured_input_mode) == FALSE) {
                    throw fail("Failed to enable raw console input");
                }
            }

            auto restoreConsole() noexcept -> void
            {
                if (m_originalInputMode.has_value()) {
                    SetConsoleMode(m_inputHandle, m_originalInputMode.value());
                    m_originalInputMode.clear();
                }

                if (m_originalOutputMode.has_value()) {
                    SetConsoleMode(m_outputHandle, m_originalOutputMode.value());
                    m_originalOutputMode.clear();
                }

                if (m_originalInputCodePage.has_value()) {
                    (void)SetConsoleCP(m_originalInputCodePage.value());
                    m_originalInputCodePage.clear();
                }

                if (m_originalOutputCodePage.has_value()) {
                    (void)SetConsoleOutputCP(m_originalOutputCodePage.value());
                    m_originalOutputCodePage.clear();
                }
            }

            HANDLE m_inputHandle{ nullptr };
            HANDLE m_outputHandle{ nullptr };
            std::optional<DWORD> m_originalInputMode;
            std::optional<DWORD> m_originalOutputMode;
            std::optional<UINT> m_originalInputCodePage;
            std::optional<UINT> m_originalOutputCodePage;
        };
    }

    auto create_terminal_backend() -> std::unique_ptr<TerminalBackend>
    {
        return std::make_unique<Win32TerminalBackend>();
    }
}
