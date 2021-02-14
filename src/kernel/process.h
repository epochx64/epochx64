#ifndef PROCESS_H
#define PROCESS_H

#include <kernel/scheduler.h>
#include <fs/ext2.h>
#include <lib/elf/elf.h>
#include <lib/elf/relocation.h>

using namespace scheduler;

class Process
{
private:
    Task **pTasks;

public:
    Process(const char *binaryPath);
    ~Process();

    UINT64 PID;
    UINT64 nTasks;

    void Start();
    void Stop();

    void AddTask(Task *task);
};

#endif
