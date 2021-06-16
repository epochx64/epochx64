#include <io.h>

/**********************************************************************
 *  Definitions
 *********************************************************************/
#define IS_HEX(A) ( ((A[0] >= '0') && (A[0] <= '9')) && ((A[1] >= '0') && (A[1] <= '9')) && (A[2] == 'x')  )

/**********************************************************************
 *  Local variables
 *********************************************************************/

static STDOUT *pStdout;

/**********************************************************************
 *  Function definitions
 *********************************************************************/

/**********************************************************************
 *  @details Creates an STDOUT object with a 4KiB buffer
 *  @param stdout - Pointer to stdout struct
 *********************************************************************/
STDOUT *createStdout()
{
    auto retVal = new STDOUT;

    retVal->pData = new UINT8[STDOUT_DEFAULT_SIZE];
    retVal->size = STDOUT_DEFAULT_SIZE;
    retVal->offset = 0;
    retVal->originOffset = 0;

    return retVal;
}

/**********************************************************************
 *  @details Sets the stdout object for printf
 *  @param stdout - Pointer to stdout struct
 *********************************************************************/
void setStdout(STDOUT *stdout)
{
    pStdout = stdout;
}

/**********************************************************************
 *  @details Writes a single character to stdout
 *  @param c - The character to print (ASCII)
 *********************************************************************/
inline void putchar(const char c)
{
    pStdout->pData[pStdout->offset] = c;
    pStdout->offset = (pStdout->offset + 1) % STDOUT_DEFAULT_SIZE;

    if (pStdout->offset == pStdout->originOffset)
    {
        pStdout->originOffset = (pStdout->originOffset + 1) % STDOUT_DEFAULT_SIZE;
    }
}

/**********************************************************************
 *  @details Writes a string to stdout
 *  @param str - Null terminated ASCII string to print
 *********************************************************************/
void printStr(const char *str)
{
    UINT64 len = string::strlen((unsigned char*)str);

    for (UINT64 i = 0; i < len; i++)
    {
        putchar(str[i]);
    }
}

/**********************************************************************
 *  @details Print an unsigned number in base-10
 *  @param dec - Number in base-10
 *********************************************************************/
void printUnsignedDec(UINT64 dec)
{
    UINT64 len = math::ilog((UINT64)10, dec);
    char outString[len];

    conversion::to_int(dec, outString);
    for (UINT64 i = 0; i < len; i++)
    {
        putchar(outString[i]);
    }
}

/**********************************************************************
 *  @details Prints unsigned number in base-16
 *  @param len - Number of digits to print
 *  @param hex - Number to print
 *********************************************************************/
void printUnsignedHex(UINT64 len, UINT64 hex)
{
    char outString[16];

    conversion::to_hex(hex, outString);

    /* Shorten the hex to desired length */
    for (UINT64 i = 0; i < len; i++)
    {
        outString[i] = outString[i+16-len];
    }

    for (UINT64 i = 0; i < len; i++)
    {
        putchar(outString[i]);
    }
}

/**********************************************************************
 *  @details Writes a double in base-10 to stdout
 *  TODO: Fix this crap
 *********************************************************************/
 void printDouble(double d)
{
    if(d < 0)
    {
        putchar('-');
        d *= -1;
    }

    UINT64 BeforeDec = d;
    UINT64 AfterDec = (d - (double)BeforeDec)*math::pow(10, 8);

    UINT8 BeforeDecSize = math::ilog((UINT64)10, BeforeDec);
    UINT8 AfterDecSize = math::ilog((UINT64)10, AfterDec);
    UINT8 Leading0s = 8 - AfterDecSize;

    char BeforeDecBuf[BeforeDecSize];
    conversion::to_int(BeforeDec, BeforeDecBuf);

    char AfterDecBuf[AfterDecSize];
    conversion::to_int(AfterDec, AfterDecBuf);

    for(UINT8 i = 0; i < BeforeDecSize; i++) putchar(BeforeDecBuf[i]);
    putchar('.');
    for(UINT8 i = 0; i < Leading0s; i++) putchar('0');
    for(UINT8 i = 0; i < AfterDecSize; i++) putchar(AfterDecBuf[i]);
}

/**********************************************************************
 *  @details Writes the formatted string with arguments to stdout
 *********************************************************************/
void printf(const char *format, ...)
{
    va_list args;
    UINT64 len = string::strlen((unsigned char*)format);

    va_start(args, format);

    /* Walk through format, print the string */
    for (UINT64 i = 0; i < len; i++)
    {
        /* Continue until a percent */
        if (format[i] != '%')
        {
            putchar(format[i]);
            continue;
        }

        /* If at end or the percent is escaped */
        if ((i == (len-1)) || (format[i+1] == '%'))
        {
            putchar(format[i++]);
            continue;
        }

        /* Get the arg and format */
        char fmt = format[++i];
        switch (fmt)
        {
            case 's': printStr(va_arg(args, const char *)); break;
            case 'u': printUnsignedDec(va_arg(args, UINT64)); break;
            case 'f': printDouble(va_arg(args, double)); break;
        }

        if (( (i + 3) <= len) && (IS_HEX((&format[i]))) )
        {
            UINT64 len = ((format[i] - '0')*10) + (format[i+1] - '0');
            printUnsignedHex(len, va_arg(args, UINT64));
            i+=2;
        }
    }

    va_end(args);
}