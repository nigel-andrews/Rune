#pragma once

namespace Rune
{
    class RenderBackend
    {
    public:
        virtual ~RenderBackend() = default;
        virtual void init() = 0;
        virtual void draw_frame() = 0;
        virtual void cleanup() = 0;
    };
} // namespace Rune
