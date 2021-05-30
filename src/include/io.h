#ifndef EPOCHX64_IO_H
#define EPOCHX64_IO_H

/**********************************************************************
 *  Includes
 *********************************************************************/

#include <defs/int.h>
#include <stdarg.h>
#include <epstring.h>
#include <math/math.h>
#include <math/conversion.h>
#include <mem.h>

/**********************************************************************
 *  Definitions
 *********************************************************************/

#define STDOUT_DEFAULT_SIZE 0x1000

typedef struct
{
    UINT64 size;            /* Size in bytes */
    UINT64 originOffset;    /* Offset of oldest data stored in the ring buffer */
    UINT64 offset;          /* Offset to newest data to append characters to */
    UINT8 *pData;           /* Pointer to ring buffer */
} STDOUT;

#define IS_HEX(A) ( ((A[0] >= '0') && (A[0] <= '9')) && ((A[1] >= '0') && (A[1] <= '9')) && (A[2] == 'x')  )

/**********************************************************************
 *  Function declarations
 *********************************************************************/

STDOUT *createStdout();
void setStdout(STDOUT *stdout);
void printf(const char *format, ...);

#endif
