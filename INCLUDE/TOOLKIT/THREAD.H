#pragma once

#include <TOOLKIT/ALLOCATOR.H>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

makeopaque(MUTEX);
makeopaque(COUNTLATCH);
makeopaque(CONDITIONVARIABLE);
makeopaque(JOB);
makeopaque(JOBPOOL);
makeopaque(JOBCOMMAND);
makeopaque(JOBGRAPH);
makeopaque(JOBGRAPHGROUP);

typedef struct MUTEX
{
    U8        OsHandle[64];
    ALLOCATOR Allocator;
} MUTEX;

typedef struct CONDITIONVARIABLE
{
    U8        OsHandle[64];
    ALLOCATOR Allocator;
} CONDITIONVARIABLE;

typedef struct COUNTLATCH
{
    PMUTEX             Mutex;
    PCONDITIONVARIABLE ConditionVariable;
    U32                Pending;
    ALLOCATOR          Allocator;
} COUNTLATCH;

typedef VOID(*ROUTINE)(PVOID UserData);
typedef struct THREAD
{
    U8        OsHandle[16];
    ROUTINE   Routine;
    PVOID     UserData;
    ALLOCATOR Allocator;
} THREAD, *PTHREAD, **PPTHREAD;

typedef VOID (*JOBROUTINE)(PVOID UserData, U64 Start, U64 End);
typedef struct JOB
{
    JOBROUTINE   Routine;
    PVOID        UserData; 
    U64          Start, End;
    PCOUNTLATCH  Latch;
    PJOB         Next;
    U32          DependencyCount;
} JOB;

typedef struct JOBPOOL
{
    ALLOCATOR Allocator;

    PPTHREAD Threads;
    U32      ThreadsCount;

    PPJOB JobsRingbuffer;
    U32   JobsRingbufferCapacity;
    U32 Head, Tail;

    PPOOL JobPool;
    U32 Pending;
    
    PMUTEX             Mutex;
    PCONDITIONVARIABLE ConditionVariable;
    
    BOOL Running;
} JOBPOOL;

typedef struct JOBCOMMAND
{
    JOBROUTINE   Routine;
    PVOID        UserData;
    U64          Start, End;
} JOBCOMMAND;

typedef struct JOBGRAPHGROUP
{
    U32 FirstJobIndex;
    U32 JobCount;
} JOBGRAPHGROUP;

typedef struct JOBGRAPH
{
    ALLOCATOR Allocator;
    PJOBPOOL JobPool;
    PCOUNTLATCH   WaitLatch;
    // Commands
    PJOBCOMMAND Commands;
    U32         CommandsCapacity, CommandsCount;
    // Jobs
    PPJOB JobStack;
    U32   JobsStackCapacity, JobsStackCount;
    // Stack
    U32 *Stack;
    U32  StackCapacity, StackCount;
    // Job Groups
    PJOBGRAPHGROUP JobGroups;
    U32 JobGroupsCapacity, JobGroupsCount;
} JOBGRAPH;

