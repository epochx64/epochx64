#include <io.h>

/**********************************************************************
 *  Definitions
 *********************************************************************/
#define IS_HEX(A) ( ((A[0] >= '0') && (A[0] <= '9')) && ((A[1] >= '0') && (A[1] <= '9')) && (A[2] == 'x')  )

/**********************************************************************
 *  Local variables
 *********************************************************************/

static STDOUT *pStdout;
static STDIN *pStdin;

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
 *  @details Creates an STDIN object with a 4KiB buffer
 *  @returns stdin - Pointer to stdin struct
 *********************************************************************/
STDIN *createStdin()
{
    //TODO: ALlow user to specify size of stdin when creating it
    auto retVal = new STDIN;

    retVal->pData = new UINT8[STDOUT_DEFAULT_SIZE];
    retVal->size = STDOUT_DEFAULT_SIZE;
    retVal->offset = 0;
    retVal->readOffset = 0;
    retVal->originOffset = 0;

    return retVal;
}

/**********************************************************************
 *  @details Checks whether more data can be read from stdin
 *  @returns True if no new data can be read from stdin
 *********************************************************************/
bool stdinFlushed()
{
    return (pStdin->readOffset == pStdin->offset);
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
 *  @details Sets the stdin object for scanf
 *  @param stdin - Pointer to stdin struct
 *********************************************************************/
void setStdin(STDIN *stdin)
{
    pStdin = stdin;
}

/**********************************************************************
 *  @details Gets the stdin object
 *  @return stdout - Pointer to stdin struct
 *********************************************************************/
STDOUT *getStdout()
{
    return pStdout;
}

/**********************************************************************
 *  @details Gets the stdin
 *  @return stdin - Pointer to stdin struct
 *********************************************************************/
STDIN *getStdin()
{
    return pStdin;
}

/**********************************************************************
 *  @details Writes a single character to stdout/in
 *  @param c - The character to print (ASCII)
 *  @param ID - 0 to write to stdout; 1 for stdin
 *********************************************************************/
void putchar(const char c, IOStreamID ID)
{
    UINT64 &offset = (ID == IOSTREAM_ID_STDOUT)? pStdout->offset : pStdin->offset;
    UINT64 &originOffset = (ID == IOSTREAM_ID_STDOUT)? pStdout->originOffset : pStdin->originOffset;
    UINT8 *pData = (ID == IOSTREAM_ID_STDOUT)? pStdout->pData : pStdin->pData;

    pData[offset] = c;
    offset = (offset + 1) % STDOUT_DEFAULT_SIZE;

    if (offset == originOffset)
    {
        originOffset = (originOffset + 1) % STDOUT_DEFAULT_SIZE;
    }
}

/**********************************************************************
 *  @details Scans a character from stdin
 *  @returns The read char, or null, from stdin
 *********************************************************************/
char scanchar()
{
    /* Do not read further if the buffer has not been filled */
    UINT64 &readOffset = pStdin->readOffset;
    if (readOffset == pStdin->offset) return 0;

    /* Get one letter from stdin and increment stdin read offset */
    char c = pStdin->pData[readOffset];
    readOffset = (readOffset + 1) % STDOUT_DEFAULT_SIZE;

    return c;
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
        putchar(str[i], IOSTREAM_ID_STDOUT);
    }
}

/**********************************************************************
 *  @details Reads a string from stdin
 *  @param dst - Destination string array
 *********************************************************************/
void scanStr(char *dst)
{
    UINT64 i = 0;

    /* Continue until a newline or space is encountered */
    while(true)
    {
        /* If there is no new data to read, yield this thread until
         * new data is available */
        if (stdinFlushed())
        {

        }

        /* Read a char from stdin */
        char c = scanchar();

        /* Stop reading once a space or newline has been encountered */
        if ((c == ' ') || (c == '\n'))
        {
            break;
        }

        /* Write scanned char to destination buffer */
        dst[i++] = c;
    }
}

/**********************************************************************
 *  @details Reads an unsigned int from stdin
 *  @returns The integer to be scanned
 *********************************************************************/
UINT64 scanUnsigned()
{
    return 0;
}

/**********************************************************************
 *  @details Reads a double-precision float from stdin
 *  @returns The double to be scanned
 *********************************************************************/
double scanDouble()
{
    return 0;
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
        putchar(outString[i], IOSTREAM_ID_STDOUT);
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
        putchar(outString[i], IOSTREAM_ID_STDOUT);
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
        putchar('-', IOSTREAM_ID_STDOUT);
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

    for(UINT8 i = 0; i < BeforeDecSize; i++) putchar(BeforeDecBuf[i], IOSTREAM_ID_STDOUT);
    putchar('.', IOSTREAM_ID_STDOUT);
    for(UINT8 i = 0; i < Leading0s; i++) putchar('0', IOSTREAM_ID_STDOUT);
    for(UINT8 i = 0; i < AfterDecSize; i++) putchar(AfterDecBuf[i], IOSTREAM_ID_STDOUT);
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
            putchar(format[i], IOSTREAM_ID_STDOUT);
            continue;
        }

        /* If at end or the percent is escaped */
        if ((i == (len-1)) || (format[i+1] == '%'))
        {
            putchar(format[i++], IOSTREAM_ID_STDOUT);
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

/**********************************************************************
 *  @details Reads from the stdin object a formatted string
 *********************************************************************/
 void scanf(const char *format, ...)
{
    va_list args;
    UINT64 len = string::strlen((unsigned char*)format);

    va_start(args, format);

    /* Walk through format, scan the string */
    for (UINT64 i = 0; i < len; i++)
    {
        /* Continue until a percent */
        if (format[i] != '%')
        {
            continue;
        }

        /* If at end or the percent is escaped */
        if ((i == (len-1)) || (format[i+1] == '%'))
        {
            continue;
        }

        /* Get the arg and format */
        char fmt = format[++i];
        switch (fmt)
        {
            //case 's': scanStr(va_arg(args, const char **));  break;
            //case 'u': *va_arg(args, UINT64*) = scanUnsigned(); break;
            //case 'f': *va_arg(args, double*) = scanDouble(); break;
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