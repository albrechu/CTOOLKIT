#include <TOOLKIT/ALLOCATOR.H>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if defined(_WIN32)
#	define WIN32_LEAN_AND_MEAN
#   include <windows.h>

#   define POOL_MUTEX            CRITICAL_SECTION
#   define POOL_MUTEX_INIT(M)    InitializeCriticalSection(&(M))
#   define POOL_MUTEX_DESTROY(M) DeleteCriticalSection(&(M))
#   define POOL_MUTEX_LOCK(M)    EnterCriticalSection(&(M))
#   define POOL_MUTEX_UNLOCK(M)  LeaveCriticalSection(&(M))

#   define POOL_TLS_KEY          DWORD
#   define POOL_TLS_CREATE(K)    ((K) = TlsAlloc(), (K) == TLS_OUT_OF_INDEXES ? -1 : 0)
#   define POOL_TLS_DELETE(K)    TlsFree(K)
#   define POOL_TLS_GET(K)       TlsGetValue(K)
#   define POOL_TLS_SET(K, V)    TlsSetValue((K), (V))
#   define POOL_TLS_NO_DESTRUCTOR
#else
#   include <pthread.h>

#   define POOL_MUTEX            pthread_mutex_t
#   define POOL_MUTEX_INIT(M)    pthread_mutex_init(&(M), null)
#   define POOL_MUTEX_DESTROY(M) pthread_mutex_destroy(&(M))
#   define POOL_MUTEX_LOCK(M)    pthread_mutex_lock(&(M))
#   define POOL_MUTEX_UNLOCK(M)  pthread_mutex_unlock(&(M))

#   define POOL_TLS_KEY          pthread_key_t
#   define POOL_TLS_CREATE(K)    pthread_key_create(&(K), PoolThreadCacheDestroyCallback)
#   define POOL_TLS_DELETE(K)    pthread_key_delete(K)
#   define POOL_TLS_GET(K)       pthread_getspecific(K)
#   define POOL_TLS_SET(K, V)    pthread_setspecific((K), (V))
#endif


PVOID Alloc(PALLOCATOR allocator, U64 size, U64 alignment)
{
    return allocator->alloc(allocator->user_data, size, alignment);
}
PVOID Calloc(PALLOCATOR allocator, U64 size, U64 alignment)
{
    PVOID data = allocator->alloc(allocator->user_data, size, alignment);
    return data ? memset(data, 0, size) : null;
}
PVOID Realloc(PALLOCATOR allocator, PVOID ptr, U64 size, U64 alignment)
{
    return allocator->realloc(allocator->user_data, ptr, size, alignment);
}
VOID  Free(PALLOCATOR allocator, PVOID ptr)
{
    allocator->free(allocator->user_data, ptr);
}
FSTR Strdup(PALLOCATOR allocator, FSTR string)
{
    FSTR r = string;
	r.str = Alloc(allocator, sizeof(CHAR) * string.size, alignof(CHAR));
    if (!r.str)
        return FSTR_INVALID;
    memcpy(r.str, string.str, string.size);
    return r;
}
VOID Strfree(PALLOCATOR allocator, FSTR string)
{
    Free(allocator, (PVOID)string.str);
}

// Default Allocator
static U64 Aligned(U64 size, U64 align)
{
    return ((size)+((align)-1)) & ~((align)-1);
}
PVOID DefaultAlloc(PVOID user_data, U64 size, U64 alignment)
{
    _ = user_data;
    return malloc(Aligned(size, alignment));
}
PVOID DefaultRealloc(PVOID user_data, PVOID ptr, U64 size, U64 alignment)
{
    _ = user_data;
    return realloc(ptr, Aligned(size, alignment));
}
VOID DefaultFree(PVOID user_data, PVOID ptr)
{
    _ = user_data;
    free(ptr);
}
ALLOCATOR DefaultAllocator(VOID)
{
    return LIT(ALLOCATOR, .user_data = null, .alloc = DefaultAlloc, .realloc = DefaultRealloc, .free = DefaultFree);
}

