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
#include <window_common.h>
#include <fs/ext2.h>
#include <process.h>
#include <io.h>
#include <elf/elf.h>
#include <elf/relocation.h>

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
 *  @details Kernel API function to spawn a process from the RAM disk
 *           Eventually will support spawning from persistent storage
 *  @param properties - Pointer to PROCESS_PROPERTIES struct
 *  @param path - RAM disk path to executable elf
 *  @param startTime - KE_TIME when the task should start
 *  @param reschedule - Whether to reschedule the task once it completes
 *  @param periodMicroSeconds - Period of consecutive executions of the task
 *  @param stdout - Pointer to stdout struct or nullptr
 *  @param noWindow - If stdout is nullptr, a console window is made, use this to override
 *                    and don't spawn a window
 *  @return None
 *********************************************************************/
KE_HANDLE KeCreateProcess(PROCESS_PROPERTIES *properties);

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
KE_HANDLE KeScheduleTask(UINT64 entry, KE_TIME startTime, UINT8 reschedule, KE_TIME periodNanoSeconds, UINT64 nArgs, ...);

/**********************************************************************
 *  @details Stops executing the current thread until unsuspend
 *********************************************************************/
void KeSuspendCurrentTask();

#endif