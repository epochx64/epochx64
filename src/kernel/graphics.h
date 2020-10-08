#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <defs/int.h>
#include <kernel/typedef.h>

namespace graphics
{
    #define COLOR_WHITE graphics::Color(255, 255, 255, 0)
    #define COLOR_BLACK graphics::Color(0,0,0,0)

    class Color
    {
    public:
        Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

        uint32_t FORCE_INLINE To_VBE_Word()
        {
            uint32_t b32 = u8_A << 24;
            uint32_t b24 = u8_R << 16;
            uint32_t b16 = u8_G << 8;
            uint32_t b8  = u8_B << 0;

            return b32 | b24 | b16 | b8;
        }

        uint8_t u8_R;
        uint8_t u8_G;
        uint8_t u8_B;
        uint8_t u8_A;
    };

    class Window
    {

    };

    static void FORCE_INLINE PutPixel (
            uint32_t                    x,
            uint32_t                    y,
            kernel::FRAMEBUFFER_INFO   *FrameBufferInfo,
            Color                       color
    )
    {
        ((uint32_t*)(FrameBufferInfo->pFrameBuffer))[x + ((FrameBufferInfo->Pitch)/4)*y] = color.To_VBE_Word();
    }
}

#endif
