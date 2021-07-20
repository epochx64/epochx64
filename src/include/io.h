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
#include <ipc.h>

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

typedef struct
{
    UINT64 size;                /* Size in bytes */
    UINT64 originOffset;        /* Offset of oldest data stored in the ring buffer */
    UINT64 offset;              /* Offset to newest data to append characters to */

    UINT64 readOffset;          /* Offset for reading from stdin */

    UINT8 *pData;               /* Pointer to ring buffer */
} STDIN;

enum IOStreamID
{
    IOSTREAM_ID_STDOUT = 0,
    IOSTREAM_ID_STDIN = 1
};
/**********************************************************************
 *  Function declarations
 *********************************************************************/

STDOUT *createStdout();
STDIN *createStdin();
void setStdout(STDOUT *stdout);
void setStdin(STDIN *stdin);
STDOUT *getStdout();
STDIN *getStdin();
void putchar(const char c, IOStreamID ID);
void printf(const char *format, ...);
void scanf(const char *format, ...);

#endif
