#ifndef KERNEL_H
#define KERNEL_H

/**********************************************************************
 *  Includes
 *********************************************************************/

#include <defs/int.h>
#include <boot/shared_boot_defs.h>

/**********************************************************************
 *  Function declarations
 *********************************************************************/

extern "C" void KeMain(KE_SYS_DESCRIPTOR*);

/**********************************************************************
 *  @details Populates keSysDescriptor function pointer list
 *  @return None
 *********************************************************************/
void KeInitAPI();

extern "C"
{
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
     *  @details Stops executing the specified thread until unsuspend
     *  @param handle - Handle of the task
     *********************************************************************/
    void KeSuspendTask(KE_HANDLE handle);

    /**********************************************************************
     *  @details Stops executing the current thread until unsuspend
     *********************************************************************/
    void KeSuspendCurrentTask();

    /**********************************************************************
     *  @details Gets the handle of the currently executing thread
     *  @param handle - Handle of the task
     *********************************************************************/
    KE_HANDLE KeGetCurrentTaskHandle();

    /**********************************************************************
     *  @details Resumes a previously suspended task
     *  @param handle - Handle of the task to resume
     *  @param resumeTime - Time to resume the task in KE_TIME
     *********************************************************************/
    void KeResumeTask(KE_HANDLE handle, KE_TIME resumeTime);

    /**********************************************************************
     *  @details Checks if a task exists, queries the info if it exists
     *  @return True if task exists
     *********************************************************************/
    void *KeQueryTask(KE_HANDLE handle);

    /**********************************************************************
     *  @details Gets a scheduler that contains the desired task
     *  @return Pointer to Scheduler
     *********************************************************************/
    void *KeQueryScheduler(KE_HANDLE handle);
}

#endif