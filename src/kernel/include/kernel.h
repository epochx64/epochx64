#ifndef KERNEL_H
#define KERNEL_H

/**********************************************************************
 *  Includes
 *********************************************************************/

#include <defs/int.h>
#include <asm/asm.h>
#include <mem.h>
#include <graphics.h>
#include <interrupt.h>
#include <acpi.h>
#include <log.h>
#include <boot/shared_boot_defs.h>
#include <window.h>
#include <fs/ext2.h>
#include <process.h>

/**********************************************************************
 *  Function declarations
 *********************************************************************/

extern "C" void KeMain(KE_SYS_DESCRIPTOR*);

/**********************************************************************
 *  @details Populates keSysDescriptor function pointer list
 *  @return None
 *********************************************************************/
void KeInitAPI();

/**********************************************************************
 *  @details Kernel API function to get the current KE_TIME
 *  @return Time in nanoseconds since system startup
 *********************************************************************/
KE_TIME KeGetTime();

/**********************************************************************
 *  @details Kernel API function to spawn a task aka thread
 *  @param entry - Pointer to function entry point
 *  @param reschedule -
 *  @return None
 *********************************************************************/
KE_HANDLE KeScheduleTask(UINT64 entry, KE_TIME startTime, UINT8 reschedule, KE_TIME periodNanoSeconds, KE_TASK_ARG* args);

#endif