PVOID PoolAlloc(PVOID user_data, U64 size, U64 alignment)
{
    _ = alignment;
    PPOOL pool = (PPOOL)user_data;
    assert(PooSameAlignedObjSize(pool, size) and "You are trying to allocate an object type that differs in size from the pool object size.");
    return PoolPut(pool);
}
PVOID PoolRealloc(PVOID user_data, PVOID ptr, U64 size, U64 alignment)
{
    _ = user_data;
    _ = ptr;
    _ = size;
    _ = alignment;
    return null;
}
VOID PoolFree(PVOID user_data, PVOID ptr)
{
    PoolDel((PPOOL)user_data, ptr);
}

ALLOCATOR PoolAllocator(PPOOL pool)
{
    return LIT(ALLOCATOR, .user_data = pool, .alloc = PoolAlloc, .realloc = PoolRealloc, .free = PoolFree);
}

PVOID ArenaRealloc(PVOID user_data, PVOID ptr, U64 size, U64 alignment)
{
    _ = user_data;
    _ = ptr;
    _ = size;
    _ = alignment;
    return null;
}
VOID ArenaFree(PVOID user_data, PVOID ptr)
{
    _ = user_data;
    _ = ptr;
}
ALLOCATOR ArenaAllocator(PARENA arena)
{
    return LIT(ALLOCATOR, .user_data = arena, .alloc = (ALLOC)ArenaPut, .realloc = ArenaRealloc, .free = ArenaFree);
}

// / / / / / / / / / / / / / / / / / / / 
// ARENA                               /
// / / / / / / / / / / / / / / / / / / /
PARENA LoadArena(ALLOCATOR allocator, U64 block_size)
{
    PARENA arena;
    if (not (arena = Alloc(&allocator, sizeof(ARENA), alignof(ARENA)))) return null;
    arena->Allocator = allocator;
    arena->DefaultBlockSize = block_size ? block_size : (64 * 1024);
    return arena;
}
VOID FreeArena(PARENA arena)
{
    assert(arena != null and "Freeing a invalid arena.");
    PARENABLOCK block = arena->Head;
    while (block)
    {
        PARENABLOCK next = block->Next;
        Free(&arena->Allocator, block);
        block = next;
    }
    Free(&arena->Allocator, arena);
}
VOID ArenaClear(PARENA arena)
{
    if (arena->Head)
    {
        PARENABLOCK block = arena->Head->Next;
        while (block)
        {
            PARENABLOCK next = block->Next;
            Free(&arena->Allocator, block);
            block = next;
        }
        arena->Head->Next = null;
        arena->Head->Used = 0;
        arena->Current    = arena->Head;
    }
}
static PVOID ArenaAlignedAlloc(PARENA arena, U64 size, U64 align)
{
    if (align < sizeof(PVOID)) align = sizeof(PVOID);
    
    PARENABLOCK block = arena->Current;
    U64 used_aligned = block ? Aligned(block->Used, align) : 0;
    if ((not block) or ((used_aligned + size) > block->Size)) // No block or insufficient size?
    {
        U64 block_size = (size > arena->DefaultBlockSize) ? size : arena->DefaultBlockSize;
        // Allocate new block
        PARENABLOCK new_block = null;
        if (not (new_block = (PARENABLOCK)Alloc(&arena->Allocator, sizeof(ARENABLOCK) + block_size, alignof(ARENABLOCK))))
            return null;
        *new_block = LIT(ARENABLOCK, .Size = block_size);
        // First block in arena
        if (not arena->Head)
            arena->Head = new_block;

        if (arena->Current)
            arena->Current->Next = new_block;
        arena->Current = new_block;
        block = new_block;
        used_aligned = 0; // New block is empty
    }
    PVOID aligned_data = block->Data + used_aligned;
    block->Used = used_aligned + size;
    return aligned_data;
}
PVOID ArenaPut(PARENA arena, U64 size, U64 alignment)
{
    return ArenaAlignedAlloc(arena, size, alignment);
}
PVOID ArenaPutz(PARENA arena, U64 size, U64 alignment)
{
    PVOID data = ArenaAlignedAlloc(arena, size, alignment);
    return data ? memset(data, 0, size) : null;
}
SNAPSHOT ArenaSnap(PARENA arena)
{
    return LIT(SNAPSHOT, .Block = arena->Current, .Used = arena->Current ? arena->Current->Used : 0);
}
VOID ArenaRewind(PARENA arena, SNAPSHOT snapshot)
{
    if (snapshot.Block)
    {
        PARENABLOCK block = snapshot.Block->Next;
        while (block)
        {
            PARENABLOCK next = block->Next;
            Free(&arena->Allocator, block);
            block = next;
        }
        snapshot.Block->Next = null;
        snapshot.Block->Used = snapshot.Used;
        arena->Current = snapshot.Block;
    }
    else // Reset to null.
        ArenaClear(arena);
}
FSTR ArenaStrdup(PARENA arena, FSTR string)
{
    U64 size = string.size + 1;
    FSTR copy = fstr((PCHAR)ArenaPut(arena, size, sizeof(CHAR)), size);
    if (fstrinvalid(copy)) return FSTR_INVALID;
    memcpy(copy.str, string.str, string.size);
    copy.str[string.size] = '\0';
    return copy;
}

