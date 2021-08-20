#include "scheduler.h"

/**********************************************************************
 *  Global variables
 *********************************************************************/
extern UINT64 nCores;

/* Array (size = nCores) of pointers to Schedulers */
Scheduler **keSchedulers;

/* Array (size = nCores) of pointers to KE_TASK_DESCRIPTOR */
KE_TASK_DESCRIPTOR **keTasks;

/**********************************************************************
 *  Function definitions
 *********************************************************************/

/**********************************************************************
 *  @details Scheduler constructor
 *  @param core - Core number that the scheduler is assigned to
 *********************************************************************/
Scheduler::Scheduler(UINT64 core)
{
    /* Initialize member variables */
    coreID = core;
    nTasks = 1;
    schedulerLock = LOCK_STATUS_FREE;

    /* Initialize idle task */
    va_list args{};
    currentTask = new Task(0, 0, false, 0, 0, args);

    idleTask = currentTask;
    schedule.heap[0] = idleTask;

    /* Set the active task to idle task */
    activeTaskList = idleTask;
    keTasks[coreID] = currentTask->pTaskInfo;
}

/**********************************************************************
 *  @details Gets time elapsed since system startup
 *  @return Number of nanoseconds since system startup
 *********************************************************************/
KE_TIME Scheduler::GetCurrentTime()
{
    /* Update the time using the current HPET counter register value */
    UINT64 hpetCounter = *(UINT64*)(keSysDescriptor->pHPET + 0xF0);
    return hpetCounter*((double)keSysDescriptor->hpetPeriod/1e6);
}

/**********************************************************************
 *  @details Adds a task to the priority queue
 *  @param t - Task to add
 *********************************************************************/
void Scheduler::AddTask(Task *t)
{
    schedule.Insert(t);
}

/**********************************************************************
 *  @details Removes the task t from the active task linked list
 *  @param t - Task to remove
 *********************************************************************/
void Scheduler::PopFromActiveTaskList(Task *t)
{
    /* Traverse the linked list to get the previous element before currentTask */
    Task *currentTaskIter = activeTaskList;
    Task *currentTaskIterPrev = nullptr;
    while(currentTaskIter != nullptr && currentTaskIter != t)
    {
        currentTaskIterPrev = currentTaskIter;
        currentTaskIter = currentTaskIter->nextActiveTask;
    }

    /* If the current task is the first in the list */
    if(currentTaskIterPrev == nullptr)
    {
        /* Remove task */
        activeTaskList = currentTaskIter->nextActiveTask;
    }
    else
    {
        /* Remove task */
        currentTaskIterPrev->nextActiveTask = currentTaskIter->nextActiveTask;
    }
}

/**********************************************************************
 *  @details Reschedules task t based on its period
 *  @param t - Task to reschedule
 *********************************************************************/
void Scheduler::RescheduleTask(Task *t)
{
    /* If the task took longer to execute than its period */
    if(currentTime > (t->startTime + t->periodNanoSeconds))
    {
        /* Reschedule immediately */
        t->startTime = currentTime;
    }
    else
    {
        /* Reschedule after one period has elapsed since previous call */
        t->startTime += currentTask->periodNanoSeconds;
    }

    /* Reset member variables and registers of the current task */
    t->ended = false;
    t->pTaskInfo->rsp = t->pStack;
    t->pTaskInfo->rbp = t->pStack;
    t->pTaskInfo->rdi = (UINT64)t->args;
    t->pTaskInfo->IRETQCS = 0x08;
    t->pTaskInfo->IRETQRFLAGS = 0x202;
    t->pTaskInfo->IRETQRIP = t->pEntryPoint;

    /* Write the return-address of the task to a special handler */
    *(UINT64*)t->pStack = (UINT64)&ReturnHandler;

    /* Reschedule the task */
    AddTask(t);
}

/**********************************************************************
 *  @details Called every APIC timer interrupt
 *********************************************************************/
