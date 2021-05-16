#include "process.h"

extern KE_SYS_DESCRIPTOR *keSysDescriptor;
extern ext2::RAMDisk *keRamDisk;

Process::Process(const char *binaryPath)
{
    ext2::File file(keRamDisk, FILETYPE_REG, binaryPath);

    auto fileBuf = KeSysMalloc(file.Size());
    keRamDisk->ReadFile(file.Data(), (UINT8*)fileBuf);

    auto entry = (void (*)(KE_SYS_DESCRIPTOR*))elf::LoadELF64((Elf64_Ehdr*)fileBuf);

    Task *t = new Task((UINT64)entry, 0, false, 0, (KE_TASK_ARG*)keSysDescriptor);
    Scheduler::ScheduleTask(t);
}

Process::~Process() { }