// / / / / / / / / / / / / / / / / / / / 
// POOL                                /
// / / / / / / / / / / / / / / / / / / /
#define POOL_ALIGN         sizeof(PVOID)
#define POOL_DEFAULT_BYTES 4096

makeopaque(POOLBLOCK);
makeopaque(THREADCACHE);

struct POOLBLOCK
{
	PPOOLBLOCK Next;
	PPOOLBLOCK Prev;
	PVOID      Freelist;
	U64        FreeCount;
	U64        Capacity;
};

#ifdef POOL_THREAD_CACHE_SIZE
struct THREADCACHE
{
	POOL *Owner;
	I32    Count;
	void *Slots[POOL_THREAD_CACHE_SIZE];
};
#endif

struct POOL
{
	ALLOCATOR Allocator;
	U64       ObjSize;
	U64       BlockCap;

	POOL_MUTEX  Lock;
	PPOOLBLOCK Partial;
	PPOOLBLOCK Full;
	PPOOLBLOCK Empty;
	U64      TotalAllocs;
	U64      TotalFrees;
	U64      BlockCount;

#ifdef POOL_THREAD_CACHE_SIZE
	POOL_TLS_KEY ThreadKey;
#endif
};

static inline U64 PoolSlotStride(U64 ObjSize) 
{
	return sizeof(PPOOLBLOCK) + ObjSize;
}
static inline PPOOLBLOCK PoolOwnerOf(PVOID User) 
{
	return *(PPOOLBLOCK *)((PCHAR)User - sizeof(PPOOLBLOCK));
}
static inline PVOID PoolToUser(PVOID Raw) 
{
	return (PCHAR)Raw + sizeof(PPOOLBLOCK);
}
static inline PVOID PoolToRaw(PVOID User) 
{
	return (PCHAR)User - sizeof(PPOOLBLOCK);
}

static void PoolListPush(PPOOLBLOCK *Head, PPOOLBLOCK B)
{
	B->Next = *Head;
	B->Prev = null;
	if (*Head)
		(*Head)->Prev = B;
	*Head = B;
}

static void PoolListRemove(PPOOLBLOCK *Head, PPOOLBLOCK B) 
{
	if (B->Prev)
		B->Prev->Next = B->Next;
	else
		*Head = B->Next;
	if (B->Next)
		B->Next->Prev = B->Prev;
	B->Next = B->Prev = null;
}

static PPOOLBLOCK PoolBlockCreate(PPOOL P, U64 ObjSize, U64 BlockCap)
{
	U64 Stride = PoolSlotStride(ObjSize);
	U64 Total = Aligned(sizeof(POOLBLOCK), POOL_ALIGN) + Stride * BlockCap;
	Total = Aligned(Total, POOL_ALIGN);

	PPOOLBLOCK B = (PPOOLBLOCK)Alloc(&P->Allocator, Total, POOL_ALIGN);
	if (!B) return null;

	B->Next = null;
	B->Prev = null;
	B->Capacity = BlockCap;
	B->FreeCount = BlockCap;

	PCHAR Base = (PCHAR)B + Aligned(sizeof(POOLBLOCK), POOL_ALIGN);
	for (U64 I = 0; I < BlockCap; I++)
	{
		PVOID Raw = Base + Stride * I;
		PVOID RawNxt = (I + 1 < BlockCap) ? Base + Stride * (I + 1) : null;
		*(PPPOOLBLOCK)Raw = B;
		*(PPVOID)PoolToUser(Raw) = RawNxt;
	}
	B->Freelist = Base;
	return B;
}

