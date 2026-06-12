#include <TOOLKIT/ALLOCATOR.H>
#include <TOOLKIT/STRING.H>
#include <TOOLKIT/CLI.H>

int main()
{
   dfs(16);

   PARENA arena = LoadArena(null, 1024);
   df(FreeArena, arena);
   ArenaClear(arena);

   for (U64 i = 0; i < 64; i++)
	  _ = ArenaPut(arena, 64 * sizeof(F64), alignof(F64));

   U64 size = 0;                    
   I32 *a = packadd(size, I32, 10); 
   I8  *b = packadd(size, I8,  1); // 7 Bytes of padding is added here.
   F64 *c = packadd(size, F64, 5);
   PVOID bundle = Alloc(ArenaAllocator(arena), size, alignof(I32));
   packfix(bundle, a);              
   packfix(bundle, b);
   packfix(bundle, c);

   PPOOL pool = LoadPool(ArenaAllocator(arena), sizeof(I32), 256);
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

   SV haystack = svlit("PlaneBUSAirportBusCarTrain");
   
   SV needle, str;
   needle = svlit("Bus");
   str = SVFind(haystack, needle);
   if (svvalid(str))
	  succln("Found %.*s at %d", (int)str.Size, str.String, (int)(str.String - haystack.String));
   else
	  errln("Did not find %.*s", (int)needle.Size, needle.String);

   needle = svlit("BuS");
   str = SVFind(haystack, needle);
   if (svvalid(str))
	  succln("Found %.*s at %d", (int)str.Size, str.String, (int)(str.String - haystack.String));
   else
	  errln("Did not find %.*s", (int)needle.Size, needle.String);

   needle = svlit("BuS");
   str = SVFindNoCase(haystack, needle);
   if (svvalid(str))
	  succln("Found %.*s at %d", (int)str.Size, str.String, (int)(str.String - haystack.String));
   else
	  errln("Did not find %.*s", (int)needle.Size, needle.String);

   dfflush();
   return 0;
}
