#pragma once

#include <TOOLKIT/DEFINES.H>

#include <stdint.h>
#include <iso646.h>

#if (defined(__STDC__) && __STDC_VERSION__ >= 199901L) || (defined(_MSC_VER) && _MSC_VER >= 1800)
#include <stdbool.h>
#elif !defined(__cplusplus) && !defined(bool)
typedef enum bool{ false = 0, true = !false } bool;
#endif

#define null NULL

#if defined(__cplusplus)
#define LIT(type, ...) type{ __VA_ARGS__ }
#else
#define LIT(type, ...) (type){ __VA_ARGS__ }
#endif

typedef int8_t      I8;
typedef int16_t     I16;
typedef int32_t     I32;
typedef int64_t     I64;
typedef uint8_t     U8;
typedef uint16_t    U16;
typedef uint32_t    U32, *PU32, **PPU32;
typedef uint64_t    U64, *PU64;
typedef float       F32;
typedef double      F64;
typedef void	    VOID, *PVOID, **PPVOID;
typedef int         BOOL;
typedef const char *CSTR;
typedef char        CHAR, *PCHAR, **PPCHAR;
typedef CHAR        PATH[512];
typedef const PATH  CPATH;
typedef CPATH       IPATH;
typedef PATH        OPATH, IOPATH;
typedef struct FILE *PFILE;

typedef struct FSTR
{
	PCHAR str;
	U64   size;
} FSTR, *PFSTR;

typedef struct NAME
{
	CHAR Name[64];
	U32  Length;
	U64  Hash; // FNV-1a hash of the name, for quick comparisons
} NAME;

typedef struct FDATA
{
	PVOID data;
	U64   size;
} FDATA;

typedef enum RESULT
{
	UNDEFINED_ERROR      = -11,
	INVALID_PARAMETER    = -10,
	INVALID_FORMAT       = -9,
	INVALID_OPERATION    = -8,
	NOT_READY            = -7,
	OUT_OF_DEVICE_MEMORY = -6,
	OUT_OF_MEMORY        = -5,
	TIMEOUT              = -4,
	ACCESS_DENIED        = -3,
	DOES_NOT_EXIST       = -2,
	BUSY                 = -1,
	OK                   = 0,
	END_OF_FILE          = 1,
} RESULT;

makeopaque(POOL);
makeopaque(ARENA);

typedef PVOID(*ALLOC)(PVOID user_data, U64 size, U64 alignment);
typedef PVOID(*REALLOC)(PVOID user_data, PVOID ptr, U64 size, U64 alignment);
typedef VOID(*FREE)(PVOID user_data, PVOID ptr);
typedef struct ALLOCATOR
{
	PVOID   user_data;
	ALLOC   alloc;
	REALLOC realloc;
	FREE    free;
} ALLOCATOR, *PALLOCATOR, **PPALLOCATOR;


static const FSTR  FSTR_INVALID;
#define fstr(string, length) LIT(FSTR, .str = (PCHAR)string, .size = length)
#define fstrinvalid(s) ((s).str == null)
#define fstrfromstr(str) fstr(str, sizeof(str) - 1)

static const FDATA FDATA_INVALID;
#define fdata(d, s) LIT(FDATA, .data = d, .size = s)
#define fdatainvalid(s) ((s).data == null)
#define fdatafromstr(str) fdata(str, sizeof(str) - 1)