static BOOL PoolSlotIsFree(PPOOLBLOCK Block, U64 ObjSize, U64 Slot)
{
	U64 Stride = PoolSlotStride(ObjSize);
	PVOID Target = (PCHAR)Block + Aligned(sizeof(POOLBLOCK), POOL_ALIGN) + Stride * Slot;
	PVOID Cursor = Block->Freelist;
	while (Cursor)
	{
		if (Cursor == Target)
			return true;
		Cursor = *(PPVOID)PoolToUser(Cursor);
	}
	return false;
}

#ifdef POOL_THREAD_CACHE_SIZE
static void ThreadCacheDrain(PPOOL P, PTHREADCACHE TC) {
	POOL_MUTEX_LOCK(P->Lock);
	for (I32 I = 0; I < TC->Count; I++) {
		void *Raw = TC->Slots[I];
		PPOOLBLOCK B = PoolOwnerOf(PoolToUser(Raw));

		if (B->FreeCount == 0) {
			PoolListRemove(&P->Full, B);
			PoolListPush(&P->Partial, B);
		}

		*(PPVOID)PoolToUser(Raw) = B->Freelist;
		B->Freelist = Raw;
		B->FreeCount++;

		if (B->FreeCount == B->Capacity) {
			PoolListRemove(&P->Partial, B);
			PoolListPush(&P->Empty, B);
		}

		P->TotalFrees++;
	}
	TC->Count = 0;
	POOL_MUTEX_UNLOCK(P->Lock);
}

#ifndef POOL_TLS_NO_DESTRUCTOR
void PoolThreadCacheDestroyCallback(PVOID Arg) {
	PTHREADCACHE TC = (PTHREADCACHE)Arg;
	if (!TC)
		return;
	ThreadCacheDrain(TC->Owner, TC);
	Free(&TC->Owner->Allocator, TC);
}
#endif

static PTHREADCACHE ThreadCacheGet(PPOOL P)
{
	PTHREADCACHE TC = (PTHREADCACHE)POOL_TLS_GET(P->ThreadKey);
	if (!TC)
	{
		TC = Alloc(&P->Allocator, sizeof(THREADCACHE), alignof(THREADCACHE));
		if (!TC)
			return null;
		TC->Owner = P;
		POOL_TLS_SET(P->ThreadKey, TC);
	}
	return TC;
}

static BOOL ThreadCacheReload(PPOOL P, PTHREADCACHE TC)
{
	I32 Target = POOL_THREAD_CACHE_SIZE / 2;

	POOL_MUTEX_LOCK(P->Lock);

	while (TC->Count < Target)
	{
		PPOOLBLOCK B = P->Partial;
		if (!B)
		{
			B = P->Empty;
			if (B) {
				PoolListRemove(&P->Empty, B);
				PoolListPush(&P->Partial, B);
			}
			else {
				POOL_MUTEX_UNLOCK(P->Lock);
				B = PoolBlockCreate(P->ObjSize, P->BlockCap);
				POOL_MUTEX_LOCK(P->Lock);
				if (!B) break;
				PoolListPush(&P->Partial, B);
				P->BlockCount++;
			}
		}

		PVOID Raw = B->Freelist;
		B->Freelist = *(PPVOID)PoolToUser(Raw);
		B->FreeCount--;

		if (B->FreeCount == 0) {
			PoolListRemove(&P->Partial, B);
			PoolListPush(&P->Full, B);
		}

		P->TotalAllocs++;
		TC->Slots[TC->Count++] = Raw;
	}

	POOL_MUTEX_UNLOCK(P->Lock);
	return TC->Count > 0;
}

