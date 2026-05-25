#pragma once

#include <TOOLKIT/TYPES.H>

#ifdef __cplusplus
extern "C" {
#endif

U64 Hash64(FDATA data, U64 seed);
U32 Crc32(U32 crc, const PVOID buf, U64 len);

FDATA RAnsEncode(ALLOCATOR allocator, FDATA uncompressed, U8 scaling_bits);
FDATA RAnsDecode(ALLOCATOR allocator, FDATA compressed);
void  FreeRAns(ALLOCATOR allocator, FDATA data);

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

#ifdef __cplusplus
}
#endif