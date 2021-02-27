#include "pic.h"

namespace interrupt
{
    extern double msPerTick;
}

UINT8 PS2Read()
{
    using namespace ASMx64;
    while(!(inb(0x64) & 1));

    return inb(0x60);
}

void PS2Write(UINT8 Port, UINT8 Value)
{
    using namespace ASMx64;

    if(Port == 2)
    {
        while(inb(0x64) & 2);
        outb(PS2_COMMAND_PORT, 0xD4);
    }

    while(inb(0x64) & 2);
    outb(0x60, Value);
}

void InitPS2()
{
    using namespace ASMx64;
    using namespace log;

    /*  TODO:   This is UGLY, make some helper functions
     *  TODO:   Implement timeout when polling devices
     */

    /*
     * Disable keyboard and mouse
     */
    while(inb(0x64) & 2);
    outb(PS2_COMMAND_PORT, 0xAD);

    while(inb(0x64) & 2);
    outb(PS2_COMMAND_PORT, 0xA7);

    //  Flush output buffer
    while(inb(0x64) & 1) inb(0x60);

    /*
     * Set configuration byte
     */
    {
        while(inb(PS2_COMMAND_PORT) & 2);
        outb(PS2_COMMAND_PORT, 0x20);

        while(!(inb(PS2_COMMAND_PORT) & 1));
        UINT8 result = (inb(0x60) & (~0b00100000)) | 0b00000010;

        while(inb(PS2_COMMAND_PORT) & 2);
        outb(PS2_COMMAND_PORT, 0x60);

        while(inb(PS2_COMMAND_PORT) & 2);
        PS2Write(1, result);
    }

    /*
     * Perform self tests
     */
    {
        while(inb(0x64) & 2);
        outb(PS2_COMMAND_PORT, 0xAA);
        if(PS2Read() != 0x55) kout << "Self test FAIL\n";

        while(inb(0x64) & 2);
        outb(PS2_COMMAND_PORT, 0xAB);
        if(PS2Read()) kout << "Port 1 test FAIL\n";

        while(inb(0x64) & 2);
        outb(PS2_COMMAND_PORT, 0xA9);
        if(PS2Read()) kout << "Port 2 test FAIL\n";
    }

    /*
     * Enable PS/2 Mouse
     */
    {
        //  Enable mouse
        while(inb(PS2_COMMAND_PORT) & 2);
        outb(PS2_COMMAND_PORT, 0xA8);

        //  Configure mouse
        PS2Write(2, 0xF6);
        PS2Read();

        PS2Write(2, 0xF4);
        PS2Read();
    }

    //  Enable Keyboard
    while(inb(0x64) & 2);
    outb(PS2_COMMAND_PORT, 0xAE);
}

void InitPIT()
{
    using namespace ASMx64;

    //  Initialization of PIC
    {
        outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
        outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);

        outb(PIC1_DATA, 0x20);
        outb(PIC2_DATA, 0x28);

        outb(PIC1_DATA, 4);
        outb(PIC2_DATA, 2);

        outb(PIC1_DATA, ICW4_8086);
        outb(PIC2_DATA, ICW4_8086);

        /*
         * Mask all IRQs
         */
        outb(PIC1_DATA, 0b11111111);
        outb(PIC2_DATA, 0b11111111);
    }

    //  Reload can be whatever, might affect performance
    UINT16 Reload = 0x1000;
    interrupt::msPerTick = ((double)Reload*1000)/PIC_FREQUENCY;

    //  Send the reload value to the PIT
    outb(0x40, (UINT8)Reload);
    outb(0x40, (UINT8)(Reload >> 8));

    //  Unmask IRQ 0, 1, 2, 7
    outb(PIC1_DATA, 0b01111001);

    //  Unmask IRQ 12
    outb(PIC2_DATA, 0b11101101);
}