static void ThreadCacheUnload(PPOOL P, PTHREADCACHE TC) {
	I32 DrainTo = POOL_THREAD_CACHE_SIZE / 2;

	POOL_MUTEX_LOCK(P->Lock);

	while (TC->Count > DrainTo) {
		void *Raw = TC->Slots[--TC->Count];
		PPOOLBLOCK B = PoolOwnerOf(PoolToUser(Raw));

		if (B->FreeCount == 0) {
			PoolListRemove(&P->Full, B);
			PoolListPush(&P->Partial, B);
		}

		*(PPVOID)PoolToUser(Raw) = B->Freelist;
		B->Freelist = Raw;
		B->FreeCount++;

		if (B->FreeCount == B->Capacity) {
			PoolListRemove(&P->Partial, B);
			PoolListPush(&P->Empty, B);
		}

		P->TotalFrees++;
	}

	POOL_MUTEX_UNLOCK(P->Lock);
}
#endif /* POOL_THREAD_CACHE_SIZE */

PPOOL LoadPool(ALLOCATOR allocator, U64 ObjSize, U64 BlockCap)
{
	if (ObjSize < sizeof(PVOID)) ObjSize = sizeof(PVOID);
	ObjSize = Aligned(ObjSize, POOL_ALIGN);

	if (BlockCap == 0)
	{
		U64 Stride = PoolSlotStride(ObjSize);
		BlockCap = POOL_DEFAULT_BYTES / Stride;
		if (BlockCap == 0) BlockCap = 1;
	}

	PPOOL P = Alloc(&allocator, sizeof(POOL), alignof(POOL));
	if (!P)
		return null;
	P->Allocator = allocator;
	P->ObjSize = ObjSize;
	P->BlockCap = BlockCap;

	POOL_MUTEX_INIT(P->Lock);

#ifdef POOL_THREAD_CACHE_SIZE
	if (POOL_TLS_CREATE(P->ThreadKey) != 0)
	{
		POOL_MUTEX_DESTROY(P->Lock);
		Free(&P->Allocator, P);
		return null;
	}
#endif

	return P;
}

void FreePool(PPOOL P)
{
	assert(P and "Pool needs to be valid");
#ifdef POOL_THREAD_CACHE_SIZE
	POOL_TLS_DELETE(P->ThreadKey);
#endif

	PPOOLBLOCK Lists[3] = { P->Partial, P->Full, P->Empty };
	for (I32 Li = 0; Li < 3; Li++)
	{
		PPOOLBLOCK B = Lists[Li];
		while (B)
		{
			PPOOLBLOCK Next = B->Next;
			Free(&P->Allocator, B);
			B = Next;
		}
	}

	POOL_MUTEX_DESTROY(P->Lock);
	Free(&P->Allocator, P);
}

PVOID PoolPut(PPOOL P)
{
	assert(P and "Pool needs to be valid");
#ifdef POOL_THREAD_CACHE_SIZE
	PTHREADCACHE TC = ThreadCacheGet(P);
	if (TC)
	{
		if (TC->Count == 0 && !ThreadCacheReload(P, TC))
			return null;
		if (TC->Count > 0)
			return PoolToUser(TC->Slots[--TC->Count]);
	}
#endif

	POOL_MUTEX_LOCK(P->Lock);

	PPOOLBLOCK B = P->Partial;
	if (!B)
	{
		B = P->Empty;
		if (B)
		{
			PoolListRemove(&P->Empty, B);
			PoolListPush(&P->Partial, B);
		}
		else
		{
			POOL_MUTEX_UNLOCK(P->Lock);
			B = PoolBlockCreate(P, P->ObjSize, P->BlockCap);
			if (!B)
				return null;
			POOL_MUTEX_LOCK(P->Lock);
			PoolListPush(&P->Partial, B);
			P->BlockCount++;
		}
	}

	PVOID Raw = B->Freelist;
	B->Freelist = *(PPVOID)PoolToUser(Raw);
	B->FreeCount--;

	if (B->FreeCount == 0)
	{
		PoolListRemove(&P->Partial, B);
		PoolListPush(&P->Full, B);
	}

	P->TotalAllocs++;
	POOL_MUTEX_UNLOCK(P->Lock);

	return PoolToUser(Raw);
}

PVOID PoolPutz(PPOOL P)
{
	PVOID Ptr = PoolPut(P);
	return Ptr ? memset(Ptr, 0, P->ObjSize) : Ptr;
}

