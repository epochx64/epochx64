#ifndef _WINDOW_H
#define _WINDOW_H

#include <kernel/typedef.h>
#include <kernel/log.h>
#include <mem.h>
#include <lib/math/math.h>
#include <kernel/graphics.h>

namespace GUI
{
    void DrawLine(math::Vec2<UINT64> Pos1, math::Vec2<UINT64> Pos2, graphics::Color color);
    void DrawRectangle(math::Vec2<UINT64> Pos1, math::Vec2<UINT64> Pos2, graphics::Color color);

    class Window
    {
    public:
        Window(UINT64 x, UINT64 y, UINT64 height, UINT64 width);

        void Draw();

        UINT64 X;
        UINT64 Y;

        UINT64 Height;
        UINT64 Width;

        log::krnlout cout;

        UINT64 pFrameBuffer;
    };
}

#endif
