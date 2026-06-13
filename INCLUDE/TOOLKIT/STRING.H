#pragma once

#include <TOOLKIT/TYPES.H>

#ifdef __cplusplus
extern "C" {
#endif

VOID SVFree(PALLOCATOR allocator, SV string);
BOOL SVEqual(SV s1, SV s2);
BOOL SVEqualNoCase(SV s1, SV s2);
BOOL SVStartsWith(SV haystack, SV needle);
SV   SVSlice(SV s, U64 start, U64 length);
SV   SVSliceSub(SV s, U64 start, U64 end);
I32  SVCompare(SV s1, SV s2);
I64  SVChr(SV s, CHAR c);
SV   SVFind(SV haystack, SV needle);
SV   SVFindNoCase(SV haystack, SV needle);
SV   SVStrdup(PALLOCATOR allocator, SV string);
SV   SVStrdupTerminated(PALLOCATOR allocator, SV string);
SV   SVJoinList(PALLOCATOR allocator, CHAR separator, U32 strings_count, PSV strings);
SV   SVJoin(PALLOCATOR allocator, U32 strings_count, PSV strings);

#define HASH64_BASIS 14695981039346656037ULL
#define HASH64_PRIME 1099511628211ULL
static inline U64 HashFNV1a(DV data, U64 seed)
{
   // Setting hash to the seed gives you maximum flexibility
   U64 hash = seed;
   const U8 *p = (const U8 *)data.Data;
   const U8 *end = p + data.Size;

   for (; p + 4 <= end; p += 4)
   {
      hash = (hash ^ p[0]) * HASH64_PRIME;
      hash = (hash ^ p[1]) * HASH64_PRIME;
      hash = (hash ^ p[2]) * HASH64_PRIME;
      hash = (hash ^ p[3]) * HASH64_PRIME;
   }
   while (p < end)
      hash = (hash ^ *p++) * HASH64_PRIME;

   return hash;
}
static inline U64 SVHash(SV str)
{
    return HashFNV1a(dv(str.String, str.Size), HASH64_BASIS);
}
#define svhash(x) SVHash(x)


#ifdef __cplusplus
}
#endif