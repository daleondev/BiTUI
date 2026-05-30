#pragma once

#include "geometry.hpp"

#include <chrono>
#include <compare>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace bitui
{
    enum class TerminalKey
    {
        Unknown,
        Character,
        Enter,
        Escape,
        Tab,
        Backspace,
        ArrowUp,
        ArrowDown,
        ArrowLeft,
        ArrowRight,
        Home,
        End,
        Insert,
        Delete,
        PageUp,
        PageDown,
    };

    struct KeyEvent
    {
        TerminalKey key{ TerminalKey::Unknown };
        std::string text{};
        bool ctrl{};
        bool alt{};
        bool shift{};

        constexpr auto operator<=>(const KeyEvent&) const = default;
    };

    struct ResizeEvent
    {
        Size size{};

        constexpr auto operator<=>(const ResizeEvent&) const = default;
    };

    using TerminalEvent = std::variant<KeyEvent, ResizeEvent>;

    struct TerminalOptions
    {
        bool raw_input{ true };
        bool use_alternate_screen{ true };
        bool hide_cursor{ true };

        constexpr auto operator<=>(const TerminalOptions&) const = default;
    };

    class Terminal
    {
      public:
        Terminal();
        explicit Terminal(const TerminalOptions& options);
        ~Terminal();

        Terminal(const Terminal&) = delete;
        auto operator=(const Terminal&) -> Terminal& = delete;
        Terminal(Terminal&&) noexcept = delete;
        auto operator=(Terminal&&) noexcept -> Terminal& = delete;

        auto setup(const TerminalOptions& options = {}) -> void;
        auto restore() -> void;

        [[nodiscard]] auto isSetup() const -> bool;
        [[nodiscard]] auto isInteractive() const -> bool;
        [[nodiscard]] auto supportsAnsi() const -> bool;
        [[nodiscard]] auto options() const -> const TerminalOptions&;

        [[nodiscard]] auto size() const -> Size;

        auto write(std::string_view data) -> void;
        auto flush() -> void;
        auto clearScreen() -> void;
        auto setCursorVisible(bool visible) -> void;
        auto useAlternateScreen(bool enabled) -> void;

        [[nodiscard]] auto pollEvent(std::chrono::milliseconds timeout = std::chrono::milliseconds::zero())
          -> std::optional<TerminalEvent>;
        [[nodiscard]] auto readEvent() -> std::optional<TerminalEvent>;

      private:
        struct State;

        [[nodiscard]] auto takeResizeEvent() -> std::optional<ResizeEvent>;
        [[nodiscard]] auto parsePendingEvent() -> std::optional<KeyEvent>;
        auto readAvailableInput(std::optional<std::chrono::milliseconds> timeout) -> bool;

        std::unique_ptr<State> m_state;
        TerminalOptions m_options{};
        Size m_last_size{};
        std::string m_pending_input;
        bool m_is_setup{};
    };
}