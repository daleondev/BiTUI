#pragma once

#include "geometry.hpp"

#include <memory>
#include <string_view>

namespace bitui
{
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
        auto setCursorVisible(bool visible) -> void;
        auto useAlternateScreen(bool enabled) -> void;

      private:
        struct State;

        std::unique_ptr<State> m_state{ nullptr };
        std::string m_pendingInput{};
    };
}