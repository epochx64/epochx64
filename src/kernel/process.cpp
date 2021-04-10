#include "process.h"

namespace kernel
{
    extern KERNEL_DESCRIPTOR *KernelDescriptor;
    extern ext2::RAMDisk *RAMDisk;
}

Process::Process(const char *binaryPath)
{
    using namespace kernel;

    ext2::File file(RAMDisk, FILETYPE_REG, binaryPath);

    auto fileBuf = SysMalloc(file.Size());
    RAMDisk->ReadFile(file.Data(), (UINT8*)fileBuf);

    auto Entry = (void (*)(KERNEL_DESCRIPTOR*))elf::LoadELF64((Elf64_Ehdr*)fileBuf);

    /*
     * Cannot use new operator here, must use SysMalloc so that t persists
     * after it's creating process terminates
     */
    Task *t = (Task*)SysMalloc(sizeof(Task));
    t->Constructor((UINT64)Entry, true, (TASK_ARG*)KernelDescriptor);

    scheduler::Scheduler::ScheduleTask(t);
}

Process::~Process() { }