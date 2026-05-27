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

#ifdef __cplusplus
}
#endif