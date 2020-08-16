//
// Created by Beta on 2020-07-18.
//

#include "log.h"

extern graphics::frame_buf_attr_t _Frame_Buffer_Info;

namespace log
{
    uint64_t write(char *str)
    {
        uint64_t len = io::strlen(str);

        using namespace x86_64_asm;
        for ( uint64_t i = 0; i < len; i++ ) asm_outb(COM1, str[i]);

        return len;
    }

    void kprint(char *str, graphics::Color background, graphics::Color foreground)
    {
        using namespace graphics;

        for(size_t i = 0; i < 16; i++)
        {
            uint8_t and_mask = 0b00000001;
            uint8_t font_rom_byte = (uint8_t)(font_rom::buffer[ ((size_t)*str)*16 + i ]);

            //  Left to right, fill in pixel
            //  row based on font_rom bits
            for(size_t j = 0; j < 8; j++)
            {
                bool isForeground = (bool) ( (font_rom_byte >> (7-j)) & and_mask );

                Color pixel_color = (isForeground) ? foreground : background;

                put_pixel(800 + j, 450 + i, &_Frame_Buffer_Info, pixel_color);
            }
        }
    }

    //TODO: Write a function that automatically does newline and prints time, importance level, yaknow, a proper logger
}
