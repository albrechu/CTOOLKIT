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

#ifdef __cplusplus
}
#endif