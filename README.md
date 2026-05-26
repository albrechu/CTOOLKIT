# CTOOLKIT
This is a collection of C utilities as a shortcut for some of my projects. They are made to have a very direct interface and implementation, 
using as less code as possible. Use at your own risk.

target_link_libraries(your_project PRIVATE TOOLKIT::TOOLKIT) // Link all

## GENERAL
### Operations
```c
alignof(x);                           // alignment of type x
alignas(16);                          // align type to 16 bytes
myfun(ALIGNSTRING("Hello", 16));      // align string to 16 bytes
_ = unused_variable;                  // mark variable as unused
typeof(x);                            // type of x
memzero(x);                           // zero out memory of x
alignup(x, 16);                       // align x up to the next multiple of 16
aligndown(x, 16);                     // align x down to the previous multiple of 16
makeopaque(TYPE);                     // make a opaque TYPE (TYPE, PTYPE, PPTYPE)
arrsize(arr);                         // size of array arr
stringify(x);                         // stringify x
breakpoint();                         // trigger a breakpoint
unreachable();                        // mark code path as unreachable
LIT(VEC3, .x = 50, .y = 10, .z = 30); // create type literal with field values
```
### Bundle
```c
U64 size = 0;                     
I32 *a = packadd(size, I32, 10);
F32 *b = packadd(size, F32, 5);  
PVOID bundle = malloc(size);      
// Adjust pointers in bundle to point to the correct locations
packfix(bundle, a); 
packfix(bundle, b); 
// Use a and b...                 
free(bundle); 
```
### DEFER
No Error
```c
dfs(16); // Define a defer stack with 16 items capacity
PVOID ptr = malloc(100);
if (ptr) {
	dfflush(); // Execute deferred functions immediately
	return -1;
}
df(free, ptr);
// Do stuff...
dfflush();
return 0; 
```
Error
```c
errdfs(16); // Define a defer stack with 16 items capacity
PVOID ptr = malloc(100);
if (ptr) {
	errdfflush(); // Execute deferred functions immediately
	return null;
}
errdf(free, ptr);
// Do stuff
return ptr; // No error, no need to flush 
```
## ALLOCATORS
```c
#include <TOOLKIT/ALLOCATOR.H>
dfs(16);

/// Default Allocator (malloc/free)
ALLOCATOR allocator = GetDefaultAllocator();
PVOID mem = Alloc(&allocator, 256 * sizeof(U32), alignof(U32)); // allocate 256 bytes
Free(&allocator, mem); 

/// ARENA
PARENA arena = LoadArena(allocator, 1024); // default_blocksize = 1024
df(FreeArena, arena); // Defer freeing the arena

PVOID a = ArenaPut(arena, 256); // allocate 256 bytes from arena
PVOID b = ArenaPutz(arena, 256); // allocate zeroed 256 bytes from arena
ArenaClear(arena);
a = ArenaPut(arena, 256);
SNAPSHOT snap = ArenaSnap(arena); // take a snapshot of arena state
FSTR str = ArenaStrdup(arena, fstrfromstr("Hello, World!"));
ArenaRewind(arena, snap); // rewind arena to snapshot, freeing str

allocator = ArenaAllocator(arena); 

/// POOL
PPOOL pool = LoadPool(allocator, sizeof(F64), 16); 
df(FreePool, pool); // Defer freeing the pool

*(F64 *)PoolPut(pool) = 3.1415; // allocate a double from the pool
*(F64 *)PoolPutz(pool) = 2.7182;
F64 *x = (F64 *)PoolPutz(pool);
*x = 42.0;
PoolDel(pool, x); // Return x to the pool

F64 *val;
POOLITER it;
PoolBeginRev(pool, &it); // Lock pool and reverse iterate.
while ((val = (F64 *)PoolNextRev(&it)))
{
}
PoolEndRev(&it); // Unlock pool

it = { 0 };
PoolBegin(pool, &it); // Lock pool and iterate.
while ((val = (F64 *)PoolNext(&it)))
{
}
PoolEnd(&it); // Unlock pool

// Cleanup
dfflush(); // Free evrything
```
## CLI
```c
/// Parsing
// Set default values for arguments
I32  a = 0;
F32  b = 0;
BOOL c = false;
CSTR d = "Default";
PATH e = "V:/Default/Path/To/Your/Folder/Or/File";
// Simulate command line arguments
CHAR arg0[] = "cli_test.exe";
CHAR arg1[] = "-a";
CHAR arg2[] = "-5325";
CHAR arg3[] = "-b=3.1415"; 
CHAR arg4[] = "-c"; // flag
CHAR arg5[] = "-d=My String"; 
CHAR arg6[] = "-e";
CHAR arg7[] = "Relative/Path/To/A/File";
PCHAR argv[] = { arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7 };
int argc = arrsize(argv);
// Define options
ARGOPTION options[] =
{
	{ "a", 1, ARGTYPE_I32, &a },
	{ "b", 1, ARGTYPE_F32, &b },
	{ "c", 0, ARGTYPE_BOOL, &c },
	{ "d", 1, ARGTYPE_CSTR, &d },
	{ "e", 1, ARGTYPE_PATH, e }, 
};
if (!LoadArgs(argc, argv, arrsize(options), options))
	return -1

/// Progressbar
for (U64 i = 0; i < (U64)1e9; i++)
{
	if (i %  9999999ULL == 0)
		Progress(LOGGING_SPECIAL, rounddiv(i,  9999999ULL), "Processing...");
}

/// PROCESS WITH PIPES
ALLOCATOR allocator = DefaultAllocator();
FSTR args[] = 
{ 
	fstrstr("powershell.exe"),
	fstrstr("-NoProfile"),
	fstrstr("-ExecutionPolicy"),
	fstrstr("Bypass"),
	fstrstr("-Command"),
	fstrstr("Get-Date")
};
PPROCESS process = LoadProcess(&allocator, PROCESSMODE_READ, arrsize(args), args);
if (process)
{
	CHAR buf[4096];
	U64 read;
	while (ProcessRead(process, buf, sizeof(buf), &read) == OK) // Read output
		info("%.*s", (int)read, buf);
	
	I32 exit_code;
	ProcessWait(process, &exit_code); // FreeProcess does this automatically if you do not need the exit code
	if (exit_code == 0)
		succln("Process exited with code: %d\n", exit_code);
	else
		errln("Process exited with code: %d\n", exit_code);
	FreeProcess(process);
}
```

