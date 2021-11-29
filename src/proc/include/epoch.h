#ifndef EPOCHX64_EPOCH_H
#define EPOCHX64_EPOCH_H

#include <log.h>
#include <asm/asm.h>
#include <fs/ext2.h>
#include <window_common.h>

extern KE_SYS_DESCRIPTOR *keSysDescriptor;

/*
 * Start of kernel function list
 */

extern "C"
{
    KE_HANDLE __attribute__((sysv_abi)) KeCreateProcess(PROCESS_PROPERTIES *properties);
    KE_TIME __attribute__((sysv_abi)) KeGetTime();
    KE_HANDLE __attribute__((sysv_abi)) KeScheduleTask(UINT64 entry, KE_TIME startTime, UINT8 reschedule, KE_TIME periodNanoSeconds, UINT64 nArgs, ...);
    void __attribute__((sysv_abi)) KeSuspendTask(KE_HANDLE handle);
    void __attribute__((sysv_abi)) KeSuspendCurrentTask();
    KE_HANDLE __attribute__((sysv_abi)) KeGetCurrentTaskHandle();
    void __attribute__((sysv_abi)) KeResumeTask(KE_HANDLE handle, KE_TIME resumeTime);
    void *__attribute__((sysv_abi)) KeQueryTask(KE_HANDLE handle);
    void *__attribute__((sysv_abi)) KeQueryScheduler(KE_HANDLE handle);
}

/* DWM function list */
extern DWM_CREATE_WINDOW DwmCreateWindow;

#endif
