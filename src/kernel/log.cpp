#include <log.h>

namespace log
{
    //dbgout::dbgout() { NumberBase = HEX_ID; }
    krnlout::krnlout()
    {
        NumberBase = HEX_ID;
        k_tty_lin = 0;
        k_tty_col = 0;
    }

    krnlout kout;
    //dbgout dout;

    void krnlout::SetPosition(TTY_COORD COORD)
    {
        k_tty_col = COORD.Col;
        k_tty_lin = COORD.Lin;
    }

    TTY_COORD krnlout::GetPosition()
    {
        TTY_COORD COORD;
        COORD.Col = k_tty_col;
        COORD.Lin = k_tty_lin;

        return COORD;
    }

    void krnlout::StepForward(UINT64 Amount)
    {
        for(UINT64 i = 0; i < Amount; i++) PutChar(' ');
    }

    void krnlout::PutChar(char c)
    {
        using namespace graphics;

        switch (c)
        {
            case '\n':
                //  TODO: This should have a scroll effect,
                //  but will inevitably be replaced by a TTY
                k_tty_col = 0;

                //  We don't want to go out of the screen
                k_tty_lin = (k_tty_lin + 1) % (pFramebufferInfo->Height/16 - 1);
                return;
            case '\r':
                return;
            case '\t':
                StepForward(4);
                return;
            case STEPUP_CHAR:
                if(k_tty_lin > 0) k_tty_lin--;
                return;
            case HEX_CHAR:
                NumberBase = 0;
                return;
            case DEC_CHAR:
                NumberBase = 1;
                return;
            case DOUBLE_DEC_CHAR:
                NumberBase = 2;
                return;
        }

        auto pixel_address = (uint64_t)(pFramebufferInfo->pFrameBuffer + 16*k_tty_lin*(pFramebufferInfo->Pitch) + k_tty_col*4*8);

        for(size_t i = 0; i < 16; i++)
        {
            uint8_t and_mask    = 0b10000000;
            auto font_rom_byte  = (uint8_t)(font_rom::buffer[ ((size_t)c)*16 + i ]);

            //  Left to right, fill in pixel
            //  row based on font_rom bits
            for(size_t j = 0; j < 8; j++)
            {
                bool isForeground = (bool) ( font_rom_byte & and_mask );
                Color pixel_color = isForeground? COLOR_WHITE : COLOR_BLACK;
                and_mask >>= (unsigned)1;

                *((uint32_t*)(pixel_address + 4*j)) = pixel_color.To_VBE_Word();
            }

            pixel_address += pFramebufferInfo->Pitch;
        }

        if(++k_tty_col >= pFramebufferInfo->Width / 8)
        {
            k_tty_col = 0;
            k_tty_lin = (k_tty_lin + 1) % (pFramebufferInfo->Height/16 - 1);
        }
    }

//    void dbgout::PutChar(char c)
//    {
//        switch (c)
//        {
//            case HEX_CHAR:
//                NumberBase = 0;
//                return;
//
//            case DEC_CHAR:
//                NumberBase = 1;
//                return;
//        }
//
//#ifdef KERNEL_H
//        outb(COM1, c);
//#endif
//    }
}
