#include "pic.h"

namespace interrupt
{
    extern "C" void int49();
    extern double nsPerTick;
}

UINT8 MouseID;
UINT8 MousePacketSize;

UINT8 KePS2Read()
{

    while(!(inb(0x64) & 1));

    return inb(0x60);
}

void KePS2Write(UINT8 Port, UINT8 Value)
{
    if(Port == 2)
    {
        while(inb(0x64) & 2);
        outb(PS2_COMMAND_PORT, 0xD4);
    }

    while(inb(0x64) & 2);
    outb(0x60, Value);
}

void KeInitPS2()
{

    using namespace log;

    /*  TODO:   This init sequence doesn't work on some hardware (possibly older)
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
        UINT8 result = (inb(0x60) & (~0b00110000)) | 0b00000011;

        while(inb(PS2_COMMAND_PORT) & 2);
        outb(PS2_COMMAND_PORT, 0x60);

        while(inb(PS2_COMMAND_PORT) & 2);
        KePS2Write(1, result);
    }

    /*
     * Perform self tests
     */
    {
        while(inb(0x64) & 2);
        outb(PS2_COMMAND_PORT, 0xAA);
        if(KePS2Read() != 0x55) kout << "Self test FAIL\n";

        while(inb(0x64) & 2);
        outb(PS2_COMMAND_PORT, 0xAB);
        if(KePS2Read()) kout << "Port 1 test FAIL\n";

        while(inb(0x64) & 2);
        outb(PS2_COMMAND_PORT, 0xA9);
        if(KePS2Read()) kout << "Port 2 test FAIL\n";
    }

    /*
     * Enable PS/2 Mouse
     */
    {
        //  Enable mouse
        while(inb(PS2_COMMAND_PORT) & 2);
        outb(PS2_COMMAND_PORT, 0xA8);

        //  Configure mouse
        KePS2Write(2, 0xF6);
        KePS2Read();

        KePS2Write(2, 0xF4);
        KePS2Read();

        //  Get mouse ID
        KePS2Write(2, 0xF2);
        KePS2Read();
        MouseID = KePS2Read();

        MousePacketSize = 3 + (UINT8)((bool)MouseID);
        kout << "[ PS/2 ]: Mouse ID = 0x" <<HEX<< MouseID << "\n";
    }

    //  Enable Keyboard
    while(inb(0x64) & 2);
    outb(PS2_COMMAND_PORT, 0xAE);
}

void KeInitPIC()
{
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
    }

    /*
     * Unmask IRQ 1, 2
     * Unmask IRQ 12 and cascade IRQ 9
     */
    outb(PIC1_DATA, 0b11111001);
    outb(PIC2_DATA, 0b11101101);
}