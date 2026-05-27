#pragma once

#include <TOOLKIT/TYPES.H>

#ifdef __cplusplus
extern "C" {
#endif

VOID FStrFree(PALLOCATOR allocator, FSTR s);
BOOL FStrEqual(FSTR s1, FSTR s2);
I32  FStrCompare(FSTR s1, FSTR s2);
I64  FStrChr(FSTR s, CHAR c);
FSTR FStrFind(FSTR haystack, FSTR needle);
FSTR FStrFindNoCase(FSTR haystack, FSTR needle);
FSTR FstrStrdup(PALLOCATOR allocator, FSTR string);
FSTR FStrJoinList(PALLOCATOR allocator, CHAR separator, U32 strings_count, PFSTR strings);
FSTR FStrJoin(PALLOCATOR allocator, U32 strings_count, PFSTR strings);

#define HASH64_BASIS 14695981039346656037ULL
#define HASH64_PRIME 1099511628211ULL
static inline U64 HashFNV1a(FDATA data)
{
    U64 hash = HASH64_BASIS;
    const U8 *p = (const U8 *)data.data;
    const U8 *end = p + data.size;
    for (; p + 4 <= end; p += 4)
    {
        hash = (hash ^ p[0]) * HASH64_PRIME;
        hash = (hash ^ p[1]) * HASH64_PRIME;
        hash = (hash ^ p[2]) * HASH64_PRIME;
        hash = (hash ^ p[3]) * HASH64_PRIME;
    }
    while (p < end) // Remaining bytes
        hash = (hash ^ *p++) * HASH64_PRIME;
    return hash;
}
#define StrHash(str) HashFNV1a(fdatafromstr(str))

static inline U64 FstrHash(FSTR str)
{
    return HashFNV1a(LIT(FDATA, .data = str.str, .size = str.size));
}


#ifdef __cplusplus
}
#endif