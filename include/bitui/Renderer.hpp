#pragma once

#include "bitui/ScreenBuffer.hpp"

#include <optional>

namespace bitui
{
    class Renderer
    {
      public:
        auto render(const ScreenBuffer& buffer) -> std::string;
        auto reset() -> void;
        auto previous_buffer() const -> const ScreenBuffer&;

      private:
        std::optional<ScreenBuffer> m_previous;
    };

}