void PoolDel(PPOOL P, PVOID Ptr)
{
	assert(P and Ptr and "Pool and ptr needs to be valid");
#ifdef POOL_THREAD_CACHE_SIZE
	PTHREADCACHE TC = ThreadCacheGet(P);
	if (TC) {
		if (TC->Count == POOL_THREAD_CACHE_SIZE)
			ThreadCacheUnload(P, TC);
		TC->Slots[TC->Count++] = PoolToRaw(Ptr);
		return;
	}
#endif

	PPOOLBLOCK B = PoolOwnerOf(Ptr);
	void *Raw = PoolToRaw(Ptr);

	POOL_MUTEX_LOCK(P->Lock);

	if (B->FreeCount == 0) {
		PoolListRemove(&P->Full, B);
		PoolListPush(&P->Partial, B);
	}

	*(PPVOID)PoolToUser(Raw) = B->Freelist;
	B->Freelist = Raw;
	B->FreeCount++;

	if (B->FreeCount == B->Capacity) {
		PoolListRemove(&P->Partial, B);
		PoolListPush(&P->Empty, B);
	}

	P->TotalFrees++;
	POOL_MUTEX_UNLOCK(P->Lock);
}

void TrimPool(PPOOL P) {
	assert(P and "Pool needs to be valid");

	POOL_MUTEX_LOCK(P->Lock);
	PPOOLBLOCK ToFree = P->Empty;
	P->Empty = null;
	POOL_MUTEX_UNLOCK(P->Lock);

	while (ToFree)
	{
		PPOOLBLOCK Next = ToFree->Next;

		POOL_MUTEX_LOCK(P->Lock);
		P->BlockCount--;
		POOL_MUTEX_UNLOCK(P->Lock);

		Free(&P->Allocator, ToFree);
		ToFree = Next;
	}
}

void ClearPool(PPOOL P)
{
	assert(P and "Pool needs to be valid");
#ifdef POOL_THREAD_CACHE_SIZE
	PoolFlushThreadCache(P);
#endif

	POOL_MUTEX_LOCK(P->Lock);

	PPOOLBLOCK All = null;
	PPOOLBLOCK Lists[3] = { P->Partial, P->Full, P->Empty };
	for (I32 Li = 0; Li < 3; Li++)
	{
		PPOOLBLOCK B = Lists[Li];
		while (B)
		{
			PPOOLBLOCK Next = B->Next;
			B->Next = All;
			B->Prev = null;
			if (All) All->Prev = B;
			All = B;
			B = Next;
		}
	}
	P->Partial = null;
	P->Full = null;
	P->Empty = null;

	U64 Stride = PoolSlotStride(P->ObjSize);
	for (PPOOLBLOCK B = All; B; B = B->Next) {
		PCHAR Base = (PCHAR)B + Aligned(sizeof(POOLBLOCK), POOL_ALIGN);
		for (U64 I = 0; I < B->Capacity; I++)
		{
			PVOID Raw = Base + Stride * I;
			PVOID RawNxt = (I + 1 < B->Capacity) ? Base + Stride * (I + 1) : null;
			*(PPPOOLBLOCK)Raw = B;
			*(PPVOID)PoolToUser(Raw) = RawNxt;
		}
		B->Freelist = Base;
		B->FreeCount = B->Capacity;
	}

	P->Empty = All;
	P->TotalAllocs = 0;
	P->TotalFrees = 0;

	POOL_MUTEX_UNLOCK(P->Lock);
}

void PoolFlushThreadCache(PPOOL P) {
#ifdef POOL_THREAD_CACHE_SIZE
	PTHREADCACHE TC = (PTHREADCACHE)POOL_TLS_GET(P->ThreadKey);
	if (!TC || TC->Count == 0) return;
	ThreadCacheDrain(P, TC);
#else
	(void)P;
#endif
}

