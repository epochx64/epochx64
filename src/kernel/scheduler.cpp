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
    isLocked = false;
    coreID = core;
    nTasks = 1;

    /* Initialize idle task */
    currentTask = new Task(0, 0, false, 0, nullptr);
    currentTask->ID = 0;
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
    nTasks++;
}

/**********************************************************************
 *  @details Called by the scheduler when a task completes
 *  @param t - Task to add
 *  @return Number of nanoseconds since system startup
 *********************************************************************/
void Scheduler::RemoveCurrentTask()
{
    nTasks--;

    /* Traverse the linked list to get the previous element before currentTask */
    Task *currentTaskIter = activeTaskList;
    Task *currentTaskIterPrev = nullptr;
    while(currentTaskIter != nullptr && currentTaskIter != currentTask)
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

    /* Idle if there are no tasks to perform */
    if(activeTaskList == nullptr)
    {
        activeTaskList = idleTask;
    }

    /* If the current task is meant to repeat */
    if(currentTask->reschedule)
    {
        /* Reset member variables of the current task */
        currentTask->ended = false;

        /* If the task took longer to execute than its period */
        if(currentTime > currentTask->startTime + currentTask->periodNanoSeconds)
        {
            /* Reschedule immediately */
            currentTask->startTime = currentTime;
        }
        else
        {
            /* Reschedule after one period has elapsed since previous call */
            currentTask->startTime += currentTask->periodNanoSeconds;
        }

        /* Reset registers of the current task */
        *(UINT64*)currentTask->pStack = (UINT64)&ReturnFrame;
        currentTask->pTaskInfo->rsp = currentTask->pStack;
        currentTask->pTaskInfo->rbp = currentTask->pStack;
        currentTask->pTaskInfo->rdi = (UINT64)currentTask->args;
        currentTask->pTaskInfo->IRETQCS = 0x08;
        currentTask->pTaskInfo->IRETQRFLAGS = 0x202;
        currentTask->pTaskInfo->IRETQRIP = currentTask->pEntryPoint;

        /* Reschedule the task */
        AddTask(currentTask);
    }
    else
    {
        /* TODO: Add a destructor for tasks */
        delete currentTask;
    }

    currentTask = activeTaskList;
}

/**********************************************************************
 *  @details Called every APIC timer interrupt
 *********************************************************************/
void Scheduler::Tick()
{
    /* Update current time */
    currentTime = GetCurrentTime();

    /* If a task is currently editing the scheduler return to the task and let it finish */
    if(isLocked) return;

    /* If the current task is finished, remove/reschedule it */
    if(currentTask->ended) RemoveCurrentTask();

    /* Check the priority queue for any tasks that need to start executing */
    while ( (schedule.size >= 1) && (schedule.heap[1]->startTime <= currentTime) )
    {
        /* Remove task from top of priority queue */
        Task *newActiveTask = schedule.Deque();

        /* If there are no active tasks, stop idling */
        if(activeTaskList == idleTask) newActiveTask->nextActiveTask = nullptr;
        else newActiveTask->nextActiveTask = activeTaskList;

        /* The new task is now the first element in the list */
        activeTaskList = newActiveTask;
    }

    /* If the current active task is at the end of the list, go back to beginning of the list */
    if(currentTask->nextActiveTask == nullptr) currentTask = activeTaskList;
    else currentTask = currentTask->nextActiveTask;

    /* Set current task in the global task structure */
    keTasks[coreID] = currentTask->pTaskInfo;
}

/**********************************************************************
 *  @details Adds a task to the scheduler with the least amount of tasks
 *  @param t - Task to add
 *********************************************************************/
void Scheduler::ScheduleTask(Task *t)
{
    UINT64 chosenOne = 0;

    for(UINT64 i = 1; i < keSysDescriptor->nCores; i++)
    {
        if (keSchedulers[chosenOne]->nTasks > keSchedulers[i]->nTasks)
            chosenOne = i;
    }

    keSchedulers[chosenOne]->isLocked = true;
    keSchedulers[chosenOne]->AddTask(t);
    keSchedulers[chosenOne]->isLocked = false;
}

/**********************************************************************
 *  @details Called immediately after a task finishes
 *********************************************************************/
void ReturnFrame()
{
    UINT64 coreID = APICID();
    auto scheduler = keSchedulers[coreID];
    auto currentTask = scheduler->currentTask;

    //log::kout << "I HAVE RETURNED!\n";

    /* Lock the scheduler to prevent it from accessing the structure we're about to edit */
    scheduler->isLocked = true;

    /* Signal to the scheduler that the task is completed */
    currentTask->ended = true;

    /* Unlock the scheduler */
    scheduler->isLocked = false;

    /* Wait for the scheduler tick */
    hlt();
}

/**********************************************************************
 *  @details Task constructor
 *  @param entry - Pointer to entry point
 *  @param startTime - Kernel time when the task should begin
 *  @param reschedule - True if you want the task to repeatedly run
 *  @param periodNanoSeconds - Period of the task in nanoseconds
 *  @param taskArgs - Array of pointers to task arguments
 *********************************************************************/
Task::Task(UINT64 entry, KE_TIME startTime, bool reschedule, KE_TIME periodNanoSeconds, KE_TASK_ARG* taskArgs)
{
    /* Initialize member variables */
    this->reschedule = reschedule;
    this->startTime = startTime;
    this->periodNanoSeconds = periodNanoSeconds;
    this->args = taskArgs;
    this->nextActiveTask = nullptr;
    this->pEntryPoint = entry;
    this->ended = false;

    this->pTaskInfo   = (KE_TASK_DESCRIPTOR*)KeSysMalloc(sizeof(KE_TASK_DESCRIPTOR));
    this->pStack      = (UINT64)KeSysMalloc(STACK_SIZE + 1) + STACK_SIZE;

    /* Zero out the newly allocated KE_TASK_DESCRIPTOR */
    for(UINT64 i = 0; i < sizeof(KE_TASK_DESCRIPTOR); i+=8)
        *(UINT64*)((UINT64)pTaskInfo + i) = 0;

    /* Set registers */
    *(UINT64*)pStack = (UINT64)&ReturnFrame;
    pTaskInfo->rsp = pStack;
    pTaskInfo->rbp = pStack;
    pTaskInfo->rdi = (UINT64)taskArgs;
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
