#include <TOOLKIT/THREAD.H>
#include <assert.h>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
#endif

// / / / / / / / / / / / / / / / / / / / 
// LOCK (MUTEX)                        /
// / / / / / / / / / / / / / / / / / / /
PMUTEX LoadMutex(PALLOCATOR allocator)
{
    PMUTEX mutex = Alloc(allocator, sizeof(MUTEX), alignof(MUTEX));
    if (not mutex) return null;
    mutex->Allocator = *allocator;
#if defined(_WIN32)
    static_assert(sizeof(mutex->OsHandle) >= sizeof(CRITICAL_SECTION));
    InitializeCriticalSection((CRITICAL_SECTION *)mutex->OsHandle);
#elif defined(__unix__) || defined(__APPLE__)
    static_assert(sizeof(mutex->OsHandle) >= sizeof(pthread_mutex_t));
    pthread_mutex_init((pthread_mutex_t *)mutex->OsHandle, null);
#else
    #error "Not Implemented"
#endif
    return mutex;
}
VOID FreeMutex(PMUTEX mutex)
{
#if defined(_WIN32)
    DeleteCriticalSection((CRITICAL_SECTION *)mutex->OsHandle);
#elif defined(__unix__) || defined(__APPLE__)
    pthread_mutex_destroy((pthread_mutex_t *)mutex->OsHandle);
#else
#error "Not Implemented"
#endif
    Free(&mutex->Allocator, mutex);
}
VOID MutexLock(PMUTEX mutex)
{
#if defined(_WIN32)
    EnterCriticalSection((CRITICAL_SECTION *)mutex->OsHandle);
#elif defined(__unix__) || defined(__APPLE__)
    pthread_mutex_lock((pthread_mutex_t *)mutex->OsHandle);
#else
#error "Not Implemented"
#endif
}
VOID MutexUnlock(PMUTEX mutex)
{
#if defined(_WIN32)
    LeaveCriticalSection((CRITICAL_SECTION *)mutex->OsHandle);
#elif defined(__unix__) || defined(__APPLE__)
    pthread_mutex_unlock((pthread_mutex_t *)mutex->OsHandle);
#else
#error "Not Implemented"
#endif
}
BOOL MutexTryLock(PMUTEX mutex)
{
#if defined(_WIN32)
    return TryEnterCriticalSection((CRITICAL_SECTION *)mutex->OsHandle) ? true : false;
#elif defined(__unix__) || defined(__APPLE__)
    return pthread_mutex_trylock((pthread_mutex_t *)mutex->OsHandle) == 0;
#else
#error "Not Implemented"
#endif
}
BOOL MutexTimedLock(PMUTEX mutex, U64 timeout_ms)
{
#if defined(_WIN32)
    DWORD start = GetTickCount();
    while (!TryEnterCriticalSection((CRITICAL_SECTION *)mutex->OsHandle))
    {
        if (GetTickCount() - start >= timeout_ms)
            return false;
    }
    return true;
#elif defined(__unix__) || defined(__APPLE__)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000ULL;
    if (ts.tv_nsec >= 1000000000ULL)
    {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000ULL;
    }
    return pthread_mutex_timedlock((pthread_mutex_t *)mutex->OsHandle, &ts) == 0;
#else
#error "Not Implemented"
#endif
}

