//
// Created by Beta on 2020-07-19.
//

#include "graphics.h"

namespace graphics
{
    Color::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        u8_R = r;
        u8_G = g;
        u8_B = b;
        u8_A = a;
    }

    uint32_t Color::To_VBE_Word()
    {
        uint32_t b32 = u8_A << 24;
        uint32_t b24 = u8_R << 16;
        uint32_t b16 = u8_G << 8;
        uint32_t b8  = u8_B << 0;

        return b32 | b24 | b16 | b8;
    }

    void put_pixel (
            uint32_t            x,
            uint32_t            y,
            frame_buf_attr_t    *frame_buf_attributes,
            Color               color
    )
    {
        auto fba = frame_buf_attributes;
        ((uint32_t*)(fba->u64Address))[x + ((fba->u32Pitch)/4)*y] = color.To_VBE_Word();
    }
}