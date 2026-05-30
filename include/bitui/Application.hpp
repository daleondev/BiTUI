#pragma once

#include <chrono>

#include "bitui/Style.hpp"
#include "bitui/Terminal.hpp"

namespace bitui
{
    using namespace std::chrono_literals;

    class Application
    {
      public:
        struct Options
        {
            std::chrono::steady_clock::duration tick_rate{ 10ms };
            Style clear_style{};
        };

        explicit Application(Options options = Options{ .tick_rate = 10ms, .clear_style = Style{} });

        auto run() -> int;

      private:
        Options m_options;
    };

} // namespace cellui
