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

/**********************************************************************
 *  Function declarations
 *********************************************************************/
extern "C"
{
    void AcquireLock(LOCK *lock);
    void ReleaseLock(LOCK *lock);
}

#endif
