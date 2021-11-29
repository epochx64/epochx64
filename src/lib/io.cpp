#include <io.h>
#include <epstring.h>
#include <math/math.h>
#include <math/conversion.h>
#include <mem.h>

/**********************************************************************
 *  Definitions
 *********************************************************************/
#define IS_HEX(A) ( ((A[0] >= '0') && (A[0] <= '9')) && ((A[1] >= '0') && (A[1] <= '9')) && (A[2] == 'x')  )

/**********************************************************************
 *  Local variables
 *********************************************************************/

static STDOUT *pStdout;
static STDIN *pStdin;
static volatile KE_HANDLE taskResumeHandle = 0;

extern "C"
{
    void __attribute__((sysv_abi)) KeSuspendCurrentTask();
    KE_HANDLE __attribute__((sysv_abi)) KeGetCurrentTaskHandle();
    void __attribute__((sysv_abi)) KeResumeTask(KE_HANDLE handle, KE_TIME resumeTime);
    void *__attribute__((sysv_abi)) KeQueryTask(KE_HANDLE handle);
    void __attribute__((sysv_abi)) KeSuspendTask(KE_HANDLE handle);
}

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
    retVal->lock = LOCK_STATUS_FREE;

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
    retVal->lock = LOCK_STATUS_FREE;

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
    /* The stdin object is asynchronously accessed from here and from scanf(). Must be thread safe */
    if (IOSTREAM_ID_STDIN == ID)
    {
        AcquireLock(&pStdin->lock);

        /* Additionally, we must wake scanf() from sleep if it's waiting for input */
        if (taskResumeHandle != 0)
        {
            while (!KeQueryTask(taskResumeHandle));

            if (taskResumeHandle != 0)
                KeResumeTask(taskResumeHandle, 0);
        }
    }

    UINT64 &offset = (ID == IOSTREAM_ID_STDOUT)? pStdout->offset : pStdin->offset;
    UINT64 &originOffset = (ID == IOSTREAM_ID_STDOUT)? pStdout->originOffset : pStdin->originOffset;
    UINT8 *pData = (ID == IOSTREAM_ID_STDOUT)? pStdout->pData : pStdin->pData;

    pData[offset] = c;
    offset = (offset + 1) % STDOUT_DEFAULT_SIZE;

    if (offset == originOffset)
    {
        originOffset = (originOffset + 1) % STDOUT_DEFAULT_SIZE;
    }

    if (IOSTREAM_ID_STDIN == ID)
    {
        ReleaseLock(&pStdin->lock);
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
 *  @details Writes a string to destination
 *  @param str - Null terminated ASCII string to print
 *********************************************************************/
 void sprintStr(char *dst, UINT64 *offset, const char *str)
{
     UINT64 len = string::strlen((unsigned char*)str);

     for (UINT64 i = 0; i < len; i++)
     {
         dst[(*offset)++] = str[i];
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
        /* Read a char from stdin */
        AcquireLock(&pStdin->lock);
        bool isStdinFlushed = stdinFlushed();
        ReleaseLock(&pStdin->lock);

        /* If there is no new data to read, yield this thread until
         * new data is available */
        if (isStdinFlushed)
        {
            taskResumeHandle = KeGetCurrentTaskHandle();
            KeSuspendCurrentTask();
            taskResumeHandle = 0;
        }

        AcquireLock(&pStdin->lock);
        char c = scanchar();
        ReleaseLock(&pStdin->lock);

        /* Stop reading once a space or newline has been encountered */
        if ((c == ' ') || (c == '\n'))
        {
            dst[i] = 0;
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
    UINT64 i = 0;
    UINT64 numbers[100];

    /* Continue until a newline or space is encountered */
    while(true)
    {
        /* Read a char from stdin */
        AcquireLock(&pStdin->lock);
        bool isStdinFlushed = stdinFlushed();
        ReleaseLock(&pStdin->lock);

        /* If there is no new data to read, yield this thread until
         * new data is available */
        if (isStdinFlushed)
        {
            taskResumeHandle = KeGetCurrentTaskHandle();
            KeSuspendCurrentTask();
            taskResumeHandle = 0;
        }

        AcquireLock(&pStdin->lock);
        char c = scanchar();
        ReleaseLock(&pStdin->lock);

        /* Stop reading once a space or newline has been encountered */
        if ((c == ' ') || (c == '\n'))
        {
            break;
        }

        /* Write scanned char to destination buffer */
        numbers[i++] = CHAR_TO_INT(c);
    }

    /* Sum the recorded numbers */
    UINT64 accumulator = 0;
    for (UINT64 j = 1; j <= i; j++)
    {
        accumulator += numbers[j-1] * math::pow(10, i - j);
    }

    return accumulator;
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

    if (len == 0)
    {
        putchar('0', IOSTREAM_ID_STDOUT);
    }
}

/**********************************************************************
 *  @details Print an unsigned number in base-10 to destination string
 *  @param dec - Number in base-10
 *********************************************************************/
 void sprintUnsignedDec(char *dst, UINT64 *offset, UINT64 dec)
{
    UINT64 len = math::ilog((UINT64)10, dec);
    char outString[len];

    conversion::to_int(dec, outString);
    for (UINT64 i = 0; i < len; i++)
    {
        dst[(*offset)++] = outString[i];
    }

    if (len == 0)
    {
        dst[(*offset)++] = '0';
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
 *  @details Prints unsigned number in base-16
 *  @param len - Number of digits to print
 *  @param hex - Number to print
 *********************************************************************/
void sprintUnsignedHex(char *dst, UINT64 *offset, UINT64 len, UINT64 hex)
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
        dst[(*offset)++] = outString[i];
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
 *  @details Writes a double in base-10 to destination string
 *  TODO: Fix this crap
 *********************************************************************/
void sprintDouble(char *dst, UINT64 *offset, double d)
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

    for(UINT8 i = 0; i < BeforeDecSize; i++) dst[(*offset)++] = BeforeDecBuf[i];
    dst[(*offset)++] = '.';
    for(UINT8 i = 0; i < Leading0s; i++) dst[(*offset)++] = '0';
    for(UINT8 i = 0; i < AfterDecSize; i++) dst[(*offset)++] = AfterDecBuf[i];
}

/**********************************************************************
 *  @details Prints a formatted string to serial with variable arguments
 *  @param format - Format string to print
 *********************************************************************/
void SerialOut(char *format, ...)
{
    va_list args;

    va_start(args, format);
    vSerialOut(format, args);
    va_end(args);
}

/**********************************************************************
 *  @details Prints a formatted string to serial with a va_list
 *  @param format - Format string to print
 *  @param args - Variable argument list (va_list)
 *********************************************************************/
void vSerialOut(char *format, va_list args)
{
    char str[MAX_SPRINT_LEN];
    vsprintf(str, format, args);

    for (UINT64 i = 0; (str[i] != 0) && (i < MAX_SPRINT_LEN); i++)
    {
        outb(0x3F8, str[i]);
    }
}

/**********************************************************************
 *  @details Writes the formatted string with arguments to stdout
 *********************************************************************/
void printf(char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

/**********************************************************************
 *  @details Writes the formatted string with arguments to stdout, with a variable argument list
 *********************************************************************/
void vprintf(char *format, va_list args)
{
    UINT64 len = string::strlen((unsigned char*)format);

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
}

/**********************************************************************
 *  @details Writes the formatted string with arguments to the destination string
 *********************************************************************/
 void sprintf(char *dst, char *format, ...)
{
     va_list args;

     va_start(args, format);

     vsprintf(dst, format, args);

     va_end(args);
}

/**********************************************************************
 *  @details va_list version of sprintf
 *********************************************************************/
void vsprintf(char *dst, char *format, va_list args)
{
    UINT64 len = string::strlen((unsigned char *)format);
    UINT64 dstIdx = 0;

    /* Walk through format, print the string */
    for (UINT64 i = 0; i < len; i++)
    {
        /* If we run out of space in the destination buffer */
        if (dstIdx >= MAX_SPRINT_LEN)
        {
            break;
        }

        /* Continue until a percent */
        if (format[i] != '%')
        {
            dst[dstIdx++] = format[i];
            continue;
        }

        /* If at end or the percent is escaped */
        if ((i == (len-1)) || (format[i+1] == '%'))
        {
            dst[dstIdx++] = format[i++];
            continue;
        }

        /* Get the arg and format */
        char fmt = format[++i];
        switch (fmt)
        {
            case 's': sprintStr(dst, &dstIdx, va_arg(args, const char *)); break;
            case 'u': sprintUnsignedDec(dst, &dstIdx, va_arg(args, UINT64)); break;
            case 'f': sprintDouble(dst, &dstIdx, va_arg(args, double)); break;
        }

        if (( (i + 3) <= len) && (IS_HEX((&format[i]))) )
        {
            UINT64 len = ((format[i] - '0')*10) + (format[i+1] - '0');
            sprintUnsignedHex(dst, &dstIdx, len, va_arg(args, UINT64));
            i+=2;
        }
    }

    dst[dstIdx] = 0;
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
            case 's': scanStr(va_arg(args, char *));  break;
            case 'u': *va_arg(args, UINT64*) = scanUnsigned(); break;
            case 'f': *va_arg(args, double*) = scanDouble(); break;
        }

        if (( (i + 3) <= len) && (IS_HEX((&format[i]))) )
        {
            UINT64 len = ((format[i] - '0')*10) + (format[i+1] - '0');
            printUnsignedHex(len, va_arg(args, UINT64));
            i+=2;
        }
    }

    va_end(args);

    taskResumeHandle = 0;
}

/**********************************************************************
 *  @details Reads from the stdin object a string until it terminates with newline
 *********************************************************************/
void scanLine(char *line)
{
    UINT64 i = 0;

    /* Continue until a newline or space is encountered */
    while(true)
    {
        /* Read a char from stdin */
        AcquireLock(&pStdin->lock);
        bool isStdinFlushed = stdinFlushed();
        ReleaseLock(&pStdin->lock);

        /* If there is no new data to read, yield this thread until
         * new data is available */
        if (isStdinFlushed)
        {
            taskResumeHandle = KeGetCurrentTaskHandle();
            KeSuspendCurrentTask();
            taskResumeHandle = 0;
        }

        AcquireLock(&pStdin->lock);
        char c = scanchar();
        ReleaseLock(&pStdin->lock);

        /* Stop reading once a space or newline has been encountered */
        if (c == '\n')
        {
            line[i] = 0;
            break;
        }

        /* Write scanned char to destination buffer */
        line[i++] = c;
    }
}
