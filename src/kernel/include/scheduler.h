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

/**********************************************************************
 *  Define macros
 *********************************************************************/

#define STACK_SIZE 8192

/**********************************************************************
 *  Classes
 *********************************************************************/
class Task
{
public:
    UINT64 pEntryPoint; /* Pointer to entry point */
    UINT64 pStack;      /* Pointer to the top of the stack */
    KE_TASK_ARG* args;  /* Array of pointers to function arguments */

    bool reschedule;            /* Whether the task repeatedly runs */
    KE_TIME periodNanoSeconds;  /* Period of consecutive task runs */

    bool sleeping;      /* True if task is sleeping */
    bool ended;         /* True if the task has finished */
    KE_TIME startTime;  /* Kernel time that the task must start by */

    Task *nextActiveTask;           /* Pointer to next entry in linked list */
    KE_TASK_DESCRIPTOR *pTaskInfo;  /* Pointer to the task's KE_TASK_DESCRIPTOR */

    UINT64 ID;
    UINT64 priority;

    /**********************************************************************
     *  @details Task constructor
     *  @param entry - Pointer to entry point
     *  @param startTime - Kernel time when the task should begin
     *  @param reschedule - True if you want the task to repeatedly run
     *  @param periodNanoSeconds - Period of the task in nanoseconds
     *  @param taskArgs - Array of pointers to task arguments
     *********************************************************************/
    Task(UINT64 entry, KE_TIME startTime, bool reschedule, KE_TIME periodNanoSeconds, KE_TASK_ARG* taskArgs);

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
    bool isLocked;          /* Should be set when scheduler structures are being edited */
    Task *currentTask;      /* Pointer to task that is currently being executed */
    Task *idleTask;         /* Pointer to idle task */
    Task *activeTaskList;   /* Linked list of tasks which are currently being executed by the scheduler */
    KE_TIME currentTime;    /* Stores the time elapsed since system startup in nanoseconds */
    PriorityQueue schedule; /* New tasks are first added here and sorted in the minheap based on start time. */
    UINT64 coreID;          /* New tasks are first added here and sorted in the minheap based on start time. */

    /**********************************************************************
     *  @details Scheduler constructor
     *  @param core - Core number that the scheduler is assigned to
     *********************************************************************/
    Scheduler(UINT64 core);

    /**********************************************************************
     *  @details Adds a task to the priority queue
     *  @param t - Task to add
     *********************************************************************/
    void AddTask(Task *t);

    /**********************************************************************
     *  @details Called by the scheduler when a task completes
     *  @param t - Task to add
     *  @return Number of nanoseconds since system startup
     *********************************************************************/
    void RemoveCurrentTask();

    /**********************************************************************
     *  @details Called every APIC timer interrupt
     *********************************************************************/
    void Tick();

    /**********************************************************************
     *  @details Adds a task to the scheduler with the least amount of tasks
     *  @param t - Task to add
     *********************************************************************/
    static void ScheduleTask(Task *t);

    /**********************************************************************
     *  @details Gets time elapsed since system startup
     *  @return Number of nanoseconds since system startup
     *********************************************************************/
    static KE_TIME GetCurrentTime();
};

/**********************************************************************
 *  Function declarations
 *********************************************************************/

/**********************************************************************
 *  @details Called immediately after a task finishes
 *********************************************************************/
void ReturnFrame();

/**********************************************************************
 *  Global variables
 *********************************************************************/
extern Scheduler **keSchedulers;
extern "C" KE_TASK_DESCRIPTOR **keTasks;

#endif
