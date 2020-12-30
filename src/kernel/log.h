#ifndef LOG_H
#define LOG_H

#include <defs/int.h>
#include <asm/asm.h>
#include <lib/string.h>
#include <kernel/graphics.h>
#include "font_rom.h"
#include <kernel/typedef.h>
#include <lib/math/conversion.h>
#include <lib/math/math.h>

namespace log
{
    /*
     * kout << and dout << need to check whether they are
     * printing a string (unsigned char*) or just a plain int
     */
    template <typename T> struct is_pointer { static const bool value = false; };
    template <typename T> struct is_pointer<T*> { static const bool value = true; };

    /*
     * Same as krnlout except it goes to COM1
     */
    class dbgout
    {
    public:
        dbgout();

        /*
         * 0 : Hex
         * 1 : Dec
         */
        UINT8 NumberBase;

        /*
         * Example usage:
         * dout << "300 in Hex: " << HEX << 300 << "\n";
         * dout << "300 in Dec: " << DEC << 300 << "\n";
         */
        template<class T> dbgout &operator<<(T val)
        {
            if(is_pointer<T>::value)
            {
                uint64_t len = string::strlen((unsigned char*)val);
                for(uint64_t i = 0; i < len; i++) PutChar(((unsigned char*)val)[i]);

                return *this;
            }

            using namespace conversion;

            if(NumberBase == HEX_ID)
            {
                UINT8 HexSize = sizeof(val) * 2;
                char OutStringHex[HexSize];

                to_hex(val, OutStringHex);
                for(uint64_t i = 0; i < HexSize; i++) PutChar(OutStringHex[i]);
            }

            if(NumberBase == DEC_ID)
            {
                UINT8 DecSize = math::ilog((UINT64)10, (UINT64)val);
                char OutStringDec[DecSize];

                to_int(val, OutStringDec);
                for(uint64_t i = 0; i < DecSize; i++) PutChar(OutStringDec[i]);
            }

            //  Floating point double
            if(NumberBase == DOUBLE_DEC_ID)
            {
                auto U64Val = (UINT64)val;
                bool Sign = U64Val & 0x8000000000000000;

            }

            return *this;
        }

        //  Goes to COM1
        void PutChar(char c);
    };

    /*
     * Supposed to mock std::cout. Prints to the kernel's screen
     */
    class krnlout
    {
    public:
        krnlout();

        uint16_t k_tty_col;
        uint16_t k_tty_lin;

        /*
         * 0 : Hex
         * 1 : Dec
         */
        UINT8 NumberBase;

        FRAMEBUFFER_INFO *pFramebufferInfo;

        /*
         * Example usage:
         * kout << "300 in Hex: " << HEX << 300 << "\n";
         * kout << "300 in Dec: " << DEC << 300 << "\n";
         */
        template<class T> krnlout &operator<<(T val)
        {
            if(is_pointer<T>::value)
            {
                uint64_t len = string::strlen((unsigned char*)val);
                for(uint64_t i = 0; i < len; i++) PutChar(((unsigned char*)val)[i]);

                return *this;
            }

            using namespace conversion;

            if(NumberBase == HEX_ID)
            {
                UINT8 HexSize = sizeof(val) * 2;
                char OutStringHex[HexSize];

                to_hex(val, OutStringHex);
                for(uint64_t i = 0; i < HexSize; i++) PutChar(OutStringHex[i]);
            }

            if(NumberBase == DEC_ID)
            {
                UINT8 DecSize = math::ilog((UINT64)10, (UINT64)val);
                char OutStringDec[DecSize];

                to_int(val, OutStringDec);
                for(uint64_t i = 0; i < DecSize; i++) PutChar(OutStringDec[i]);
            }

            return *this;
        }

        krnlout &operator<<(double val)
        {
            using namespace conversion;

            if(NumberBase == DOUBLE_DEC_ID)
            {
                if(val < 0)
                {
                    PutChar('-');
                    val *= -1;
                }

                UINT64 BeforeDec = val;
                UINT64 AfterDec = (val - (double)BeforeDec)*math::pow(10, 8);

                UINT8 BeforeDecSize = math::ilog((UINT64)10, BeforeDec);
                UINT8 AfterDecSize = math::ilog((UINT64)10, AfterDec);
                UINT8 Leading0s = 8 - AfterDecSize;

                char BeforeDecBuf[BeforeDecSize];
                to_int(BeforeDec, BeforeDecBuf);

                char AfterDecBuf[AfterDecSize];
                to_int(AfterDec, AfterDecBuf);

                for(UINT8 i = 0; i < BeforeDecSize; i++) PutChar(BeforeDecBuf[i]);
                PutChar('.');
                for(UINT8 i = 0; i < Leading0s; i++) PutChar('0');
                for(UINT8 i = 0; i < AfterDecSize; i++) PutChar(AfterDecBuf[i]);
            }

            return *this;
        }

        void StepForward(UINT64 Amount = 0);

        void SetPosition(TTY_COORD COORD);
        TTY_COORD GetPosition();

        void PutChar(char c);
    };

    extern krnlout kout;
    extern dbgout dout;

    //TODO: Write a function that automatically does newline and prints time, importance level, yaknow, a proper logger
}

#endif