void Scheduler::Tick()
{
    /* Update current time */
    currentTime = GetCurrentTime();

    /* If a task is currently editing the scheduler return to the task and let it finish */
    if (LOCK_STATUS_LOCKED == schedulerLock)
    {
        return;
    }

    AcquireLock(&schedulerLock);

    /* If the current task is finished, remove/reschedule it */
    if(currentTask->ended)
    {
        /* Remove the current task from the active task list */
        PopFromActiveTaskList(currentTask);
        nTasks--;

        /* Idle if there are no tasks to perform */
        if(activeTaskList == nullptr)
        {
            activeTaskList = idleTask;
        }

        /* If need be, reschedule the task */
        if (currentTask->reschedule)
        {
            RescheduleTask(currentTask);
        }
        else
        {
            /* Do not destroy object if the task is just being suspended */
            if (!currentTask->suspended)
            {
                TaskTree.Remove(currentTask->handle);
                delete currentTask;
            }
        }
    }

    /* Check the priority queue for any tasks that need to start executing */
    while ( (schedule.size >= 1) && (schedule.heap[1]->startTime <= currentTime) )
    {
        nTasks++;

        /* Remove task from top of priority queue */
        Task *newActiveTask = schedule.Deque();

        /* If there are no active tasks, stop idling */
        if(activeTaskList == idleTask) newActiveTask->nextActiveTask = nullptr;
        else newActiveTask->nextActiveTask = activeTaskList;

        /* The new task is now the first element in the list */
        activeTaskList = newActiveTask;
    }

    /* Start executing tasks starting from top of active task list */
    currentTask = activeTaskList;

    /* If the current active task is at the end of the list, go back to beginning of the list */
    if(currentTask->nextActiveTask == nullptr) currentTask = activeTaskList;
    else currentTask = currentTask->nextActiveTask;

    /* Set current task in the global task structure */
    keTasks[coreID] = currentTask->pTaskInfo;

    ReleaseLock(&schedulerLock);
}

/**********************************************************************
 *  @details Called immediately after a task finishes
 *********************************************************************/
void ReturnHandler()
{
    UINT64 coreID = APICID();
    auto scheduler = keSchedulers[coreID];
    auto currentTask = scheduler->currentTask;

    /* Lock the scheduler to prevent it from accessing the structure we're about to edit */
    AcquireLock(&scheduler->schedulerLock);

    currentTask->ended = true;

    /* Unlock the scheduler */
    ReleaseLock(&scheduler->schedulerLock);

    /* Invoke the APIC timer interrupt, repeating if the scheduler is locked */
    while (true)
    {
        asm volatile("int $48");
        asm volatile("pause");
    }
}

/**********************************************************************
 *  @details Task constructor
 *  @param entry - Pointer to entry point
 *  @param startTime - Kernel time when the task should begin
 *  @param reschedule - True if you want the task to repeatedly run
 *  @param periodNanoSeconds - Period of the task in nanoseconds
 *  @param nArgs - Number of arguments to the entry point. Maximum of 6.
 *  @param ... - The arguments. Must be integer or pointer type only
 *********************************************************************/
