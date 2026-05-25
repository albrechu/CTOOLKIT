#include <TOOLKIT/THREAD.H>
#include <assert.h>
#include <TOOLKIT/CLI.H>

int main()
{
	dfs(16);

	ALLOCATOR allocator = DefaultAllocator();

	PJOBPOOL pool = LoadJobPool(allocator, 2, 256);
	df(FreeJobPool, pool);

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