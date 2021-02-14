#ifndef EPOCHX64_EPOCH_H
#define EPOCHX64_EPOCH_H

#include "../../kernel/log.h"
#include <kernel/scheduler.h>
#include <kernel/process.h>

extern __attribute__((sysv_abi)) int main();

namespace kernel
{
    extern KERNEL_DESCRIPTOR *KernelDescriptor;
    extern ext2::RAMDisk *RAMDisk;
}

using namespace kernel;

namespace scheduler
{
    extern Scheduler **Schedulers;
    extern TASK_INFO **TASK_INFOS;
}

/*
 * Start of kernel function list
 */


#endif
