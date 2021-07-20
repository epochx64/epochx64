#ifndef IPC_H
#define IPC_H

/**********************************************************************
 *  Includes
 *********************************************************************/
#include <defs/int.h>
#include <asm/asm.h>

/**********************************************************************
 *  Definitions
 *********************************************************************/
typedef volatile UINT64 LOCK;
typedef LOCK MUTEX;
typedef LOCK SEMAPHORE;

typedef enum
{
    LOCK_STATUS_FREE = 0,
    LOCK_STATUS_LOCKED
} LOCK_STATUS;

/**********************************************************************
 *  Function declarations
 *********************************************************************/
extern "C"
{
    void AcquireLock(LOCK *lock);
    void ReleaseLock(LOCK *lock);
}

#endif
