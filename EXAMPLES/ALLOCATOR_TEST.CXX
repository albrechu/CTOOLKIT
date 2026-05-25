#include <TOOLKIT/ALLOCATOR.H>
#include <assert.h>

int main()
{
	dfs(16);

	ALLOCATOR allocator = DefaultAllocator();

	PARENA arena = LoadArena(allocator, 1024);
	df(FreeArena, arena);
	ArenaClear(arena);
	

	allocator = ArenaAllocator(arena);
	for (U64 i = 0; i < 64; i++)
	{
		_ = Alloc(&allocator, 64 * sizeof(double), alignof(double));
	}

	PPOOL pool = LoadPool(allocator, sizeof(I32), 256);
	df(FreePool, pool);
	for (U64 i = 0; i < 32324; i++)
	{
		*(I32 *)PoolPut(pool) = i;
	}
	
	I32 *val;
	POOLITER it;
	PoolBeginRev(pool, &it);
	while ((val = (I32 *)PoolNextRev(&it)))
	{
	}
	PoolEndRev(&it);

	it = { 0 };
	PoolBegin(pool, &it);
	while ((val = (I32 *)PoolNext(&it)))
	{
	}
	PoolEnd(&it);

	dfflush();
	return 0;
}
