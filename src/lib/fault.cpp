#include <fault.h>
#include <io.h>

static bool vFaultLogAssert (bool condition, const char *format, va_list args, FaultLogOutput output, const char *fileName, const UINT64 lineNumber, const char *functionName);

/**********************************************************************
 *  @details Prints an error message to serial
 *  @param condition - If FALSE, then the fault log message is printed
 *  @param format - Format string to print
 *  @fileName - Replaced with __FILE__ macro
 *  @lineNumber - Replaced with __LINE__ macro
 *  @functionName - Replaced with __FUNCTION__ macro
 *********************************************************************/
bool fFaultLogAssertSerial(
        bool condition,
        const char *format,
        const char *fileName,
        const UINT64 lineNumber,
        const char *functionName,
        ...)
{
    bool result;

    va_list args;
    va_start(args, functionName);
    result = vFaultLogAssert(condition, format, args, FAULT_LOG_OUTPUT_SERIAL, fileName, lineNumber, functionName);
    va_end(args);

    return result;
}

/**********************************************************************
 *  @details Prints an error message to the specified output
 *  @param condition - If FALSE, then the fault log message is printed
 *  @param format - Format string to print
 *  @fileName - Replaced with __FILE__ macro
 *  @lineNumber - Replaced with __LINE__ macro
 *  @functionName - Replaced with __FUNCTION__ macro
 *********************************************************************/
static bool vFaultLogAssert (
        bool condition,
        const char *format,
        va_list args,
        FaultLogOutput output,
        const char *fileName,
        const UINT64 lineNumber,
        const char *functionName
        )
{
    void (*PrintFunctionList[])(char *, ...) = { SerialOut,  printf };
    void (*vPrintFunctionList[])(char *, va_list args) = { vSerialOut, vprintf };

    /* Check for invalid arguments */
    if (format == nullptr)
    {
        PrintFunctionList[output]("%sIn %s, line %u: %s: NULL format string passed\n", FAULT_ERROR, fileName, lineNumber, functionName);
    }

    /* Leave if the assert passes */
    if (condition)
    {
        return true;
    }

    /* Log the assert failure */
    PrintFunctionList[output]("%sAssert fail in: %s, line %u: %s: ", FAULT_ERROR, fileName, lineNumber, functionName);
    vPrintFunctionList[output]((char*)format, args);

    return false;
}

