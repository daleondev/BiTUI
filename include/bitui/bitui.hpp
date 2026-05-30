#pragma once

#include "Color.hpp"
#include "Renderer.hpp"
#include "ScreenBuffer.hpp"
#include "Style.hpp"
#include "Terminal.hpp"
#include "ansi.hpp"
#include "geometry.hpp"

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace bitui::detail
{
    [[maybe_unused]]
    inline const auto CONFIGURED_TERMINAL_HANDLE{ [] {
        const auto fail{ [] {
            std::fputs("Error: Failed to configure terminal\n", stderr);
            std::fflush(stderr);
            std::abort();
        } };

        HANDLE hOut{ GetStdHandle(STD_OUTPUT_HANDLE) };
        if (hOut == INVALID_HANDLE_VALUE || !hOut) {
            fail();
        }

        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

        DWORD dwMode{};
        if (GetConsoleMode(hOut, &dwMode) == FALSE) {
            fail();
        }

        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (SetConsoleMode(hOut, dwMode) == FALSE) {
            fail();
        }

        return hOut;
    }() };
}
#endif