Task::Task(UINT64 entry, KE_TIME startTime, bool reschedule, KE_TIME periodNanoSeconds, UINT64 nArgs, va_list args)
{
    static UINT64 handleCounter = 0;
    static LOCK handleCounterLock = LOCK_STATUS_FREE;

    /* Initialize member variables */
    this->reschedule = reschedule;
    this->startTime = startTime;
    this->periodNanoSeconds = periodNanoSeconds;
    this->nextActiveTask = nullptr;
    this->pEntryPoint = entry;
    this->ended = false;
    this->suspended = false;

    /* Ensure mutual exclusion for thread safety */
    AcquireLock(&handleCounterLock);
    this->handle = ++handleCounter;
    ReleaseLock(&handleCounterLock);

    /* Allocate 16-byte aligned stack */
    this->pStackUnaligned = (UINT64)heap::malloc(STACK_SIZE + 8 + 0x10);
    this->pStack = this->pStackUnaligned + STACK_SIZE;
    if (this->pStack & 0x0F) this->pStack = (this->pStack + 0x10) & 0xFFFFFFFFFFFFFFF0ULL;

    /* Allocate 16-byte aligned task descriptor */
    this->pTaskInfoUnaligned = (UINT64)heap::malloc(sizeof(KE_TASK_DESCRIPTOR) + 0x10);
    this->pTaskInfo = (KE_TASK_DESCRIPTOR *)this->pTaskInfoUnaligned;
    if ((UINT64)this->pTaskInfo & 0x0F)
        this->pTaskInfo = (KE_TASK_DESCRIPTOR*)(((UINT64)this->pTaskInfo + 0x10) & 0xFFFFFFFFFFFFFFF0ULL);

    /* Zero out the newly allocated KE_TASK_DESCRIPTOR */
    memset64(pTaskInfo, sizeof(KE_TASK_DESCRIPTOR), 0);

    /* SYS-V ABI argument arrangement */
    UINT64 *taskArguments[6] = {
            &pTaskInfo->rdi,
            &pTaskInfo->rsi,
            &pTaskInfo->rdx,
            &pTaskInfo->rcx,
            &pTaskInfo->r8,
            &pTaskInfo->r9,
    };

    /* Set argument registers (SYS-V ABI calling convention) */
    for (UINT64 i = 0; i < nArgs; i++)
    {
        *taskArguments[i] = va_arg(args, UINT64);
    }

    /* Set other registers */
    *(UINT64*)pStack = (UINT64)&ReturnHandler;
    pTaskInfo->rsp = pStack;
    pTaskInfo->rbp = pStack;
    pTaskInfo->IRETQCS = 0x08;
    pTaskInfo->IRETQRFLAGS = 0x202;
    pTaskInfo->IRETQRIP = entry;

    /* Mask all exceptions in MXCSR (used in FXSAVE and FXRSTOR)
     * Otherwise, floating point instructions will cause an exception */
    *((UINT32*)((UINT64)pTaskInfo + 0x18)) |= 0x00001F80;
}

/**********************************************************************
 *  @details Task destructor
 *********************************************************************/
Task::~Task()
{
    delete (UINT8*)pStackUnaligned;
    delete (UINT8*)pTaskInfoUnaligned;
}

/**********************************************************************
 *  @details Priority queue constructor
 *********************************************************************/
PriorityQueue::PriorityQueue()
{
    heap = new Task*[INIT_PQ_SIZE];
    maxSize = INIT_PQ_SIZE;
    size = 0;
}

/**********************************************************************
 *  @details Heapify subroutine for minheap
 *  @param index - Node index to perform the operation
 *********************************************************************/
void PriorityQueue::Heapify(UINT64 index)
{
    UINT64 left;
    UINT64 right;
    UINT64 min;

    /* Iterative approach instead of recursive approach */
    while(index < size)
    {
        left = index*2;
        right = index*2 + 1;
        min = index;

        if(left <= size && heap[left]->startTime < heap[index]->startTime)
            min = left;

        if(right <= size && heap[right]->startTime < heap[min]->startTime)
            min = right;

        if(min == index) return;

        mem::swap(heap[index], heap[min]);
        index = min;
    }
}

/**********************************************************************
 *  @details Insert a task into the priority queue
 *  @param t - Task to insert
 *********************************************************************/
void PriorityQueue::Insert(Task *t)
{
    /* Increase the heap size */
    if(++size > maxSize - 1)
    {
        auto newHeap = new Task*[maxSize*2];
        for(UINT64 i = 0; i < maxSize; i++)
            newHeap[i] = heap[i];

        delete[] heap;
        heap = newHeap;

        maxSize *= 2;
    }

    /* Place the new member at the end of the heap */
    heap[size] = t;

    /* Obtain index of the new member's parent node */
    UINT64 parent = size/2;

    /* Climb up the tree and heapify each node to move the new task into correct spot */
    while(parent > 0)
    {
        Heapify(parent);
        parent /= 2;
    }
}

/**********************************************************************
 *  @details Remove and get the task at the top of the priority queue
 *  @returns Task at index 1 of minheap
 *********************************************************************/
Task *PriorityQueue::Deque()
{
    Task *retVal = heap[1];
    heap[1] = heap[size--];
    Heapify(1);

    return retVal;
}
