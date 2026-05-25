#pragma once

#include <TOOLKIT/TYPES.H>

#ifdef __cplusplus
extern "C" {
#endif

makeopaque(TLSF);

// / / / / / / / / / / / / / / / / / / / 
// ALLOCATOR                           /
// / / / / / / / / / / / / / / / / / / /
PVOID Alloc(PALLOCATOR allocator, U64 size, U64 alignment);
PVOID Calloc(PALLOCATOR allocator, U64 size, U64 alignment);
#define New(allocator, type) \
    ((typeof(type) *)Alloc(&(allocator), sizeof(type), alignof(type)))
#define NewArr(allocator, type, n) \
    ((typeof(type) *)Alloc(&(allocator), sizeof(type) * (n), alignof(type)))
#define CNew(allocator, type) \
    ((typeof(type) *)Calloc(&(allocator), sizeof(type), alignof(type)))
#define CNewArr(allocator, type, n) \
    ((typeof(type) *)Calloc(&(allocator), sizeof(type) * (n), alignof(type)))

PVOID Realloc(PALLOCATOR allocator, PVOID ptr, U64 size, U64 alignment);
VOID  Free(PALLOCATOR allocator, PVOID ptr);
FSTR  Strdup(PALLOCATOR allocator, FSTR string);
VOID  Strfree(PALLOCATOR allocator, FSTR string);

ALLOCATOR DefaultAllocator(void);
ALLOCATOR PoolAllocator(PPOOL pool);
ALLOCATOR ArenaAllocator(PARENA arena);

// / / / / / / / / / / / / / / / / / / / 
// ARENA                               /
// / / / / / / / / / / / / / / / / / / /
typedef struct ARENABLOCK
{
    struct ARENABLOCK *Next;
    U64 Used, Size;
    U8  Data[];
} ARENABLOCK, *PARENABLOCK, **PPARENABLOCK;
typedef struct ARENA
{
    ALLOCATOR   Allocator;
    U64         DefaultBlockSize;
    PARENABLOCK Head, Current;
} ARENA, *PARENA, **PPARENA;
typedef struct SNAPSHOT
{
    PARENABLOCK Block;
    U64         Used;
} SNAPSHOT;

PARENA   LoadArena(ALLOCATOR allocator, U64 default_block_size);
VOID     FreeArena(PARENA arena);
VOID     ArenaClear(PARENA arena);
PVOID    ArenaPut(PARENA arena, U64 size, U64 alignment);
PVOID    ArenaPutz(PARENA arena, U64 size, U64 alignment);
SNAPSHOT ArenaSnap(PARENA arena);
VOID     ArenaRewind(PARENA arena, SNAPSHOT snapshot);
FSTR     ArenaStrdup(PARENA arena, FSTR string);


// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / /
// FIXED-SIZE THREAD-SAFE OBJECT POOL                                          /
//                                                                             /
// :example  PPOOL P = LoadPool(allocator, sizeof(MyStruct), 64);              /
//           MyStruct *Obj = PoolPut(P);                                       /
//           PoolDel(P, Obj);                                                  /
//           FreePool(P);                                                      /
//                                                                             /
// :optional PER-THREAD STASH                                                  /
//   Define POOL_THREAD_CACHE_SIZE to a positive integer.. It has no effect in /
//   consumer translation units and does not need to be defined there.         /
//                                                                             /
//   #define POOL_THREAD_CACHE_SIZE 32                                         /
//                                                                             /
//   Windows note: Win32 TLS has no thread-exit destructor. If you use         /
//   POOL_THREAD_CACHE_SIZE on Windows you MUST call PoolFlushThreadCache(P)   /
//   at the end of every thread that uses the pool, otherwise objects sitting  /
//   in the stash will leak until FreePool(P) is called.                       /
//   On pthreads (Linux/macOS) this is handled automatically.                  /
// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / /
makeopaque(POOL);
makeopaque(POOLITER);
makeopaque(POOLSTATS);

// / / / / / / / / / / / / / / / / / / /
// STATS ABOUT THE POOL STATE          /
// / / / / / / / / / / / / / / / / / / /
struct POOLSTATS
{
    U64 ObjSize;
    U64 BlockCap;
    U64 BlockCount;
    U64 TotalSlots;
    U64 UsedSlots;
    U64 FreeSlots;
    U64 TotalAllocs;
    U64 TotalFrees;
    U64 MemoryBytes;
};

// / / / / / / / / / / / / / / / / / / / 
// Traverses live objects in order.    /
//                                     /
// Pool is locked for the traversal.   /
// You MUST call PoolEnd() when done.  /
// While iterating, do NOT call        /
// PoolPut/PoolDel/ClearPool.          / 
// / / / / / / / / / / / / / / / / / / / 
struct POOLITER
{
    PPOOL  Pool;
    PVOID  Block;
    I64    Slot;
    I32    List;
};

//// ALLOCATOR LIFETIME ////////////////
PPOOL LoadPool(ALLOCATOR allocator, U64 ObjSize, U64 BlockCap);
VOID  FreePool(PPOOL P);
VOID  ClearPool(PPOOL P);
VOID  TrimPool(PPOOL P);
//// ALLOCATE/FREE /////////////////////
VOID *PoolPut(PPOOL P);          /* allocate one object (uninitialised)  */
VOID *PoolPutz(PPOOL P);          /* allocate one object (zeroed)         */
VOID  PoolDel(PPOOL P, PVOID Ptr);
//// DIAGNOSTICS ///////////////////////
VOID  PoolStats(const PPOOL P, PPOOLSTATS Out);
BOOL  PooSameAlignedObjSize(const PPOOL P, U64 ObjSize); // To catch wrong size allocation early e.g. Assert(PoolPerhapsObjSize(pool, sizeof(MyStruct)));

// / / / / / / / / / / / / / / / / / / / 
// Drain this thread's stash back into the pool.
//
// Required on Windows when POOL_THREAD_CACHE_SIZE is defined: call at the
// end of every thread that uses the pool.
// On pthreads this is called automatically on thread exit.
// Safe to call even when POOL_THREAD_CACHE_SIZE is not defined (no-op).
// / / / / / / / / / / / / / / / / / / / 
VOID  PoolFlushThreadCache(PPOOL P);
//// FORWARD ITERATION /////////////////
VOID  PoolBegin(PPOOL P, PPOOLITER It);
PVOID PoolNext(PPOOLITER It);
VOID  PoolEnd(PPOOLITER It);
//// REVERSE ITERATION /////////////////
VOID  PoolBeginRev(PPOOL P, PPOOLITER It);
PVOID PoolNextRev(PPOOLITER It);
VOID  PoolEndRev(PPOOLITER It);

// / / / / / / / / / / / / / / / / / / / 
// TLSF                                /
// / / / / / / / / / / / / / / / / / / /

#ifdef __cplusplus
}
#endif
