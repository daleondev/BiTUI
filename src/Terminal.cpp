#include "bitui/Terminal.hpp"

#include "bitui/ansi.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <cerrno>
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
        void* input_handle{ nullptr };
        void* output_handle{ nullptr };
        unsigned long original_input_mode{};
        unsigned long original_output_mode{};
        bool has_original_input_mode{};
        bool has_original_output_mode{};
        bool input_is_console{};
        bool output_is_console{};
        std::deque<TerminalEvent> queued_events;
#else
        int input_fd{ STDIN_FILENO };
        int output_fd{ STDOUT_FILENO };
        termios original_termios{};
        bool has_original_termios{};
        bool input_is_tty{};
        bool output_is_tty{};
#endif
        bool cursor_hidden{};
        bool alternate_screen{};
    };

    namespace
    {
#if defined(_WIN32) || defined(_WIN64)
        constexpr auto SHIFT_STATE{ static_cast<DWORD>(SHIFT_PRESSED) };
        constexpr auto ALT_STATE{ static_cast<DWORD>(LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED) };
        constexpr auto CTRL_STATE{ static_cast<DWORD>(LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED) };

        auto fail_last_error(std::string_view message) -> void
        {
            const auto error_code{ static_cast<int>(GetLastError()) };
            throw std::runtime_error(std::string(message) + ": " +
                                     std::system_category().message(error_code));
        }

        auto to_utf8(wchar_t wide_char) -> std::string
        {
            if (wide_char == 0) {
                return {};
            }

            std::array<char, 5> buffer{};
            const auto size{ WideCharToMultiByte(
              CP_UTF8, 0, &wide_char, 1, buffer.data(), static_cast<int>(buffer.size()), nullptr, nullptr) };
            if (size <= 0) {
                fail_last_error("Failed to convert console input to UTF-8");
            }

            return std::string(buffer.data(), buffer.data() + size);
        }

        auto make_key_event(TerminalKey key, DWORD control_state) -> KeyEvent
        {
            return KeyEvent{ .key = key,
                             .ctrl = (control_state & CTRL_STATE) != 0,
                             .alt = (control_state & ALT_STATE) != 0,
                             .shift = (control_state & SHIFT_STATE) != 0 };
        }

        auto map_console_key(const KEY_EVENT_RECORD& record) -> std::optional<KeyEvent>
        {
            if (!record.bKeyDown) {
                return std::nullopt;
            }

            switch (record.wVirtualKeyCode) {
                case VK_RETURN:
                    return make_key_event(TerminalKey::Enter, record.dwControlKeyState);
                case VK_ESCAPE:
                    return make_key_event(TerminalKey::Escape, record.dwControlKeyState);
                case VK_TAB:
                    return make_key_event(TerminalKey::Tab, record.dwControlKeyState);
                case VK_BACK:
                    return make_key_event(TerminalKey::Backspace, record.dwControlKeyState);
                case VK_UP:
                    return make_key_event(TerminalKey::ArrowUp, record.dwControlKeyState);
                case VK_DOWN:
                    return make_key_event(TerminalKey::ArrowDown, record.dwControlKeyState);
                case VK_LEFT:
                    return make_key_event(TerminalKey::ArrowLeft, record.dwControlKeyState);
                case VK_RIGHT:
                    return make_key_event(TerminalKey::ArrowRight, record.dwControlKeyState);
                case VK_HOME:
                    return make_key_event(TerminalKey::Home, record.dwControlKeyState);
                case VK_END:
                    return make_key_event(TerminalKey::End, record.dwControlKeyState);
                case VK_INSERT:
                    return make_key_event(TerminalKey::Insert, record.dwControlKeyState);
                case VK_DELETE:
                    return make_key_event(TerminalKey::Delete, record.dwControlKeyState);
                case VK_PRIOR:
                    return make_key_event(TerminalKey::PageUp, record.dwControlKeyState);
                case VK_NEXT:
                    return make_key_event(TerminalKey::PageDown, record.dwControlKeyState);
                default:
                    break;
            }

            if ((record.dwControlKeyState & CTRL_STATE) != 0 && record.wVirtualKeyCode >= 'A' &&
                record.wVirtualKeyCode <= 'Z') {
                auto event{ make_key_event(TerminalKey::Character, record.dwControlKeyState) };
                event.text = std::string(1, static_cast<char>(record.wVirtualKeyCode - 'A' + 'a'));
                return event;
            }

            if (record.uChar.UnicodeChar == 0) {
                return std::nullopt;
            }

            auto event{ make_key_event(TerminalKey::Character, record.dwControlKeyState) };
            event.text = to_utf8(record.uChar.UnicodeChar);
            return event;
        }
#else
        auto fail_errno(std::string_view message) -> void
        {
            throw std::runtime_error(std::string(message) + ": " + std::strerror(errno));
        }
#endif

        auto parse_env_dimension(const char* name) -> uint16_t
        {
            const auto* value{ std::getenv(name) };
            if (value == nullptr || *value == '\0') {
                return 0;
            }

            char* end{};
            const auto parsed{ std::strtol(value, &end, 10) };
            if (end == value || parsed <= 0 || parsed > std::numeric_limits<uint16_t>::max()) {
                return 0;
            }
            return static_cast<uint16_t>(parsed);
        }

        struct SequenceParseResult
        {
            std::optional<KeyEvent> event;
            size_t consumed{};
            bool incomplete{};
        };

        auto apply_xterm_modifier(KeyEvent& event, int modifier) -> void
        {
            switch (modifier) {
                case 2:
                    event.shift = true;
                    break;
                case 3:
                    event.alt = true;
                    break;
                case 4:
                    event.alt = true;
                    event.shift = true;
                    break;
                case 5:
                    event.ctrl = true;
                    break;
                case 6:
                    event.ctrl = true;
                    event.shift = true;
                    break;
                case 7:
                    event.ctrl = true;
                    event.alt = true;
                    break;
                case 8:
                    event.ctrl = true;
                    event.alt = true;
                    event.shift = true;
                    break;
                default:
                    break;
            }
        }

        auto parse_csi_params(std::string_view params_text) -> std::vector<int>
        {
            if (params_text.empty()) {
                return {};
            }

            std::vector<int> params;
            size_t cursor{};
            while (cursor <= params_text.size()) {
                const auto next{ params_text.find(';', cursor) };
                const auto token{ params_text.substr(
                  cursor, next == std::string_view::npos ? params_text.size() - cursor : next - cursor) };

                if (token.empty()) {
                    params.push_back(1);
                }
                else {
                    int value{};
                    const auto [ptr,
                                error]{ std::from_chars(token.data(), token.data() + token.size(), value) };
                    params.push_back(error == std::errc{} && ptr == token.data() + token.size() ? value : 1);
                }

                if (next == std::string_view::npos) {
                    break;
                }
                cursor = next + 1;
            }

            return params;
        }

        auto parse_csi_sequence(std::string_view input) -> SequenceParseResult
        {
            const auto tail{ input.substr(2) };
            const auto terminator{ std::ranges::find_if(
              tail, [](unsigned char ch) { return ch >= '@' && ch <= '~'; }) };
            if (terminator == tail.end()) {
                return SequenceParseResult{ .event = std::nullopt, .consumed = 0, .incomplete = true };
            }

            const auto terminator_index{ static_cast<size_t>(std::distance(tail.begin(), terminator)) };
            const auto final_byte{ *terminator };
            const auto params{ parse_csi_params(tail.substr(0, terminator_index)) };

            KeyEvent event{};
            auto recognized{ true };

            switch (final_byte) {
                case 'A':
                    event.key = TerminalKey::ArrowUp;
                    break;
                case 'B':
                    event.key = TerminalKey::ArrowDown;
                    break;
                case 'C':
                    event.key = TerminalKey::ArrowRight;
                    break;
                case 'D':
                    event.key = TerminalKey::ArrowLeft;
                    break;
                case 'H':
                    event.key = TerminalKey::Home;
                    break;
                case 'F':
                    event.key = TerminalKey::End;
                    break;
                case 'Z':
                    event.key = TerminalKey::Tab;
                    event.shift = true;
                    break;
                case '~': {
                    const auto code{ params.empty() ? 0 : params.front() };
                    switch (code) {
                        case 1:
                        case 7:
                            event.key = TerminalKey::Home;
                            break;
                        case 2:
                            event.key = TerminalKey::Insert;
                            break;
                        case 3:
                            event.key = TerminalKey::Delete;
                            break;
                        case 4:
                        case 8:
                            event.key = TerminalKey::End;
                            break;
                        case 5:
                            event.key = TerminalKey::PageUp;
                            break;
                        case 6:
                            event.key = TerminalKey::PageDown;
                            break;
                        default:
                            recognized = false;
                            break;
                    }
                    break;
                }
                default:
                    recognized = false;
                    break;
            }

            if (params.size() >= 2) {
                apply_xterm_modifier(event, params[1]);
            }
            if (!recognized) {
                event.key = TerminalKey::Unknown;
            }

            return SequenceParseResult{ .event = event, .consumed = 2 + terminator_index + 1 };
        }

        auto parse_ss3_sequence(std::string_view input) -> SequenceParseResult
        {
            if (input.size() < 3) {
                return SequenceParseResult{ .event = std::nullopt, .consumed = 0, .incomplete = true };
            }

            KeyEvent event{};
            switch (input[2]) {
                case 'A':
                    event.key = TerminalKey::ArrowUp;
                    break;
                case 'B':
                    event.key = TerminalKey::ArrowDown;
                    break;
                case 'C':
                    event.key = TerminalKey::ArrowRight;
                    break;
                case 'D':
                    event.key = TerminalKey::ArrowLeft;
                    break;
                case 'H':
                    event.key = TerminalKey::Home;
                    break;
                case 'F':
                    event.key = TerminalKey::End;
                    break;
                default:
                    event.key = TerminalKey::Unknown;
                    break;
            }

            return SequenceParseResult{ .event = event, .consumed = 3 };
        }

        auto utf8_codepoint_length(unsigned char lead_byte) -> size_t
        {
            if ((lead_byte & 0x80U) == 0U) {
                return 1;
            }
            if ((lead_byte & 0xE0U) == 0xC0U) {
                return 2;
            }
            if ((lead_byte & 0xF0U) == 0xE0U) {
                return 3;
            }
            if ((lead_byte & 0xF8U) == 0xF0U) {
                return 4;
            }
            return 1;
        }

        auto parse_bare_key(std::string_view input) -> SequenceParseResult
        {
            if (input.empty()) {
                return {};
            }

            const auto byte{ static_cast<unsigned char>(input.front()) };

            if (byte == '\r' || byte == '\n') {
                return SequenceParseResult{ .event = KeyEvent{ .key = TerminalKey::Enter }, .consumed = 1 };
            }
            if (byte == '\t') {
                return SequenceParseResult{ .event = KeyEvent{ .key = TerminalKey::Tab }, .consumed = 1 };
            }
            if (byte == 0x7FU || byte == '\b') {
                return SequenceParseResult{ .event = KeyEvent{ .key = TerminalKey::Backspace },
                                            .consumed = 1 };
            }
            if (byte < 0x20U) {
                KeyEvent event{ .key = TerminalKey::Character, .ctrl = true };
                if (byte >= 0x01U && byte <= 0x1AU) {
                    event.text = std::string(1, static_cast<char>('a' + byte - 0x01U));
                }
                return SequenceParseResult{ .event = event, .consumed = 1 };
            }

            const auto codepoint_size{ utf8_codepoint_length(byte) };
            if (input.size() < codepoint_size) {
                return SequenceParseResult{ .event = std::nullopt, .consumed = 0, .incomplete = true };
            }

            return SequenceParseResult{ .event =
                                          KeyEvent{ .key = TerminalKey::Character,
                                                    .text = std::string(input.substr(0, codepoint_size)) },
                                        .consumed = codepoint_size };
        }
    } // namespace

    Terminal::Terminal()
      : m_state(std::make_unique<State>())
    {
    }

    Terminal::Terminal(const TerminalOptions& options)
      : Terminal()
    {
        setup(options);
    }

    Terminal::~Terminal()
    {
        try {
            restore();
        } catch (...) {
        }
    }

    auto Terminal::setup(const TerminalOptions& options) -> void
    {
        restore();
        m_options = options;

#if defined(_WIN32) || defined(_WIN64)
        m_state->input_handle = GetStdHandle(STD_INPUT_HANDLE);
        m_state->output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (m_state->input_handle == INVALID_HANDLE_VALUE || m_state->output_handle == INVALID_HANDLE_VALUE) {
            fail_last_error("Failed to acquire standard console handles");
        }

        DWORD input_mode{};
        if (GetConsoleMode(static_cast<HANDLE>(m_state->input_handle), &input_mode) != FALSE) {
            m_state->original_input_mode = input_mode;
            m_state->has_original_input_mode = true;
            m_state->input_is_console = true;
        }

        DWORD output_mode{};
        if (GetConsoleMode(static_cast<HANDLE>(m_state->output_handle), &output_mode) != FALSE) {
            m_state->original_output_mode = output_mode;
            m_state->has_original_output_mode = true;
            m_state->output_is_console = true;
        }

        if (m_state->output_is_console) {
            if (SetConsoleOutputCP(CP_UTF8) == FALSE || SetConsoleCP(CP_UTF8) == FALSE) {
                fail_last_error("Failed to configure console UTF-8 code pages");
            }

            auto configured_output_mode{ m_state->original_output_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING };
            if (SetConsoleMode(static_cast<HANDLE>(m_state->output_handle), configured_output_mode) ==
                FALSE) {
                fail_last_error("Failed to enable virtual terminal output");
            }
        }

        if (m_options.raw_input) {
            if (!m_state->input_is_console) {
                throw std::runtime_error("Raw terminal input requires a Windows console input handle");
            }

            auto configured_input_mode{ m_state->original_input_mode };
            configured_input_mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
            configured_input_mode |= ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS;
            if (SetConsoleMode(static_cast<HANDLE>(m_state->input_handle), configured_input_mode) == FALSE) {
                fail_last_error("Failed to enable raw console input");
            }
        }
#else
        m_state->input_is_tty = ::isatty(m_state->input_fd) == 1;
        m_state->output_is_tty = ::isatty(m_state->output_fd) == 1;

        if (m_options.raw_input) {
            if (!m_state->input_is_tty) {
                throw std::runtime_error("Raw terminal input requires an interactive TTY");
            }

            if (::tcgetattr(m_state->input_fd, &m_state->original_termios) != 0) {
                fail_errno("Failed to read terminal attributes");
            }
            m_state->has_original_termios = true;

            termios raw_mode{ m_state->original_termios };
            ::cfmakeraw(&raw_mode);
            raw_mode.c_cc[VMIN] = 0;
            raw_mode.c_cc[VTIME] = 0;
            if (::tcsetattr(m_state->input_fd, TCSAFLUSH, &raw_mode) != 0) {
                fail_errno("Failed to enable raw terminal input");
            }
        }
#endif

        m_pending_input.clear();
#if defined(_WIN32) || defined(_WIN64)
        m_state->queued_events.clear();
#endif
        m_state->cursor_hidden = false;
        m_state->alternate_screen = false;
        m_last_size = size();
        m_is_setup = true;

        if (m_options.use_alternate_screen) {
            useAlternateScreen(true);
        }
        if (m_options.hide_cursor) {
            setCursorVisible(false);
        }
        flush();
    }

    auto Terminal::restore() -> void
    {
        if (!m_is_setup) {
            return;
        }

        if (supportsAnsi()) {
            if (m_state->cursor_hidden) {
                write(ansi::show_cursor());
                m_state->cursor_hidden = false;
            }
            if (m_state->alternate_screen) {
                write(ansi::leave_alternate_screen());
                m_state->alternate_screen = false;
            }
            write(ansi::reset());
            flush();
        }

#if defined(_WIN32) || defined(_WIN64)
        if (m_state->has_original_input_mode) {
            SetConsoleMode(static_cast<HANDLE>(m_state->input_handle), m_state->original_input_mode);
        }
        if (m_state->has_original_output_mode) {
            SetConsoleMode(static_cast<HANDLE>(m_state->output_handle), m_state->original_output_mode);
        }
#else
        if (m_state->has_original_termios) {
            ::tcsetattr(m_state->input_fd, TCSAFLUSH, &m_state->original_termios);
            m_state->has_original_termios = false;
        }
#endif

        m_pending_input.clear();
        m_is_setup = false;
    }

    auto Terminal::isSetup() const -> bool { return m_is_setup; }

    auto Terminal::isInteractive() const -> bool
    {
#if defined(_WIN32) || defined(_WIN64)
        return m_state->input_is_console && m_state->output_is_console;
#else
        return m_state->input_is_tty && m_state->output_is_tty;
#endif
    }

    auto Terminal::supportsAnsi() const -> bool
    {
#if defined(_WIN32) || defined(_WIN64)
        return m_state->output_is_console;
#else
        return m_state->output_is_tty;
#endif
    }

    auto Terminal::options() const -> const TerminalOptions& { return m_options; }

    auto Terminal::size() const -> Size
    {
#if defined(_WIN32) || defined(_WIN64)
        if (m_state->output_is_console) {
            CONSOLE_SCREEN_BUFFER_INFO info{};
            if (GetConsoleScreenBufferInfo(static_cast<HANDLE>(m_state->output_handle), &info) == FALSE) {
                fail_last_error("Failed to query console size");
            }
            return Size{ .width = static_cast<uint16_t>(info.srWindow.Right - info.srWindow.Left + 1),
                         .height = static_cast<uint16_t>(info.srWindow.Bottom - info.srWindow.Top + 1) };
        }
#else
        if (m_state->output_is_tty) {
            winsize window_size{};
            if (::ioctl(m_state->output_fd, TIOCGWINSZ, &window_size) == 0) {
                return Size{ .width = window_size.ws_col, .height = window_size.ws_row };
            }
        }
#endif

        return Size{ .width = parse_env_dimension("COLUMNS"), .height = parse_env_dimension("LINES") };
    }

    auto Terminal::write(std::string_view data) -> void
    {
        if (data.empty()) {
            return;
        }

        const auto written{ std::fwrite(data.data(), sizeof(char), data.size(), stdout) };
        if (written != data.size()) {
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
        if (!supportsAnsi()) {
            return;
        }

        std::string output{ ansi::clear_screen() };
        ansi::append_move_cursor(output, 0, 0);
        write(output);
    }

    auto Terminal::setCursorVisible(bool visible) -> void
    {
        if (!supportsAnsi()) {
            return;
        }

        if (visible && m_state->cursor_hidden) {
            write(ansi::show_cursor());
            m_state->cursor_hidden = false;
        }
        else if (!visible && !m_state->cursor_hidden) {
            write(ansi::hide_cursor());
            m_state->cursor_hidden = true;
        }
    }

    auto Terminal::useAlternateScreen(bool enabled) -> void
    {
        if (!supportsAnsi()) {
            return;
        }

        if (enabled && !m_state->alternate_screen) {
            write(ansi::enter_alternate_screen());
            m_state->alternate_screen = true;
        }
        else if (!enabled && m_state->alternate_screen) {
            write(ansi::leave_alternate_screen());
            m_state->alternate_screen = false;
        }
    }

    auto Terminal::takeResizeEvent() -> std::optional<ResizeEvent>
    {
        const auto current_size{ size() };
        if (current_size != m_last_size) {
            m_last_size = current_size;
            return ResizeEvent{ .size = current_size };
        }
        return std::nullopt;
    }

    auto Terminal::parsePendingEvent() -> std::optional<KeyEvent>
    {
        if (m_pending_input.empty()) {
            return std::nullopt;
        }

        SequenceParseResult result{};
        const auto input{ std::string_view(m_pending_input) };

        if (input.front() == '\x1b') {
            if (input.size() >= 2 && input[1] == '[') {
                result = parse_csi_sequence(input);
            }
            else if (input.size() >= 2 && input[1] == 'O') {
                result = parse_ss3_sequence(input);
            }
            else if (input.size() == 1) {
                result =
                  SequenceParseResult{ .event = KeyEvent{ .key = TerminalKey::Escape }, .consumed = 1 };
            }
            else {
                auto nested{ parse_bare_key(input.substr(1)) };
                if (nested.incomplete) {
                    return std::nullopt;
                }
                if (nested.event.has_value()) {
                    nested.event->alt = true;
                    nested.consumed += 1;
                    result = std::move(nested);
                }
                else {
                    result =
                      SequenceParseResult{ .event = KeyEvent{ .key = TerminalKey::Escape }, .consumed = 1 };
                }
            }
        }
        else {
            result = parse_bare_key(input);
        }

        if (result.incomplete || !result.event.has_value() || result.consumed == 0) {
            return std::nullopt;
        }

        m_pending_input.erase(0, result.consumed);
        return result.event;
    }

    auto Terminal::readAvailableInput(std::optional<std::chrono::milliseconds> timeout) -> bool
    {
#if defined(_WIN32) || defined(_WIN64)
        static_cast<void>(timeout);
        return false;
#else
        if (!m_options.raw_input || !m_state->input_is_tty) {
            return false;
        }

        pollfd descriptor{ .fd = m_state->input_fd, .events = POLLIN, .revents = 0 };
        const auto timeout_ms{ timeout.has_value()
                                 ? static_cast<int>(std::clamp<long long>(
                                     static_cast<long long>(timeout->count()),
                                     0LL,
                                     static_cast<long long>(std::numeric_limits<int>::max())))
                                 : -1 };

        int poll_result{};
        do {
            poll_result = ::poll(&descriptor, 1, timeout_ms);
        } while (poll_result < 0 && errno == EINTR);

        if (poll_result == 0) {
            return false;
        }
        if (poll_result < 0) {
            fail_errno("Failed while waiting for terminal input");
        }

        std::array<char, 64> buffer{};
        auto read_any{ false };
        while (true) {
            const auto bytes_read{ ::read(m_state->input_fd, buffer.data(), buffer.size()) };
            if (bytes_read > 0) {
                m_pending_input.append(buffer.data(), static_cast<size_t>(bytes_read));
                read_any = true;
                continue;
            }
            if (bytes_read == 0) {
                break;
            }
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            fail_errno("Failed while reading terminal input");
        }

        return read_any;
#endif
    }

    auto Terminal::pollEvent(std::chrono::milliseconds timeout) -> std::optional<TerminalEvent>
    {
        if (auto resize{ takeResizeEvent() }) {
            return resize;
        }
        if (!m_options.raw_input) {
            return std::nullopt;
        }

#if defined(_WIN32) || defined(_WIN64)
        if (!m_state->input_is_console) {
            return std::nullopt;
        }
        if (!m_state->queued_events.empty()) {
            auto event{ m_state->queued_events.front() };
            m_state->queued_events.pop_front();
            return event;
        }

        const auto wait_indefinitely{ timeout == std::chrono::milliseconds::max() };
        const auto deadline{ wait_indefinitely ? std::optional<std::chrono::steady_clock::time_point>{}
                                               : std::optional(std::chrono::steady_clock::now() + timeout) };

        while (true) {
            const auto timeout_ms{ wait_indefinitely
                                     ? INFINITE
                                     : static_cast<DWORD>(std::clamp(
                                         std::chrono::duration_cast<std::chrono::milliseconds>(
                                           deadline.value() - std::chrono::steady_clock::now())
                                           .count(),
                                         0LL,
                                         static_cast<long long>(std::numeric_limits<DWORD>::max()))) };

            const auto wait_result{ WaitForSingleObject(static_cast<HANDLE>(m_state->input_handle),
                                                        timeout_ms) };
            if (wait_result == WAIT_TIMEOUT) {
                return takeResizeEvent();
            }
            if (wait_result != WAIT_OBJECT_0) {
                fail_last_error("Failed while waiting for console input");
            }

            INPUT_RECORD record{};
            DWORD read_count{};
            if (ReadConsoleInputW(static_cast<HANDLE>(m_state->input_handle), &record, 1, &read_count) ==
                FALSE) {
                fail_last_error("Failed while reading console input");
            }
            if (read_count == 0) {
                return takeResizeEvent();
            }

            if (record.EventType == WINDOW_BUFFER_SIZE_EVENT) {
                const auto current_size{ size() };
                m_last_size = current_size;
                return ResizeEvent{ .size = current_size };
            }
            if (record.EventType != KEY_EVENT) {
                continue;
            }

            const auto event{ map_console_key(record.Event.KeyEvent) };
            if (!event.has_value()) {
                continue;
            }

            const auto repeat_count{ std::max<WORD>(1, record.Event.KeyEvent.wRepeatCount) };
            for (WORD repeat{ 1 }; repeat < repeat_count; ++repeat) {
                m_state->queued_events.push_back(*event);
            }
            return *event;
        }
#else
        if (!m_state->input_is_tty) {
            return std::nullopt;
        }
        if (auto event{ parsePendingEvent() }) {
            return *event;
        }
        if (!readAvailableInput(timeout)) {
            return takeResizeEvent();
        }
        if (auto event{ parsePendingEvent() }) {
            return *event;
        }
        return takeResizeEvent();
#endif
    }

    auto Terminal::readEvent() -> std::optional<TerminalEvent>
    {
        return pollEvent(std::chrono::milliseconds::max());
    }
}