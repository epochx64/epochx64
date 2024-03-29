#ifndef _SCHEDULER_H
#define _SCHEDULER_H

/**********************************************************************
 *  Includes
 *********************************************************************/

#include <defs/int.h>
#include <typedef.h>
#include <mem.h>
#include <log.h>
#include <asm/asm.h>
#include <math/math.h>
#include <stdarg.h>
#include <ipc.h>
#include <algorithm.h>
#include <io.h>

/**********************************************************************
 *  Define macros
 *********************************************************************/

#define STACK_SIZE 4096

/**********************************************************************
 *  Classes
 *********************************************************************/
class Task
{
public:
    UINT64 pEntryPoint; /* Pointer to entry point */
    UINT64 pStack;      /* Pointer to the top of the stack */
    UINT64 pStackUnaligned;

    UINT64 nArgs;       /* Number of arguments passed to the entrypoint */
    UINT64 args[6];     /* Array of argument values (pointer or integer) */

    bool reschedule;            /* Whether the task repeatedly runs */
    KE_TIME periodNanoSeconds;  /* Period of consecutive task runs */

    bool suspended;
    bool sleeping;      /* True if task is sleeping */
    bool ended;         /* True if the task has finished */
    KE_TIME startTime;  /* Kernel time that the task must start by */

    Task *nextActiveTask;           /* Pointer to next entry in linked list */
    KE_TASK_DESCRIPTOR *pTaskInfo;  /* Pointer to the task's KE_TASK_DESCRIPTOR */
    UINT64 pTaskInfoUnaligned;

    KE_HANDLE handle;
    UINT64 priority;

    /**********************************************************************
     *  @details Task constructor
     *  @param entry - Pointer to entry point
     *  @param startTime - Kernel time when the task should begin
     *  @param reschedule - True if you want the task to repeatedly run
     *  @param periodNanoSeconds - Period of the task in nanoseconds
     *  @param nArgs - Number of arguments to the entry point
     *  @param ... - The arguments. Must be integer or pointer type only
     *********************************************************************/
    Task(UINT64 entry, KE_TIME startTime, bool reschedule, KE_TIME periodNanoSeconds, UINT64 nArgs, va_list args);

    /**********************************************************************
     *  @details Task destructor
     *********************************************************************/
    ~Task();
};

/* Initial size of the priority queue minheap. The array size doubles if this size is ever exceeded */
#define INIT_PQ_SIZE 1024

class PriorityQueue
{
public:
    UINT64 maxSize;
    UINT64 size;
    Task **heap;

    PriorityQueue();
    void Heapify(UINT64 index);
    void Insert(Task *t);
    Task *Deque();
};

class Scheduler
{
public:
    UINT64 nTasks;          /* Number of tasks the scheduler is currently using */
    Task *currentTask;      /* Pointer to task that is currently being executed */
    Task *idleTask;         /* Pointer to idle task */
    Task *activeTaskList;   /* Linked list of tasks which are currently being executed by the scheduler */
    KE_TIME currentTime;    /* Stores the time elapsed since system startup in nanoseconds */
    PriorityQueue schedule; /* New tasks are first added here and sorted in the minheap based on start time. */
    UINT64 coreID;          /* New tasks are first added here and sorted in the minheap based on start time. */
    LOCK schedulerLock;     /* Should be acquired when scheduler structures are being edited */
    AvlTree TaskTree;       /* AVL tree of all tasks that are waiting to execute, or executing */

    Scheduler(UINT64 core);
    void AddTask(Task *t);
    void PopFromActiveTaskList(Task *t);
    void RescheduleTask(Task *t);
    void Tick();
    static KE_TIME GetCurrentTime();
};

/**********************************************************************
 *  Function declarations
 *********************************************************************/

/**********************************************************************
 *  @details Called immediately after a task finishes
 *********************************************************************/
void ReturnHandler();

/**********************************************************************
 *  Global variables
 *********************************************************************/
extern Scheduler **keSchedulers;
extern "C" KE_TASK_DESCRIPTOR **keTasks;

#endif
