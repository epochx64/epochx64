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

    if(Port == 2) outb(PS2_COMMAND_PORT, 0xD4);

    while(inb(0x64) & 2);
    outb(0x60, Value);
}

void InitPS2()
{
    using namespace ASMx64;
    using namespace log;

    //  TODO:   This is UGLY, make some helper functions

    //  Disable keyboard
    while(inb(0x64) & 2);
    outb(PS2_COMMAND_PORT, 0xAD);

    //  Keyboard stuff
    {
        //  Flush output buffer
        while(inb(0x64) & 1) inb(0x60);

        //  Read the config
        while(inb(0x64) & 2);
        outb(PS2_COMMAND_PORT, (0x20|(0&0x1F)));
        UINT8 Config = PS2Read();

        while(inb(0x64) & 2);
        outb(PS2_COMMAND_PORT, 0xAA);
        if(PS2Read() != 0x55) kout << "Self test FAIL\n";

        //  Test the ports
        while(inb(0x64) & 2);
        outb(PS2_COMMAND_PORT, 0xAB);
        if(PS2Read()) kout << "Port 1 test FAIL\n";

        while(inb(0x64) & 2);
        outb(PS2_COMMAND_PORT, 0xA9);
        if(PS2Read()) kout << "Port 2 test FAIL\n";

        //  Write the config
        while(inb(0x64) & 2);
        outb(PS2_COMMAND_PORT, (0x60|(0&0x1F)));

        Config = (Config | 0b01000011) & (~0b00110000);
        PS2Write(1, Config);

        //  Enable Keyboard
        while(inb(0x64) & 2);
        outb(PS2_COMMAND_PORT, 0xAE);
    }
}

void InitPIT()
{
    using namespace ASMx64;

    //  Initialization of PIC
    {
        outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
        IOWait();
        outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
        IOWait();

        outb(PIC1_DATA, 0x20);
        IOWait();
        outb(PIC2_DATA, 0x28);
        IOWait();

        outb(PIC1_DATA, 4);
        IOWait();
        outb(PIC2_DATA, 2);
        IOWait();

        outb(PIC1_DATA, ICW4_8086);
        IOWait();
        outb(PIC2_DATA, ICW4_8086);
        IOWait();

        outb(PIC1_DATA, 0b11111111);
        IOWait();
        outb(PIC2_DATA, 0b11111111);
        IOWait();

        outb(0x43, 0b00110100);
    }

    //  Reload can be whatever, might affect performance
    UINT16 Reload = 0x1000;
    interrupt::msPerTick = ((double)Reload*1000)/PIC_FREQUENCY;

    //  Send the reload value to the PIT
    outb(0x40, (UINT8)Reload);
    IOWait();
    outb(0x40, (UINT8)(Reload >> 8));
    IOWait();

    //  Unmask IRQ 0, 1, 7
    outb(PIC1_DATA, 0b01111100);
    IOWait();

    //  Unmask IRQ 12
    outb(PIC2_DATA, 0b11101111);
    IOWait();
}
