#include "kernel.h"

<<<<<<< HEAD
namespace kernel
{
    //  TODO:   make these pointers
    KERNEL_BOOT_INFO KernelBootInfo;
    KERNEL_DESCRIPTOR KernelDescriptor;
}

/*
 * Temporary
 */
void gfxroutine(scheduler::TASK_ARG *TaskArgs)
{
    double x = *((double*)TaskArgs[0]);
    double y = *((double*)TaskArgs[1]);
    int radius = *((int*)TaskArgs[2]);

    graphics::Color c(128, 192, 128, 0);

    while (true)
    {
        using namespace graphics;
        using namespace kernel;
        using namespace math;

        double divisions = 2400.0;
        double dtheta = 2 * PI / divisions;

        //  Draw a fat circle
        for (double theta = 0.0; theta < 2*PI; theta += dtheta) {
            for (int j = 0; j < radius; j++)
                PutPixel(x + j*cos(theta), y + j*sin(theta), &(KernelBootInfo.FramebufferInfo), c);
        }

        c.u8_R += 10;
        c.u8_G += 20;
        c.u8_B += 10;
    }
}

void KernelMain(kernel::KERNEL_BOOT_INFO KernelInfo)
{
    using namespace kernel;

    /*
     * Setup IDT, SSE, PIT, and PS2
     */
    KernelBootInfo = KernelInfo;
    {
        using namespace interrupt;
        using namespace ASMx64;
        using namespace PIC;

        EnableSSE();

        cli();
        FillIDT64();
        InitPS2();
        InitPIT();
    }

    /*
     * Setup a scheduler before the APIC timer is enabled
     * and add a couple of tasks
     */
    using namespace scheduler;
    {
        Scheduler0 = new scheduler::Scheduler(true, 0);
        Scheduler0->AddTask(new Task(
                (UINT64)&gfxroutine,
                true,
                new TASK_ARG[3] {
                    (TASK_ARG)(new double(200.0)),
                    (TASK_ARG)(new double(200.0)),
                    (TASK_ARG)(new int(100))
                }
                ));

        Scheduler0->AddTask(new Task(
                (UINT64)&gfxroutine,
                true,
                new TASK_ARG[3] {
                    (TASK_ARG)(new double(900.0)),
                    (TASK_ARG)(new double(300.0)),
                    (TASK_ARG)(new int(130))
                }
        ));
    }

    /*
     *  So far this only initializes APIC timer
     */
    ACPI::InitACPI(&KernelInfo, &(KernelDescriptor.KernelACPIInfo));

    ASMx64::sti();

    /*
     * No code runs beyond here
     * This is now an idle task
     */
    ASMx64::hlt();

    /*
     * Test graphics routine (draws a circle)
     * TODO:   Double buffering
     */
    gfxroutine(new scheduler::TASK_ARG[3] {
        (TASK_ARG)(new double(500.0)),
        (TASK_ARG)(new double(500.0)),
        (TASK_ARG)(new int(100))
    });
}

=======
uint64_t u64Heap_Addr;
uint64_t u64Heap_Size;
graphics::frame_buf_attr_t _Frame_Buffer_Info;

namespace kernel
{
    static uint64_t u64PML4_Addr;
    static uint64_t u64Multiboot2_Info_Addr;

    static mem_info_t mem_info;

    static dbgout dout;
    static kout krnlout;

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
                        mem_info.size = (memtag->mem_lower + memtag->mem_upper)*0x400;

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

    dbgout::dbgout()
    {

    }

    dbgout &dbgout::operator<<(char *str)
    {
        log::write(str);
        return *this;
    }

    kout::kout()
    {

    }

    //  K_tty = kernel_tty
    static uint16_t k_tty_col = 0;
    static uint16_t k_tty_lin = 0;

    void
    k_puchar(char c)
    {
        using namespace graphics;

        if (c == '\n')
        {
            k_tty_col = 0;
            k_tty_lin++;
            return;
        }

        auto pixel_address = (uint64_t)(_Frame_Buffer_Info.u64Address + k_tty_lin*_Frame_Buffer_Info.u32Pitch*16 + k_tty_col*4*8);

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

                *((uint32_t*)(pixel_address + 4*j)) =
                        (pixel_color.u8_A << 24)
                        | (pixel_color.u8_R << 16)
                        | (pixel_color.u8_G << 8)
                        | (pixel_color.u8_B << 0);
            }

            pixel_address += _Frame_Buffer_Info.u32Pitch;
        }

        k_tty_col++;
    }

    kout
    &kout::operator<<(char *str)
    {
        uint64_t len = io::strlen(str);

        for(uint64_t i = 0; i < len; i++) k_puchar(*(str + i));

        return *this;
    }

    void
    print_logo()
    {
        krnlout <<
                "|        \\|       \\  /      \\  /      \\ |  \\  |  \\|  \\  |  \\ /      \\ |  \\  |  \\\n"
                "| $$$$$$$$| $$$$$$$\\|  $$$$$$\\|  $$$$$$\\| $$  | $$| $$  | $$|  $$$$$$\\| $$  | $$\n"
                "| $$__    | $$__/ $$| $$  | $$| $$   \\$$| $$__| $$ \\$$\\/  $$| $$___\\$$| $$__| $$\n"
                "| $$  \\   | $$    $$| $$  | $$| $$      | $$    $$  >$$  $$ | $$    \\ | $$    $$\n"
                "| $$$$$   | $$$$$$$ | $$  | $$| $$   __ | $$$$$$$$ /  $$$$\\ | $$$$$$$\\ \\$$$$$$$$\n"
                "| $$_____ | $$      | $$__/ $$| $$__/  \\| $$  | $$|  $$ \\$$\\| $$__/ $$      | $$\n"
                "| $$     \\| $$       \\$$    $$ \\$$    $$| $$  | $$| $$  | $$ \\$$    $$      | $$\n"
                " \\$$$$$$$$ \\$$        \\$$$$$$   \\$$$$$$  \\$$   \\$$ \\$$   \\$$  \\$$$$$$        \\$$\n\n";
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
            put_pixel(800 + (200+j*j*j)*cos(theta), 450+(80+j*j)*sin(theta), &_Frame_Buffer_Info, *c);
            put_pixel(800 + (80+j)*cos(theta), 450+(200+j)*sin(theta), &_Frame_Buffer_Info, *c);
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

    dout << "-_-_-_-_- Init -_-_-_-_-\n";
    //  TODO: Print kernel size

    u64PML4_Addr            = u64pml4_addr;
    u64Multiboot2_Info_Addr = u64multiboot2_info_addr;
    u64Heap_Addr            = u64heap_addr;
    u64Heap_Size            = u64heap_size;

    process_multiboot();
    init_vesa();

    print_logo();

    //  TODO: to_hex/int without delete[] creates memory leaks FYI
    //  i'm lazy and need debug messages tho
    krnlout << "Detected memory: " << io::to_int(mem_info.size/0x100000) << " MiB\n";
    krnlout << "Heap address: 0x"      << io::to_hex(u64heap_addr)                     << "\n";
    krnlout << "Heap_end address: 0x"  << io::to_hex(u64heap_addr + u64heap_size)  << "\n";

    Color c(128, 255, 0, 0);
    while(gfxroutine(&c));
}
>>>>>>> 3ba874f320446bb2a68008015348e10f7f24c408
