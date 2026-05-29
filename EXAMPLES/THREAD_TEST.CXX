#include <TOOLKIT/THREAD.H>
#include <TOOLKIT/CLI.H>

int main()
{
	dfs(16);

	ALLOCATOR allocator = DefaultAllocator();

	PMUTEX mtx = LoadMutex(&allocator);
	df(FreeMutex, mtx);
	PCONDITIONVARIABLE cv = LoadConditionVariable(&allocator);
	df(FreeConditionVariable, cv);
	volatile U32 a = 0;
	StoreU32(&a, 42);
	volatile PVOID b = cv;
	StorePtr(&b, null);

	PJOBPOOL pool = LoadJobPool(allocator, 2, 256);
	df(FreeJobPool, pool);
	JobPoolDispatchN(pool, null, 64, 8, [](PVOID data, U64 start, U64 end) { 
		while (start != end)
		{
			specln("Work %llu", start++);
		}
	}, null);

	PJOBGRAPH graph = LoadJobGraph(allocator, pool);
	df(FreeJobGraph, graph);

	JobGraphPut(graph);
	{
		JobGraphDispatch(graph, [](PVOID data, U64 start, U64 end) {
			succln("Hello from job 1! UserData: %s", (PCHAR)data);
		}, (PVOID)"Job 1 Data");
		JobGraphDispatch(graph, [](PVOID data, U64 start, U64 end) {
			succln("Hello from job 2! UserData: %s", (PCHAR)data);
		}, (PVOID)"Job 2 Data");
		JobGraphDispatchN(graph, 10, 2, [](PVOID data, U64 start, U64 end) {
			succln("Hello from job group! UserData: %s, Start: %llu, End: %llu", (PCHAR)data, start, end);
		}, (PVOID)"Job Group Data");
	}
	JobGraphPop(graph);
		
	JobGraphRun(graph);
	JobPoolWait(pool);

	JobGraphRun(graph);
	JobPoolWait(pool);

	dfflush();
	return 0;
}