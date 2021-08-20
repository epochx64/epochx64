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
extern KE_CREATE_PROCESS KeCreateProcess;
extern KE_SCHEDULE_TASK KeScheduleTask;
extern KE_SUSPEND_TASK KeSuspendTask;
extern KE_GET_CURRENT_TASK_HANDLE KeGetCurrentTaskHandle;
extern KE_RESUME_TASK KeResumeTask;
extern KE_GET_TIME KeGetTime;

/* DWM function list */
extern DWM_CREATE_WINDOW DwmCreateWindow;

#endif
