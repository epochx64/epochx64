#ifndef EPOCHX64_EPOCH_H
#define EPOCHX64_EPOCH_H

#include <log.h>
#include <asm/asm.h>
#include <fs/ext2.h>

extern __attribute__((sysv_abi)) int main();

extern KE_SYS_DESCRIPTOR *keSysDescriptor;
extern ext2::RAMDisk *keRamDisk;

/*
 * Start of kernel function list
 */
extern KE_SCHEDULE_TASK KeScheduleTask;
extern KE_GET_TIME KeGetTime;

#endif
