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
        Task();

        void Constructor(UINT64 Entry, bool Enabled, TASK_ARG* TaskArgs);

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
        TASK_INFO TaskInfo;
    };

    class Scheduler
    {
    public:
        Scheduler(bool Root, UINT64 Core);

        UINT64 Ticks;
        UINT64 nTasks;

        //  Array of Task*
        UINT64 *pTasks;

        Task *CurrentTask;

        /*
         * CoreID is the same as APICID
         */
        UINT64 CoreID;

        void AddTask(Task *T);

        //  Executed every APIC tick
        void Tick();

        /*
         * Use this instead of AddTask
         * It will find the best scheduler to add the task
         * rather than requiring you to do it yourself
         */
        static void ScheduleTask(Task *T);
    };

    extern Scheduler **Schedulers;
    extern "C" TASK_INFO **TASK_INFOS;
}

#endif