// / / / / / / / / / / / / / / / / / / / 
// CONDITION VARIABLE                  /
// / / / / / / / / / / / / / / / / / / /
PCONDITIONVARIABLE LoadConditionVariable(PALLOCATOR allocator)
{
    PCONDITIONVARIABLE cv = Alloc(allocator, sizeof(CONDITIONVARIABLE), alignof(CONDITIONVARIABLE));
    if (not cv) return null;
    cv->Allocator = *allocator;
#ifdef _WIN32
    static_assert(sizeof(cv->OsHandle) >= sizeof(CONDITION_VARIABLE));
    InitializeConditionVariable((CONDITION_VARIABLE *)cv->OsHandle);
    return (PCONDITIONVARIABLE)cv;
#elif defined(__unix__) || defined(__APPLE__)
    static_assert(sizeof(cv->OsHandle) >= sizeof(pthread_cond_t));
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    pthread_cond_init((pthread_cond_t *)cv->OsHandle, &attr);
    pthread_condattr_destroy(&attr);
#else
#error "Not Implemented"
#endif
    return cv;
}
VOID FreeConditionVariable(PCONDITIONVARIABLE condition_variable)
{
#ifdef _WIN32
#elif defined(__unix__) || defined(__APPLE__)
    pthread_cond_destroy((pthread_cond_t *)cv->OsHandle);
#endif
    Free(&condition_variable->Allocator, condition_variable);
}
VOID ConditionVariableWait(PCONDITIONVARIABLE condition_variable, PMUTEX mutex)
{
#ifdef _WIN32
    SleepConditionVariableCS((PCONDITION_VARIABLE)condition_variable->OsHandle, (CRITICAL_SECTION *)mutex->OsHandle, INFINITE);
#elif defined(__unix__) || defined(__APPLE__)
    pthread_cond_wait((pthread_cond_t *)condition_variable->OsHandle, (pthread_mutex_t *)mutex->OsHandle);
#else
#error "Not Implemented"
#endif
}
VOID ConditionVariableSignal(PCONDITIONVARIABLE condition_variable)
{
#ifdef _WIN32
    WakeConditionVariable((PCONDITION_VARIABLE)condition_variable->OsHandle);
#elif defined(__unix__) || defined(__APPLE__)
    pthread_cond_signal((pthread_cond_t *)condition_variable->OsHandle);
#else
#error "Not Implemented"
#endif
}
VOID ConditionVariableBroadcast(PCONDITIONVARIABLE condition_variable)
{
#ifdef _WIN32
    WakeAllConditionVariable((PCONDITION_VARIABLE)condition_variable->OsHandle);
#elif defined(__unix__) || defined(__APPLE__)
    pthread_cond_broadcast((pthread_cond_t *)condition_variable->OsHandle);
#else
#error "Not Implemented"
#endif
}

// / / / / / / / / / / / / / / / / / / / 
// COUNT LATCH                         /
// / / / / / / / / / / / / / / / / / / /
PCOUNTLATCH LoadCountLatch(PALLOCATOR allocator)
{
    errdfs(3);

    // Load Latch
    PCOUNTLATCH latch;
    if (not (latch = Alloc(allocator, sizeof(COUNTLATCH), alignof(COUNTLATCH))))
        return errdfflush(), null;
    latch->Allocator = *allocator;
    errdf(Free, &allocator, latch);
    // Load Mutex
    if (not (latch->Mutex = LoadMutex(allocator)))
        return errdfflush(), null;
    errdf(FreeMutex, latch->Mutex);
    // Load ConditionVariable
    if (not (latch->ConditionVariable = LoadConditionVariable(allocator)))
        return errdfflush(), null;
    errdf(FreeConditionVariable, latch->ConditionVariable);
    return latch;
}
VOID FreeCountLatch(PCOUNTLATCH latch)
{
    FreeConditionVariable(latch->ConditionVariable);
    FreeMutex(latch->Mutex);
    Free(&latch->Allocator, latch);
}
VOID CountLatchWait(PCOUNTLATCH latch, U64 expected)
{
    MutexLock(latch->Mutex);
    while (latch->Pending != expected)
        ConditionVariableWait(latch->ConditionVariable, latch->Mutex);
    MutexUnlock(latch->Mutex);
}
U64 CountLatchIncrement(PCOUNTLATCH latch)
{
    MutexLock(latch->Mutex);
    U64 value = IncU32(&latch->Pending);
    ConditionVariableBroadcast(latch->ConditionVariable);
    MutexUnlock(latch->Mutex);
    return value;
}
U64 CountLatchDecrement(PCOUNTLATCH latch)
{
    MutexLock(latch->Mutex);
    U64 value = DecU32(&latch->Pending);
    ConditionVariableBroadcast(latch->ConditionVariable);
    MutexUnlock(latch->Mutex);
    return value;
}
U64 CountLatchDecrementSignalZero(PCOUNTLATCH latch)
{
    MutexLock(latch->Mutex);
    U64 value = DecU32(&latch->Pending);
    if (value == 0)
        ConditionVariableBroadcast(latch->ConditionVariable);
    MutexUnlock(latch->Mutex);
    return value;
}

