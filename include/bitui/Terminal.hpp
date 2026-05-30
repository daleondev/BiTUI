#pragma once

#include "geometry.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace bitui
{
    namespace detail
    {
        class TerminalBackend;
    }

    class Terminal
    {
      public:
        Terminal();
        ~Terminal();

        Terminal(const Terminal&) = delete;
        auto operator=(const Terminal&) -> Terminal& = delete;
        Terminal(Terminal&&) noexcept = delete;
        auto operator=(Terminal&&) noexcept -> Terminal& = delete;

        auto write(std::string_view data) -> void;
        auto flush() -> void;
        auto clearScreen() -> void;

      private:
        std::unique_ptr<detail::TerminalBackend> m_backend{ nullptr };
        std::string m_pendingInput{};
    };
}
