#ifndef PROCESS_H
#define PROCESS_H

#include <scheduler.h>
#include <fs/ext2.h>
#include <elf/elf.h>
#include <elf/relocation.h>

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