void PoolStats(const PPOOL P, PPOOLSTATS Out)
{
	assert(P and Out and "Pool and stats need to be valid");

	PPOOL Mp = (PPOOL)(uintptr_t)P;
	POOL_MUTEX_LOCK(Mp->Lock);

	memzero(Out);
	Out->ObjSize = P->ObjSize;
	Out->BlockCap = P->BlockCap;
	Out->BlockCount = P->BlockCount;
	Out->TotalAllocs = P->TotalAllocs;
	Out->TotalFrees = P->TotalFrees;

	PPOOLBLOCK const Lists[3] = { P->Partial, P->Full, P->Empty };
	for (I32 Li = 0; Li < 3; Li++)
	{
		for (PPOOLBLOCK B = Lists[Li]; B; B = B->Next)
		{
			Out->TotalSlots += B->Capacity;
			Out->FreeSlots += B->FreeCount;
		}
	}
	Out->UsedSlots = Out->TotalSlots - Out->FreeSlots;
	Out->MemoryBytes = P->BlockCount * (Aligned(sizeof(POOLBLOCK), POOL_ALIGN) + PoolSlotStride(P->ObjSize) * P->BlockCap);
	POOL_MUTEX_UNLOCK(Mp->Lock);
}
BOOL PooSameAlignedObjSize(const PPOOL P, U64 ObjSize)
{
	return P->ObjSize == Aligned(ObjSize, POOL_ALIGN);
}

void PoolBegin(PPOOL P, PPOOLITER It)
{
	assert(P and It and "Pool and iterator need to be valid");
	It->Pool = P;
	It->List = 0;
	It->Slot = 0;
	POOL_MUTEX_LOCK(P->Lock);
	It->Block = P->Partial ? P->Partial : P->Full;
	if (!It->Block)
		It->List = 2;
}

PVOID PoolNext(POOLITER *It)
{
	PPOOL P = It->Pool;
	assert(P and "Pool in iterator needs to be valid.");
	
	while (It->Block)
	{
		PPOOLBLOCK B = (PPOOLBLOCK)It->Block;

		while (It->Slot < B->Capacity)
		{
			U64 I = It->Slot++;
			if (PoolSlotIsFree(B, P->ObjSize, I))
				continue;

			U64  Stride = PoolSlotStride(P->ObjSize);
			PCHAR Base = (PCHAR)B + Aligned(sizeof(POOLBLOCK), POOL_ALIGN);
			return PoolToUser(Base + Stride * I);
		}

		It->Slot = 0;
		if (B->Next)
		{
			It->Block = B->Next;
		}
		else if (It->List == 0)
		{
			It->List = 1;
			It->Block = P->Full;
		}
		else
		{
			It->Block = null;
		}
	}
	return null;
}
void PoolEnd(POOLITER *It)
{
	POOL_MUTEX_UNLOCK(It->Pool->Lock);
	*It = LIT(POOLITER, 0);
}

void PoolBeginRev(PPOOL P, PPOOLITER It)
{
	assert(P and It and "Pool and iterator need to be valid");
	It->Pool = P;
	It->List = 0;
	POOL_MUTEX_LOCK(P->Lock);

	PPOOLBLOCK tail = null;
	I32    list = 0;
	for (PPOOLBLOCK B = P->Full; B; B = B->Next)
		tail = B;
	if (not tail)
	{
		list = 1;
		for (PPOOLBLOCK B = P->Partial; B; B = B->Next)
			tail = B;
	}

	It->List = list;
	It->Block = tail;
	It->Slot = tail ? (ptrdiff_t)tail->Capacity - 1 : -1;
}

PVOID PoolNextRev(PPOOLITER It)
{
	PPOOL pool = It->Pool;
	while (It->Block)
	{
		PPOOLBLOCK block = (PPOOLBLOCK)It->Block;
		while (It->Slot >= 0)
		{
			U64 slot = (U64)It->Slot--;
			if (PoolSlotIsFree(block, pool->ObjSize, slot)) continue;
			return PoolToUser((PCHAR)block + Aligned(sizeof(POOLBLOCK), POOL_ALIGN) + PoolSlotStride(pool->ObjSize) * slot);
		}

		if (block->Prev)
		{
			It->Block = block->Prev;
		}
		else if (It->List == 0)
		{
			It->List = 1;
			PPOOLBLOCK Tail = null;
			for (PPOOLBLOCK C = pool->Partial; C; C = C->Next) Tail = C;
			It->Block = Tail;
		}
		else
		{
			It->Block = null;
		}

		if (It->Block)
			It->Slot = (ptrdiff_t)((PPOOLBLOCK)It->Block)->Capacity - 1;
	}
	return null;
}

void PoolEndRev(PPOOLITER It)
{
	POOL_MUTEX_UNLOCK(It->Pool->Lock);
	It->Pool = null;
}