////////////////////////////////////////
// MULTI THREADING INTERFACE          //
////////////////////////////////////////
// / / / / / / / / / / / / / / / / / / / 
// MUTEX (LOCK)                        /
// / / / / / / / / / / / / / / / / / / /
PMUTEX LoadMutex(PALLOCATOR allocator);
VOID   FreeMutex(PMUTEX mutex);
VOID   MutexLock(PMUTEX mutex);
VOID   MutexUnlock(PMUTEX mutex);
BOOL   MutexTryLock(PMUTEX mutex);
BOOL   MutexTimedLock(PMUTEX mutex, U64 timeout_ms);
// / / / / / / / / / / / / / / / / / / / 
// CONDITION VARIABLE                  /
// / / / / / / / / / / / / / / / / / / /
PCONDITIONVARIABLE LoadConditionVariable(PALLOCATOR allocator);
VOID               FreeConditionVariable(PCONDITIONVARIABLE condition_variable);
VOID               ConditionVariableWait(PCONDITIONVARIABLE condition_variable, PMUTEX mutex);
VOID               ConditionVariableSignal(PCONDITIONVARIABLE condition_variable);
VOID               ConditionVariableBroadcast(PCONDITIONVARIABLE condition_variable);
// / / / / / / / / / / / / / / / / / / / 
// COUNT LATCH                         /
// / / / / / / / / / / / / / / / / / / /
PCOUNTLATCH LoadCountLatch(PALLOCATOR allocator);
VOID        FreeCountLatch(PCOUNTLATCH latch);
VOID        CountLatchWait(PCOUNTLATCH latch, U64 expected);
U64         CountLatchIncrement(PCOUNTLATCH latch);
U64         CountLatchDecrement(PCOUNTLATCH latch);
U64         CountLatchDecrementSignalZero(PCOUNTLATCH latch);
// / / / / / / / / / / / / / / / / / / / 
// THREADS                             /
// / / / / / / / / / / / / / / / / / / /
PTHREAD LoadThread(PALLOCATOR allocator, ROUTINE routine, PVOID user_data);
VOID    JoinThread(PTHREAD thread);
U64     ThreadId(VOID);
VOID    ThreadSleep(U32 milliseconds);
VOID    ThreadYield(VOID);
// / / / / / / / / / / / / / / / / / / / 
// THREAD POOL                         /
// / / / / / / / / / / / / / / / / / / /
PJOBPOOL LoadJobPool(ALLOCATOR allocator, U32 threads_count, U32 job_capacity);
VOID     FreeJobPool(PJOBPOOL job_pool);
VOID     JobPoolWait(PJOBPOOL job_pool);
BOOL     JobPoolDispatch(PJOBPOOL job_pool, PCOUNTLATCH latch, JOBROUTINE fn, PVOID data);
U64      JobPoolDispatchN(PJOBPOOL job_pool, PCOUNTLATCH latch, U64 work_count, U64 batch_size, JOBROUTINE fn, PVOID data);
// / / / / / / / / / / / / / / / / / / / 
// JOB GRAPH                           /
// / / / / / / / / / / / / / / / / / / /
PJOBGRAPH LoadJobGraph(ALLOCATOR allocator, PJOBPOOL job_pool);
VOID      FreeJobGraph(PJOBGRAPH job_graph);
VOID      JobGraphClear(PJOBGRAPH job_graph);
VOID      JobGraphRun(PJOBGRAPH job_graph);
VOID      JobGraphDispatch(PJOBGRAPH job_graph, JOBROUTINE routine, PVOID user_data);
VOID      JobGraphDispatchN(PJOBGRAPH job_graph, U64 work_count, U64 batch_size, JOBROUTINE routine, PVOID user_data);
VOID      JobGraphPut(PJOBGRAPH job_graph);
VOID      JobGraphPop(PJOBGRAPH job_graph);
// / / / / / / / / / / / / / / / / / / / 
// ATOMICS                             /
// / / / / / / / / / / / / / / / / / / /
// U32
U32  LoadU32(volatile U32 *a);
VOID StoreU32(volatile U32 *a, U32 v);
U32  AddU32(volatile U32 *a, U32 v);
U32  SubU32(volatile U32 *a, U32 v);
U32  IncU32(volatile U32 *a);
U32  DecU32(volatile U32 *a);
U32  ExchangeU32(volatile U32 *a, U32 v);
BOOL CASU32(volatile U32 *a, U32 *expected, U32 desired);
// PVOID
PVOID LoadPtr(PVOID volatile *a);
VOID  StorePtr(PVOID volatile *a, PVOID v);
PVOID ExchangePtr(PVOID volatile *a, PVOID v);
BOOL  CASPtr(PVOID volatile *a, PPVOID expected, PVOID desired);

#ifdef __cplusplus
}
#endif