## THREAD
```c
#include <TOOLKIT/THREAD.H>
#include <TOOLKIT/CLI.H>

dfs(16);
ALLOCATOR allocator = DefaultAllocator();

PJOBPOOL pool = LoadJobPool(allocator, 2, 256);
df(FreeJobPool, pool);

PJOBGRAPH graph = LoadJobGraph(allocator, pool);
df(FreeJobGraph, graph);

JobGraphPut(graph);
{
	JobGraphPut(graph);
	{
		JobGraphDispatch(graph, [](PVOID data, U64 start, U64 end) {
			succln("Hello from job 1! UserData: %s", (PCHAR)data);
		}, (PVOID)"Job 1 Data");
		JobGraphDispatch(graph, [](PVOID data, U64 start, U64 end) {
			succln("Hello from job 2! UserData: %s", (PCHAR)data);
		}, (PVOID)"Job 2 Data");
	}
	JobGraphPop(graph);

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
```

## FILE
```c
#include <TOOLKIT/FILE.H>
#include <TOOLKIT/CLI.H> // For printing
/// General
PathSetWorkingDirToExecutable();

/// File Reading
FILEDATA f;
if (BeginFile(DefaultAllocator(), fstrfromstr("../CMakeCache.txt"), FILEMODE_READ, 0, &f) == OK)
{
	FSTR content;
	RESULT rc = FileReadAll(&f, true, &content);
	if (rc != OK)
	{
		errln("Failed to read file.");
		return -1;
	}
	noteln("File content:\n%.*s", (int)content.size, content.str);
	EndFile(&f);
}

/// INI Writing
if (BeginFile(DefaultAllocator(), fstrfromstr("config.ini"), FILEMODE_WRITETEXT, 0, &f) == OK)
{
	// [Settings]
	IniGroup(&f, fstrfromstr("Settings"));
	IniFloat(&f, fstrfromstr("Volume"), 0.75f);
	IniInteger(&f, fstrfromstr("ResolutionWidth"), 1920);
	IniBool(&f, fstrfromstr("Fullscreen"), true);
	// [User]
	IniGroup(&f, fstrfromstr("User"));
	IniString(&f, fstrfromstr("Username"), fstrfromstr("JohnDoe"));
	IniInteger(&f, fstrfromstr("Age"), 42);

	EndFile(&f);
}

/// INI Reading
if (BeginFile(DefaultAllocator(), fstrfromstr("config.ini"), FILEMODE_READTEXT, 0, &f) == OK)
{
	INIENTRY entry;
	while (IniReadNextEntry(&f, &entry) == OK)
	{
		switch (entry.ValueType)
		{
		case VALUETYPE_BOOL:
			noteln("[%.*s] %.*s = %s", entry.Group.Length, entry.Group.Name, entry.Key.Length, entry.Key.Name, entry.Value.Boolean ? "true" : "false");
			break;
		case VALUETYPE_FLOAT:
			noteln("[%.*s] %.*s = %f", entry.Group.Length, entry.Group.Name, entry.Key.Length, entry.Key.Name, entry.Value.Float);
			break;
		case VALUETYPE_INT:
			noteln("[%.*s] %.*s = %d", entry.Group.Length, entry.Group.Name, entry.Key.Length, entry.Key.Name, entry.Value.Int);
			break;
		case VALUETYPE_STRING:
			noteln("[%.*s] %.*s = %.*s", entry.Group.Length, entry.Group.Name, entry.Key.Length, entry.Key.Name, entry.Value.String.Length, entry.Value.String.Name);
			break;
		default:
			errln("Unknown value type for key [%.*s] %.*s", entry.Group.Length, entry.Group.Name, entry.Key.Length, entry.Key.Name);
			break;
		}
	}

	EndFile(&f);
}

/// JSON Writing
if (BeginFile(DefaultAllocator(), fstrfromstr("test.json"), FILEMODE_WRITETEXT, 0, &f) == OK)
{
	JsonBegin(&f);
	{
		JsonObjectBegin(&f, fstrfromstr("player"));
		{
			JsonString(&f, fstrfromstr("name"), fstrfromstr("Alice"));
			JsonInteger(&f, fstrfromstr("score"), 12345);
			JsonFloat(&f, fstrfromstr("health"), 0.85f);
			JsonArrayBegin(&f, fstrfromstr("position"));
			for (I32 i = 0; i < 3; i++)
			{
				JsonFloat(&f, FSTR_INVALID, (F32)(i * 1.5f));
			}
			JsonArrayEnd(&f);

			JsonArrayBegin(&f, fstrfromstr("inventory"));
			{
				JsonString(&f, FSTR_INVALID, fstrfromstr("Sword"));
				JsonString(&f, FSTR_INVALID, fstrfromstr("Hammer"));
			}
			JsonArrayEnd(&f);

			JsonObjectBegin(&f, fstrfromstr("skills"));
			JsonObjectEnd(&f);
			JsonFloat(&f, fstrfromstr("movement speed"), 4.512331);
		}
		JsonObjectEnd(&f);
	}
	JsonEnd(&f);
	EndFile(&f);
}

/// JSON Reading
if (BeginFile(DefaultAllocator(), fstrfromstr("test.json"), FILEMODE_READTEXT, 0, &f) == OK)
{
	JSONENTRY entry;
	while (JsonReadNextEntry(&f, &entry) == OK)
	{
		switch (entry.ValueType)
		{
		case VALUETYPE_BOOL:
			noteln("%.*s = %s", entry.Key.Length, entry.Key.Name, entry.Value.Boolean ? "true" : "false");
			break;
		case VALUETYPE_FLOAT:
			noteln("%.*s = %f", entry.Key.Length, entry.Key.Name, entry.Value.Float);
			break;
		case VALUETYPE_INT:
			noteln("%.*s = %d", entry.Key.Length, entry.Key.Name, entry.Value.Int);
			break;
		case VALUETYPE_STRING:
			noteln("%.*s = %.*s", entry.Key.Length, entry.Key.Name, entry.Value.String.Length, entry.Value.String.Name);
			break;
		case VALUETYPE_ARRAY:
			noteln("%.*s = [Array]", entry.Key.Length, entry.Key.Name);
			break;
		case VALUETYPE_OBJECT:
			if (entry.Key.Length)
				noteln("%.*s = {Object}", entry.Key.Length, entry.Key.Name);
			break;
		case VALUETYPE_NULL:
			noteln("%.*s = null", entry.Key.Length, entry.Key.Name);
			break;
		default:
			errln("Unknown value type for key %.*s", entry.Key.Length, entry.Key.Name);
			break;
		}
	}
	EndFile(&f);
}

// MessagePack

```
