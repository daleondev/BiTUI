#pragma once

#include <memory>
#include <string_view>

namespace bitui::detail
{
    class TerminalBackend
    {
      public:
        TerminalBackend() = default;
        virtual ~TerminalBackend() = default;

        TerminalBackend(const TerminalBackend&) = delete;
        auto operator=(const TerminalBackend&) -> TerminalBackend& = delete;
        TerminalBackend(TerminalBackend&&) noexcept = delete;
        auto operator=(TerminalBackend&&) noexcept -> TerminalBackend& = delete;

        virtual auto write(std::string_view data) -> void = 0;
        virtual auto flush() -> void = 0;
    };

    auto create_terminal_backend() -> std::unique_ptr<TerminalBackend>;
}
