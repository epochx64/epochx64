#include "window.h"

namespace GUI
{
    void DrawLine(math::Vec2<UINT64> Pos1, math::Vec2<UINT64> Pos2, graphics::Color color)
    {
        using namespace math;
        using namespace graphics;

        double Slope = (double)(Pos2.Y - Pos1.Y)/(double)(Pos2.X - Pos1.X);

        for(auto vPixel = Pos1; vPixel.X < Pos2.X; vPixel.X++)
        {
            vPixel.Y = Slope*(double)(vPixel.X - Pos1.X) + Pos1.Y;
            PutPixel(vPixel.X, vPixel.Y, &(keSysDescriptor->gopInfo), color);
        }
    }

    void DrawRectangle(math::Vec2<UINT64> Pos1, math::Vec2<UINT64> Pos2, graphics::Color color)
    {
        using namespace math;
        using namespace graphics;

        for(Vec2<UINT64> Pos = Pos1; Pos.X < Pos2.X; Pos.X++)
        {
            PutPixel(Pos.X, Pos1.Y, &(keSysDescriptor->gopInfo), color);
            PutPixel(Pos.X, Pos2.Y, &(keSysDescriptor->gopInfo), color);
        }

        for(Vec2<UINT64> Pos = Pos1; Pos.Y < Pos2.Y; Pos.Y++)
        {
            PutPixel(Pos1.X, Pos.Y, &(keSysDescriptor->gopInfo), color);
            PutPixel(Pos2.X, Pos.Y, &(keSysDescriptor->gopInfo), color);
        }
    }

    Window::Window(UINT64 x, UINT64 y, UINT64 height, UINT64 width)
    {
        X = x;
        Y = y;
        Height = height;
        Width = width;

        cout.pFramebufferInfo = new FRAMEBUFFER_INFO;
        cout.pFramebufferInfo->Height = Height;
        cout.pFramebufferInfo->Width = Width;
        cout.pFramebufferInfo->Pitch = keSysDescriptor->gopInfo.Pitch;
        cout.pFramebufferInfo->pFrameBuffer = (
                keSysDescriptor->gopInfo.pFrameBuffer
                + X*FRAMEBUFFER_BYTES_PER_PIXEL
                + Y*cout.pFramebufferInfo->Pitch
                );
    }

    void Window::Draw()
    {
        using namespace math;
        using namespace graphics;

        DrawRectangle(
                Vec2<UINT64>(X-1, Y-1), Vec2<UINT64>(X+1 + Width, Y+1 + Height), COLOR_WHITE
                );
    }
}