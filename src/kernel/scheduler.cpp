#include "scheduler.h"

namespace ACPI
{
    extern UINT64 nCores;
}

namespace scheduler
{
    Scheduler **Schedulers;

    //  An array of TASK_INFO pointers
    //  Size will be number of cores
    //  Used for linkage with ASM component
    TASK_INFO **TASK_INFOS;

    /*
     *  TODO:   There are a few functions that will need to be changed once deallocating tasks is implemented
     */

    Scheduler::Scheduler(bool Root, UINT64 Core)
    {
        /*
         * We're gonna have max 64 tasks for now
         */
        pTasks = new UINT64[64];
        CoreID = Core;
        nTasks = 1;

        /*
         * The idle task
         */
        CurrentTask = new Task(0, true, nullptr);
        CurrentTask->ID = 0;
        CurrentTask->Enabled = true;
        pTasks[0] = (UINT64)CurrentTask;

        TASK_INFOS[CoreID] = CurrentTask->pTaskInfo;
    }

    void Scheduler::AddTask(Task *T)
    {
        //  TODO: This will have to change
        T->ID = nTasks;
        pTasks[nTasks] = (UINT64)T;
        nTasks++;
    }

    void Scheduler::Tick()
    {
        //  TODO:   Not every tick should result in a task switch

        CurrentTask = (Task*)pTasks[(CurrentTask->ID + 1) % nTasks];
        TASK_INFOS[CoreID] = ((Task*)pTasks[CurrentTask->ID])->pTaskInfo;
    }

    void Scheduler::ScheduleTask(Task *T)
    {
        UINT64 chosenOne = 0;

        for(UINT64 i = 1; i < kernel::KernelDescriptor->nCores; i++)
        {
            if (Schedulers[chosenOne]->nTasks > Schedulers[i]->nTasks) chosenOne = i;
        }

        Schedulers[chosenOne]->AddTask(T);
    }

    Task::Task(UINT64 Entry, bool Enabled, TASK_ARG* TaskArgs)
    {
        this->Enabled = Enabled;

        //  TODO:   Once we have access to dynamic memory outside of the heap these calls should change
        pTaskInfo   = (TASK_INFO*)heap::MallocAligned(sizeof(TASK_INFO), 16);
        pStack      = (UINT64)heap::MallocAligned(STACK_SIZE, 16) + STACK_SIZE;

        /*
         * TODO:    When scheduler prepares a task, it needs to setup a new stack frame. When the task's
         *          main function returns, it needs to go to a special handler which signals to the scheduler
         *          that the task is over, and halts until it's killed
         */
        pTaskInfo->rsp = (UINT64)pStack;
        pTaskInfo->rbp = (UINT64)pStack;
        pTaskInfo->rdi = (UINT64)TaskArgs;
        pTaskInfo->IRETQCS = 0x08;
        pTaskInfo->IRETQRFLAGS = 0x206;
        pTaskInfo->IRETQRIP = Entry;

        /*
         * Mask all exceptions in MXCSR (used in FXSAVE and FXRSTOR)
         * Otherwise, floating point instructions will cause an exception
         */
        *((UINT32*)((UINT64)pTaskInfo + 0x18)) |= 0x00001F80;
    }
}