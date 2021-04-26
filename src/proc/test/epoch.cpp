#include "epoch.h"

extern "C" void __attribute__((sysv_abi)) _epochstart(KERNEL_DESCRIPTOR* kernelDescriptor);

namespace kernel
{
    KERNEL_DESCRIPTOR *KernelDescriptor;
    ext2::RAMDisk *RAMDisk;
}

void _epochstart(KERNEL_DESCRIPTOR *kernelDescriptor)
{
    KernelDescriptor = kernelDescriptor;
    log::kout.pFramebufferInfo = &(kernelDescriptor->GOPInfo);

    Schedulers = (scheduler::Scheduler**)KernelDescriptor->pSchedulers;
    TASK_INFOS = (scheduler::TASK_INFO**)KernelDescriptor->pTaskInfos;

    kernel::RAMDisk = (ext2::RAMDisk*)KernelDescriptor->pRAMDisk;

    heap::init();

    main();

    while (true);

}
