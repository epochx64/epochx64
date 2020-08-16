#include "kernel.h"

uint64_t u64Heap_Addr;
uint64_t u64Heap_Size;
graphics::frame_buf_attr_t _Frame_Buffer_Info;

namespace kernel
{
    static uint64_t u64PML4_Addr;
    static uint64_t u64Multiboot2_Info_Addr;

    static uint64_t u64Memory_Size;

    static io::dbgout dout;

    void
    process_multiboot()
    {
        multiboot_tag *tag;

        for
        (
                tag = (multiboot_tag*)(u64Multiboot2_Info_Addr + 8);
                tag->type != MULTIBOOT_TAG_TYPE_END;
                tag = (multiboot_tag*)( (uint8_t*)tag + ((tag->size + 7) & ~7) )
        )
        {
            switch ( tag->type )
            {
                case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
                    {
                        auto memtag = (multiboot_tag_basic_meminfo*)tag;
                        u64Memory_Size = (memtag->mem_lower + memtag->mem_upper)*0x400;

                        dout << "Detected memory: " << io::to_int(u64Memory_Size/0x100000) << " MiB\n";
                        break;
                    }

                case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
                    {
                        auto fbtag = (multiboot_tag_framebuffer*)tag;

                        _Frame_Buffer_Info.u64Address   = fbtag->common.framebuffer_addr;
                        _Frame_Buffer_Info.u32Height    = fbtag->common.framebuffer_height;
                        _Frame_Buffer_Info.u32Width     = fbtag->common.framebuffer_width;
                        _Frame_Buffer_Info.u32Pitch     = fbtag->common.framebuffer_pitch;
                        _Frame_Buffer_Info.u32Size      = fbtag->common.size;

                        //  to_hex/int without delete[] creates memory leaks FYI
                        //  i'm lazy and need debug messages tho
                        dout << "VESA framebuffer address: 0x" << io::to_hex(_Frame_Buffer_Info.u64Address) << "\n";
                        //dout << "VESA framebuffer size: " << io::to_int()
                        break;
                    }

            }
        }
    }

    void
    init_vesa()
    {
        using namespace paging;

        for (uint16_t i = 0; i < 1420; i++)
        {
            identity_map(_Frame_Buffer_Info.u64Address + (uint64_t)i*0x1000, PRESENT | WRITE | USER);
        }
    }
}

/*
 * Temporary
 */
bool gfxroutine(graphics::Color *c)
{
    using namespace graphics;
    using namespace math;

    double divisions = 2400.0;
    double dtheta = 2*PI / divisions;
    double theta = 0.0;

    for(int i = 0; i < (int)divisions; i++ )
    {
        for(int j = 0; j < 8; j++)
        {
            put_pixel(800 + (200+j)*cos(theta), 450+(80+j)*sin(theta), &_Frame_Buffer_Info, *c);
            put_pixel(800 + (80+j)*cos(theta), 450+(200+j)*sin(theta), &_Frame_Buffer_Info, *c);
            put_pixel(800 + (80-8+j/2)*cos(theta), 450+(80-8+j/2)*sin(theta), &_Frame_Buffer_Info, *c);
        }

        c->u8_R += 1;

        theta += dtheta;
    }

    c->u8_G += 2;
    c->u8_B += 1;

    return true;
}

void
kernel_main (
        uint64_t    u64multiboot2_info_addr,
        uint64_t    u64pml4_addr,
        uint64_t    u64heap_size,
        uint64_t    u64heap_addr
)
{
    using namespace kernel;
    using namespace paging;
    using namespace graphics;

    dout << "- - - - - - - - Init - - - - - - - - -\n";

    u64PML4_Addr            = u64pml4_addr;
    u64Multiboot2_Info_Addr = u64multiboot2_info_addr;
    u64Heap_Addr            = u64heap_addr;
    u64Heap_Size            = u64heap_size;

    process_multiboot();
    init_vesa();

    dout << "Heap address: 0x"      << io::to_hex(u64heap_addr)                     << "\n";
    dout << "Heap_end address: 0x"  << io::to_hex(u64heap_addr + u64heap_size)  << "\n";

    Color c1(128, 255, 0, 0);
    Color c2(0, 0, 0, 0);

    log::kprint("A", c2, c1);
    while(gfxroutine(&c1));
}
