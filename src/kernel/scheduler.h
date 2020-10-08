#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include <defs/int.h>
#include <kernel/typedef.h>
#include <mem.h>
#include <kernel/log.h>

namespace scheduler
{
    #define STACK_SIZE 8192

    class Task
    {
    public:
        Task(UINT64 Entry, bool Enabled, TASK_ARG* TaskArgs);

        UINT64 pEntryPoint;
        UINT64 pStack;

        UINT64 ID;
        UINT64 Priority;

        bool Enabled;
        bool Sleeping;

        /*
         * Our APIC timer counts milliseconds elapsed since
         * it started. When a task starts sleeping, this is set
         * to whatever the count currently is
         */
        UINT64 msSleepStart;

        /*
         * The time that a sleeping task will resume
         */
        UINT64 msSleepEnd;

        TASK_INFO *pTaskInfo;
    };

    class Scheduler
    {
    public:
        Scheduler(bool Root, UINT64 Core);

        UINT64 nTasks;

        //  Array of Task*
        UINT64 *pTasks;
        Task *CurrentTask;

        UINT64 CoreID;

        //  Returns task number
        void AddTask(Task *T);

        //  Executed every APIC tick
        void Tick();
    };

    extern Scheduler *Scheduler0;
    extern "C" TASK_INFO **TASK_INFOS;
}

#endif
