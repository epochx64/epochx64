#include "include/epoch.h"

/**********************************************************************
 *  Externals
 *********************************************************************/

extern __attribute__((sysv_abi)) int main();
extern "C" void __attribute__((sysv_abi)) _epochstart(KE_SYS_DESCRIPTOR* kernelDescriptor, STDOUT *out, UINT8 noWindow);

/**********************************************************************
 *  Global variables
 *********************************************************************/

KE_SYS_DESCRIPTOR *keSysDescriptor;
ext2::RAMDisk *keRamDisk;

/**********************************************************************
 *  Function declarations
 *********************************************************************/

/* Kernel API function list */
KE_CREATE_PROCESS KeCreateProcess;
KE_SCHEDULE_TASK KeScheduleTask;
KE_SUSPEND_CURRENT_TASK KeSuspendCurrentTask;
KE_GET_TIME KeGetTime;

/* DWM function list */
DWM_CREATE_WINDOW DwmCreateWindow;

/**********************************************************************
 *  Function definitions
 *********************************************************************/

void KeInitAPI()
{
    KeCreateProcess = keSysDescriptor->KeCreateProcess;
    KeScheduleTask = keSysDescriptor->KeScheduleTask;
    KeSuspendCurrentTask = keSysDescriptor->KeSuspendCurrentTask;
    KeGetTime = keSysDescriptor->KeGetTime;
}

void DwmInitAPI()
{
    auto dwmDescriptor = (DWM_DESCRIPTOR*)keSysDescriptor->pDwmDescriptor;

    DwmCreateWindow = dwmDescriptor->dwmCreateWindow;
}

void _epochstart(KE_SYS_DESCRIPTOR *sysDescriptor, STDOUT *out, UINT8 noWindow)
{
    keSysDescriptor = sysDescriptor;
    log::kout.pFramebufferInfo = &(sysDescriptor->gopInfo);
    keRamDisk = (ext2::RAMDisk*)keSysDescriptor->pRAMDisk;

    heap::init();
    KeInitAPI();
    DwmInitAPI();

    STDOUT *stdout;
    stdout = (out == nullptr) ? createStdout():out;
    setStdout(stdout);

    if(!noWindow)
    {
        DWM_WINDOW_PROPERTIES properties;
        properties.pStdout = (UINT64)stdout;
        properties.height = 400;
        properties.width = 600;
        properties.state = WINDOW_STATE_FOCUED;
        properties.type = WINDOW_TYPE_TERMINAL;
        properties.x = 200;
        properties.y = 100;

        DwmCreateWindow(&properties);
    }

    main();
}