// / / / / / / / / / / / / / / / / / / / 
// THREADS                             /
// / / / / / / / / / / / / / / / / / / /
#if defined(_WIN32)
static DWORD WINAPI ThreadStartWrapper(LPVOID arg)
{
    PTHREAD thread = (PTHREAD)arg;
    thread->Routine(thread->UserData);
    return 0;
}
#elif defined(__unix__) || defined(__APPLE__)
static void *ThreadStartWrapper(void *arg)
{
    PTHREAD thread = (PTHREAD)arg;
    thread->Routine(thread->UserData);
    return null;
}
#else

#endif

PTHREAD LoadThread(PALLOCATOR allocator, ROUTINE routine, PVOID user_data)
{
    PTHREAD thread = (PTHREAD)Alloc(allocator, sizeof(THREAD), alignof(THREAD));
    if (!thread)
        return null;
    thread->Routine  = routine;
    thread->UserData = user_data;
	thread->Allocator = *allocator;
#if defined(_WIN32)
    static_assert(sizeof(thread->OsHandle) >= sizeof(HANDLE));
    HANDLE *handle = ((HANDLE *)thread->OsHandle);
    *handle = CreateThread(null, 0, ThreadStartWrapper, thread, 0, null);
    if (*handle == null)
    {
        Free(allocator, thread);
        return null;
    }
    return thread;
#elif defined(__unix__) || defined(__APPLE__)
    static_assert(sizeof(thread->OsHandle) >= sizeof(pthread_t));
    if (pthread_create((pthread_t *restrict)thread->OsHandle, null, ThreadStartWrapper, thread) != 0)
    {
        Free(allocator, thread);
        return null;
    }
    return (PTHREAD)thread;
#else
#error "Not Implemented"
#endif
}
U64 ThreadId(VOID)
{
#if defined(_WIN32)
    return (U64)GetCurrentThreadId();
#elif defined(__linux__)
    return (U64)gettid();
#elif defined(__APPLE__)
    U64 tid;
    pthread_threadid_np(null, &tid);
    return tid;
#else
    #error "Not Implemented"
#endif
}
VOID JoinThread(PTHREAD thread)
{
#if defined(_WIN32)
    HANDLE handle = *(HANDLE *)thread->OsHandle;
    WaitForSingleObject(handle, INFINITE);
    CloseHandle(handle);
#elif defined(__unix__) || defined(__APPLE__)
    pthread_t handle = *(pthread_t *)thread->OsHandle;
    pthread_join(handle, null);
#else
    #error "Not Implemented"
#endif
    Free(&thread->Allocator, thread);
}
VOID ThreadSleep(U32 milliseconds)
{
#if defined(_WIN32)
    Sleep(milliseconds);
#elif defined(__unix__) || defined(__APPLE__)
    usleep(milliseconds * 1000);
#else 
    #error "Not Implemented"
#endif
}
VOID ThreadYield(VOID)
{
#if defined(_WIN32)
    SwitchToThread();
#elif defined(__unix__) || defined(__APPLE__)
    sched_yield();
#else 
    #error "Not Implemented"
#endif
}

