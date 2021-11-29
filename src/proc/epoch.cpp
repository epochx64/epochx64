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

UINT64 keSyscallTable;

/**********************************************************************
 *  Function declarations
 *********************************************************************/

/* DWM function list */
DWM_CREATE_WINDOW DwmCreateWindow;

/**********************************************************************
 *  Function definitions
 *********************************************************************/

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
    DwmInitAPI();
    keSyscallTable = keSysDescriptor->keSyscallTable;

    /* Assign stdin/out objects */
    STDOUT *stdout = (out == nullptr) ? createStdout():out;
    STDIN *stdin = (in == nullptr) ? createStdin():in;
    setStdout(stdout);
    setStdin(stdin);

    main();
}
