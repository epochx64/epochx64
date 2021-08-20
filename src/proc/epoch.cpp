#include "include/epoch.h"
#include <io.h>

/**********************************************************************
 *  Externals
 *********************************************************************/

extern __attribute__((sysv_abi)) int main();
extern "C" void __attribute__((sysv_abi)) _epochstart(KE_SYS_DESCRIPTOR *sysDescriptor, STDOUT *out, STDIN *in, UINT8 noWindow);

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
KE_SUSPEND_TASK KeSuspendTask;
KE_RESUME_TASK KeResumeTask;
KE_GET_CURRENT_TASK_HANDLE KeGetCurrentTaskHandle;
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
    KeSuspendTask = keSysDescriptor->KeSuspendTask;
    KeGetCurrentTaskHandle = keSysDescriptor->KeGetCurrentTaskHandle;
    KeResumeTask = keSysDescriptor->KeResumeTask;
    KeGetTime = keSysDescriptor->KeGetTime;
}

void DwmInitAPI()
{
    auto dwmDescriptor = (DWM_DESCRIPTOR*)keSysDescriptor->pDwmDescriptor;

    DwmCreateWindow = dwmDescriptor->DwmCreateWindow;
}

void _epochstart(KE_SYS_DESCRIPTOR *sysDescriptor, STDOUT *out, STDIN *in, UINT8 noWindow)
{
    /* Set global variables */
    keSysDescriptor = sysDescriptor;
    log::kout.pFramebufferInfo = &(sysDescriptor->gopInfo);
    keRamDisk = (ext2::RAMDisk*)keSysDescriptor->pRAMDisk;

    /* Initialize process heap and API function pointers */
    heap::init();
    KeInitAPI();
    DwmInitAPI();

    /* Assign stdin/out objects */
    STDOUT *stdout = (out == nullptr) ? createStdout():out;
    STDIN *stdin = (in == nullptr) ? createStdin():in;
    setStdout(stdout);
    setStdin(stdin);

    if(!noWindow)
    {

    }

    main();
}
