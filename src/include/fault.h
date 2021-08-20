#ifndef _FAULT_H
#define _FAULT_H

#include <stdarg.h>
#include <defs/int.h>

#define FAULT_ERROR "[ ERROR ]: "
#define FAULT_INFO "[ INFO ]: "

#define FaultLogAssertSerial(condition, format, ...) \
    fFaultLogAssertSerial(condition, format, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

enum FaultLogOutput
{
    FAULT_LOG_OUTPUT_SERIAL = 0,
    FAULT_LOG_OUTPUT_STDOUT,
};

bool fFaultLogAssertSerial(bool condition, const char *format, const char *fileName, const UINT64 lineNumber, const char *functionName, ...);

#endif