// / / / / / / / / / / / / / / / / / / / 
// THREAD POOL                         /
// / / / / / / / / / / / / / / / / / / /
static VOID JobPoolWorker(PVOID data)
{
    PJOBPOOL pool = (PJOBPOOL)data;
    while (true)
    {
        MutexLock(pool->Mutex);
        while (pool->Running && pool->Head == pool->Tail)
        {
            ConditionVariableWait (pool->ConditionVariable, pool->Mutex);
        }
        if (!pool->Running && pool->Head == pool->Tail) // Finished and no work to do.
        {
            MutexUnlock(pool->Mutex);
            break;
        }

        PJOB job = pool->JobsRingbuffer[pool->Head];
        pool->Head = (pool->Head + 1) % pool->JobsRingbufferCapacity;
        MutexUnlock(pool->Mutex);

        // Run job outside lock
        job->Routine(job->UserData, job->Start, job->End);

        if (job->Next)
        {
            U32 remaining = DecU32(&job->Next->DependencyCount);
            if (!remaining)
            {
                // push continuation into the queue
                MutexLock(pool->Mutex);
                pool->JobsRingbuffer[pool->Tail] = job->Next;
                pool->Tail = (pool->Tail + 1) % pool->JobsRingbufferCapacity;
                IncU32(&pool->Pending);
                ConditionVariableSignal(pool->ConditionVariable);
                MutexUnlock(pool->Mutex);
            }
        }

        PCOUNTLATCH latch = job->Latch;
        if (latch)
            CountLatchDecrementSignalZero(latch);

        // Possibly push job successor into job ring buffer.
        PJOB next = job->Next;
        MutexLock(pool->Mutex);
        if (next and DecU32(&next->DependencyCount) == 0)
        {
            U32 tail = pool->Tail;
            U32 next = (tail + 1) % pool->JobsRingbufferCapacity;
            pool->JobsRingbuffer[tail] = job;
            pool->Tail = next;
            IncU32(&pool->Pending);
        }

        // Job is done.
        PoolDel(pool->JobPool, job);
        if (DecU32(&pool->Pending) == 0)
            ConditionVariableBroadcast(pool->ConditionVariable);

        MutexUnlock(pool->Mutex);
    }
}
PJOBPOOL LoadJobPool(ALLOCATOR allocator, U32 threads_count, U32 job_capacity)
{
    errdfs(6);

    PJOBPOOL job_pool;
    if (not (job_pool = Alloc(&allocator, sizeof(JOBPOOL), alignof(JOBPOOL))))
        return errdfflush(), null;
    memzero(job_pool);
    errdf(Free, &allocator, job_pool);
    job_pool->Allocator = allocator;
    job_pool->Running = true;
    // Load ConditionVariable to tell threads when a new job was enqueued
    if (not (job_pool->ConditionVariable = LoadConditionVariable(&allocator)))
        return errdfflush(), null;
    errdf(FreeConditionVariable, job_pool->ConditionVariable);
    // Load Mutex to synchronize job access
    if (not (job_pool->Mutex = LoadMutex(&allocator)))
        return errdfflush(), null;
    errdf(FreeMutex, job_pool->Mutex);
    // Load ringbuffer of handles to enqueued jobs.
    job_pool->JobsRingbufferCapacity = job_capacity;
    if (not (job_pool->JobsRingbuffer = (PPJOB)Alloc(&allocator, sizeof(PJOB) * job_capacity, alignof(PJOB))))
        return errdfflush(), null;
    errdf(Free, &allocator, job_pool->JobsRingbuffer);
    // Load pool of jobs 
    if (not (job_pool->JobPool = LoadPool(allocator, sizeof(JOB), (job_capacity + threads_count) * 2)))
        return errdfflush(), null;
    errdf(FreePool, job_pool->JobPool);
    // Load threads which run the jobs
    job_pool->ThreadsCount = threads_count;
    job_pool->Threads = (PPTHREAD)Alloc(&allocator, sizeof(PTHREAD) * threads_count, alignof(PTHREAD));
    U64 i;
    for (i = 0; i < threads_count; i++)
    {
        if (not (job_pool->Threads[i] = LoadThread(&allocator, JobPoolWorker, job_pool)))
            break; 
    }
    if (i != threads_count)
    {
        for (U64 j = 0; j < i; j++)
        {
            JoinThread(job_pool->Threads[j]);
        }
        return errdfflush(), null;
    }
    return job_pool;
}
VOID FreeJobPool(PJOBPOOL job_pool)
{
    MutexLock(job_pool->Mutex);
    job_pool->Running = false;
    ConditionVariableBroadcast(job_pool->ConditionVariable);
    MutexUnlock(job_pool->Mutex);
    // Free 
    for (U64 i = 0; i < job_pool->ThreadsCount; i++)
    {
        JoinThread(job_pool->Threads[i]);
    }
    FreePool(job_pool->JobPool);
    Free(&job_pool->Allocator, job_pool->JobsRingbuffer);
    FreeMutex(job_pool->Mutex);
    FreeConditionVariable(job_pool->ConditionVariable);
    Free(&job_pool->Allocator, job_pool);
}
VOID JobPoolWait(PJOBPOOL job_pool)
{
    MutexLock(job_pool->Mutex);
    while (job_pool->Pending)
    {
        ConditionVariableWait(job_pool->ConditionVariable, job_pool->Mutex);
    }
    MutexUnlock(job_pool->Mutex);
}
static BOOL JobPoolEnqueue(PJOBPOOL job_pool, PJOB job)
{
    MutexLock(job_pool->Mutex);
    while (((job_pool->Tail + 1) % job_pool->JobsRingbufferCapacity) == job_pool->Head)
    {
        MutexUnlock(job_pool->Mutex);
        ThreadYield();
        MutexLock(job_pool->Mutex);
    }

    U32 tail = job_pool->Tail;
    U32 next = (tail + 1) % job_pool->JobsRingbufferCapacity;
    job_pool->JobsRingbuffer[tail] = job;
    job_pool->Tail = next;

    IncU32(&job_pool->Pending);
    ConditionVariableSignal(job_pool->ConditionVariable);
    MutexUnlock(job_pool->Mutex);
    return true;
}
BOOL JobPoolDispatch(PJOBPOOL job_pool, PCOUNTLATCH latch, JOBROUTINE fn, PVOID data)
{
    PJOB job = PoolPut(job_pool->JobPool);
    if (!job)
        return false;
    job->Routine = fn;
    job->UserData = data;
    job->Start = 0;
    job->End = 1;
    job->Latch = latch;
    job->Next = null;
    StoreU32(&job->DependencyCount, 0);
    if (latch)
        CountLatchIncrement(latch);
    return JobPoolEnqueue(job_pool, job);
}
U64 JobPoolDispatchN(PJOBPOOL job_pool, PCOUNTLATCH latch, U64 work_count, U64 batch_size, JOBROUTINE fn, PVOID data)
{
    assert(batch_size > 0 and "You are trying to dispatch a batch size of 0.");
    
    U64 j = 0;
    for (U64 i = 0; i < work_count; i += batch_size)
    {
        PJOB job = (PJOB)PoolPut(job_pool->JobPool);
        if (!job)
        {
            return j;
        }
        U64 end = i + batch_size;
        if (end > work_count)
            end = work_count;
        job->Routine = fn;
        job->UserData = data;
        job->Start = i;
        job->End = end;
        job->Latch = latch;
        job->DependencyCount = 0;
        job->Next = null;
        if (latch)
            CountLatchIncrement(latch);
        JobPoolEnqueue(job_pool, job);
        ++j;
    }
    return j;
}

