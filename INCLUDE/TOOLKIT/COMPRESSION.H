#pragma once

#include <TOOLKIT/TYPES.H>

#ifdef __cplusplus
extern "C" {
#endif

U64 Hash64(DV data, U64 seed);
U32 Crc32(U32 crc, const PVOID buf, U64 len);

DV RAnsEncode(PALLOCATOR allocator, DV uncompressed, U8 scaling_bits);
DV RAnsDecode(PALLOCATOR allocator, DV compressed);
void  FreeRAns(PALLOCATOR allocator, DV data);

#ifdef __cplusplus
}
#endif