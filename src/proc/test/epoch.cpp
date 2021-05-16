#include "epoch.h"

extern "C" void __attribute__((sysv_abi)) _epochstart(KE_SYS_DESCRIPTOR* kernelDescriptor);

KE_SYS_DESCRIPTOR *keSysDescriptor;
ext2::RAMDisk *keRamDisk;

/* Kernel function list */
KE_SCHEDULE_TASK KeScheduleTask;
KE_GET_TIME KeGetTime;

void KeInitAPI()
{
    KeScheduleTask = keSysDescriptor->KeScheduleTask;
    KeGetTime = keSysDescriptor->KeGetTime;
}

void _epochstart(KE_SYS_DESCRIPTOR *kernelDescriptor)
{
    keSysDescriptor = kernelDescriptor;
    log::kout.pFramebufferInfo = &(kernelDescriptor->gopInfo);
    keRamDisk = (ext2::RAMDisk*)keSysDescriptor->pRAMDisk;

    heap::init();
    KeInitAPI();

    main();

    while (true) asm volatile("hlt");

}