// / / / / / / / / / / / / / / / / / / / 
// JOB GRAPH                           /
// / / / / / / / / / / / / / / / / / / /
PJOBGRAPH LoadJobGraph(ALLOCATOR allocator, PJOBPOOL job_pool)
{
    errdfs(5);

    PJOBGRAPH graph;
    if (not (graph = Alloc(&allocator, sizeof(JOBGRAPH), alignof(JOBGRAPH))))
        return null;
    memzero(graph);
    errdf(Free, &allocator, graph);
    graph->Allocator = allocator;
    graph->JobPool   = job_pool;
    // Load latch for waiting on when jobgraph empty.
    if (not (graph->WaitLatch = LoadCountLatch(&allocator)))
        return errdfflush(), null;
    errdf(FreeCountLatch, graph->WaitLatch);
    // Only so much jobs can be enqueued into the job pool.
    graph->JobGroupsCapacity = job_pool->JobsRingbufferCapacity;
    if (not (graph->JobGroups = (PJOBGRAPHGROUP)Alloc(&allocator, sizeof(JOBGRAPHGROUP) * job_pool->JobsRingbufferCapacity, alignof(JOBGRAPHGROUP))))
        return errdfflush(), null;
    errdf(Free, &allocator, graph->JobGroups);
    // Same for this
    graph->StackCapacity = job_pool->JobsRingbufferCapacity;
    if (not (graph->Stack = (U32 *)Alloc(&allocator, sizeof(U32) * graph->StackCapacity, alignof(U32))))
        return errdfflush(), null;
    errdf(Free, &allocator, graph->Stack);
    // And for this
    graph->CommandsCapacity = job_pool->JobsRingbufferCapacity;
    if (not (graph->Commands = (PJOBCOMMAND)Alloc(&allocator, sizeof(JOBCOMMAND) * graph->CommandsCapacity, alignof(JOBCOMMAND))))
        return errdfflush(), null;
    errdf(Free, &allocator, graph->Commands);
    
    return graph;
}
VOID FreeJobGraph(PJOBGRAPH job_graph)
{
    Free(&job_graph->Allocator, job_graph->Commands);
    Free(&job_graph->Allocator, job_graph->JobStack);
    Free(&job_graph->Allocator, job_graph->JobGroups);
    Free(&job_graph->Allocator, job_graph->Stack);
    FreeCountLatch(job_graph->WaitLatch);
    Free(&job_graph->Allocator, job_graph);
}
VOID JobGraphClear(PJOBGRAPH job_graph)
{
    //CountLatchWait(job_graph->WaitLatch, 0);
    job_graph->CommandsCount  = 0;
    job_graph->JobsStackCount = 0;
    job_graph->JobGroupsCount = 0;
    job_graph->StackCount     = 0;
}
VOID JobGraphRun(PJOBGRAPH job_graph)
{
    if (job_graph->CommandsCount == 0)
        return;

    job_graph->JobsStackCount = job_graph->CommandsCount;
    if (job_graph->JobsStackCapacity < job_graph->CommandsCount)
    {
        job_graph->JobsStackCapacity = job_graph->CommandsCount;
        job_graph->JobStack = Realloc(&job_graph->Allocator, job_graph->JobStack, sizeof(PJOB) * (U32)(job_graph->JobsStackCapacity * 1.5f), alignof(PJOB));
    }

    for (U32 i = 0; i < job_graph->CommandsCount; i++)
    {
        PJOBCOMMAND cmd = &job_graph->Commands[i];

        PJOB job             = (PJOB)PoolPut(job_graph->JobPool->JobPool);
        job->Routine         = cmd->Routine;
        job->UserData        = cmd->UserData;
        job->Start           = cmd->Start;
        job->End             = cmd->End;
        job->Next            = null;
        StoreU32(&job->DependencyCount, 0);
        job->Latch           = job_graph->WaitLatch;
        IncU32(&job_graph->WaitLatch->Pending);
        job_graph->JobStack[i] = job;
    }

    for (U32 group = 0; group < job_graph->JobGroupsCount - 1; group++) // Wire groups
    {
        PJOBGRAPHGROUP current = &job_graph->JobGroups[group];
        PJOBGRAPHGROUP next = &job_graph->JobGroups[group + 1];

        PJOB gate = job_graph->JobStack[next->FirstJobIndex];
        for (U32 j = 0; j < current->JobCount; j++)
        {
            PJOB job = job_graph->JobStack[current->FirstJobIndex + j];
            job->Next = gate;
        }
        StoreU32(&gate->DependencyCount, current->JobCount);
    }

    PJOBGRAPHGROUP root = &job_graph->JobGroups[0];
    for (U32 i = 0; i < root->JobCount; i++) // Submit root
        JobPoolEnqueue(job_graph->JobPool, job_graph->JobStack[root->FirstJobIndex + i]);
}
VOID JobGraphDispatch(PJOBGRAPH job_graph, JOBROUTINE routine, PVOID user_data)
{
    PJOBCOMMAND cmd = &job_graph->Commands[job_graph->CommandsCount++];
    cmd->Routine    = routine;
    cmd->UserData   = user_data;
    cmd->Start      = 0;
    cmd->End        = 1;
}
VOID JobGraphDispatchN(PJOBGRAPH job_graph, U64 work_count, U64 batch_size, JOBROUTINE routine, PVOID user_data)
{
    for (U64 i = 0; i < work_count; i += batch_size)
    {
        U64 end = i + batch_size;
        if (end > work_count)
            end = work_count;

        PJOBCOMMAND cmd = &job_graph->Commands[job_graph->CommandsCount++];
        cmd->Routine    = routine;
        cmd->UserData   = user_data;
        cmd->Start      = i;
        cmd->End        = end;
    }
}
VOID JobGraphPut(PJOBGRAPH job_graph)
{
    job_graph->Stack[job_graph->StackCount++] = job_graph->JobsStackCount;
}
VOID JobGraphPop(PJOBGRAPH job_graph)
{
    U64 start = job_graph->Stack[--job_graph->StackCount];
    PJOBGRAPHGROUP group = &job_graph->JobGroups[job_graph->JobGroupsCount++];
    group->FirstJobIndex = (U32)start;
    group->JobCount = (U32)(job_graph->CommandsCount - start);
}
// / / / / / / / / / / / / / / / / / / / 
// ATOMICS                             /
// / / / / / / / / / / / / / / / / / / /
U32 LoadU32(volatile U32 *a)
{
#if defined(_MSC_VER)
    return (U32)InterlockedCompareExchange((volatile long *)a, 0, 0);
#else
    return __atomic_load_n(a, __ATOMIC_SEQ_CST);
#endif
}
VOID StoreU32(volatile U32 *a, U32 v)
{
#if defined(_MSC_VER)
    InterlockedExchange((volatile long *)a, v);
#else
    __atomic_store_n(a, v, __ATOMIC_SEQ_CST);
#endif
}
U32 AddU32(volatile U32 *a, U32 v)
{
#if defined(_MSC_VER)
    return InterlockedExchangeAdd((volatile long *)a, v);
#else
    return __atomic_fetch_add(a, v, __ATOMIC_SEQ_CST);
#endif
}
U32 SubU32(volatile U32 *a, U32 v)
{
#if defined(_MSC_VER)
    return InterlockedExchangeAdd((volatile long *)a, -(long)v);
#else
    return __atomic_fetch_sub(a, v, __ATOMIC_SEQ_CST);
#endif
}
U32 IncU32(volatile U32 *a)
{
#if defined(_MSC_VER)
    return InterlockedIncrement((volatile long *)a);
#else
    return __atomic_add_fetch(a, 1, __ATOMIC_SEQ_CST);
#endif
}
U32  DecU32(volatile U32 *a)
{
#if defined(_MSC_VER)
    return InterlockedDecrement((volatile long *)a);
#else
    return __atomic_sub_fetch(a, 1, __ATOMIC_SEQ_CST);
#endif
}
U32 ExchangeU32(volatile U32 *a, U32 v)
{
#if defined(_MSC_VER)
    return InterlockedExchange((volatile long *)a, v);
#else
    return __atomic_exchange_n(a, v, __ATOMIC_SEQ_CST);
#endif
}
BOOL CASU32(volatile U32 *a, U32 *expected, U32 desired)
{
#if defined(_MSC_VER)
    U32 old = (U32)InterlockedCompareExchange((volatile long *)a, (long)desired, (long)*expected);
    if (old == *expected) // Success
        return true;
    *expected = old;
    return false;
#else
    return __atomic_compare_exchange_n(a, expected, desired, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#endif
}

PVOID LoadPtr(PVOID volatile *a)
{
#if defined(_MSC_VER)
    return InterlockedCompareExchangePointer(a, NULL, NULL);
#else
    return __atomic_load_n(a, __ATOMIC_SEQ_CST);
#endif
}
VOID  StorePtr(PVOID volatile *a, PVOID v)
{
#if defined(_MSC_VER)
    InterlockedExchangePointer(a, v);
#else
    __atomic_store_n(a, v, __ATOMIC_SEQ_CST);
#endif
}
PVOID ExchangePtr(PVOID volatile *a, PVOID v)
{
#if defined(_MSC_VER)
    return InterlockedExchangePointer(a, v);
#else
    return __atomic_exchange_n(a, v, __ATOMIC_SEQ_CST);
#endif
}
BOOL CASPtr(PVOID volatile *a, PPVOID expected, PVOID desired)
{
#if defined(_MSC_VER)
    void *old = InterlockedCompareExchangePointer(a, desired, *expected);
    if (old == *expected)
        return true;
    *expected = old;
    return false;
#else
    return __atomic_compare_exchange_n(a, expected, desired, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#endif
}