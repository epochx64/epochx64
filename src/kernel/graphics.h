<<<<<<< HEAD
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <defs/int.h>
#include <kernel/typedef.h>

namespace graphics
{
    #define COLOR_WHITE graphics::Color(255, 255, 255, 0)
    #define COLOR_BLACK graphics::Color(0,0,0,0)
=======
//
// Created by Beta on 2020-07-19.
//

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

namespace graphics
{
    typedef struct
    {
        uint64_t    u64Address;
        uint32_t    u32Height;
        uint32_t    u32Width;
        uint32_t    u32Pitch;
        uint32_t    u32Size;
    }__attribute((packed))
    frame_buf_attr_t;
>>>>>>> 3ba874f320446bb2a68008015348e10f7f24c408

    class Color
    {
    public:
        Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

<<<<<<< HEAD
        uint32_t FORCE_INLINE To_VBE_Word()
        {
            uint32_t b32 = u8_A << 24;
            uint32_t b24 = u8_R << 16;
            uint32_t b16 = u8_G << 8;
            uint32_t b8  = u8_B << 0;

            return b32 | b24 | b16 | b8;
        }
=======
        uint32_t To_VBE_Word();
>>>>>>> 3ba874f320446bb2a68008015348e10f7f24c408

        uint8_t u8_R;
        uint8_t u8_G;
        uint8_t u8_B;
        uint8_t u8_A;
    };

    class Window
    {

    };

<<<<<<< HEAD
    static void FORCE_INLINE PutPixel (
            uint32_t                    x,
            uint32_t                    y,
            kernel::FRAMEBUFFER_INFO   *FrameBufferInfo,
            Color                       color
    )
    {
        ((uint32_t*)(FrameBufferInfo->pFrameBuffer))[x + ((FrameBufferInfo->Pitch)/4)*y] = color.To_VBE_Word();
=======
    inline uint32_t Color::To_VBE_Word()
    {
        uint32_t b32 = u8_A << 24;
        uint32_t b24 = u8_R << 16;
        uint32_t b16 = u8_G << 8;
        uint32_t b8  = u8_B << 0;

        return b32 | b24 | b16 | b8;
    }

    inline void put_pixel (
            uint32_t            x,
            uint32_t            y,
            frame_buf_attr_t    *frame_buf_attributes,
            Color               color
    )
    {
        auto fba = frame_buf_attributes;
        ((uint32_t*)(fba->u64Address))[x + ((fba->u32Pitch)/4)*y] = color.To_VBE_Word();
>>>>>>> 3ba874f320446bb2a68008015348e10f7f24c408
    }
}

#endif
