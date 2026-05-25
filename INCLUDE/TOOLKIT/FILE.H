#pragma once

#include <TOOLKIT/TYPES.H>

#ifdef __cplusplus
extern "C" {
#endif

makeopaque(FILEDATA);

typedef enum FILEMODE_
{
	FILEMODE_NONE,
	FILEMODE_READ,
	FILEMODE_READTEXT, // Normalizes OS-specific line breakes.
	FILEMODE_WRITE,
	FILEMODE_WRITETEXT,

	FILEMODE_COUNT,
} FILEMODE;

typedef struct FILEDATA
{
	U64       FileSize;
	U64       Offset;
	FILEMODE  Mode;
	IOPATH    Path;
	FSTR      PathView;
	FSTR      Buffer;
	U64       BufferCapacity;
	PFILE     File;
	ALLOCATOR Allocator;
	union 
	{
		struct
		{
			U32  Depth;
			BOOL HasExplicitKey;
			U64  LineOffset;
			FSTR CurrentLine;
		} Json;
		struct
		{
			BOOL HasWrittenFirstValue;
			U64  LineOffset;
			FSTR CurrentLine;
		} Csv;
	};
} FILEDATA;

typedef enum VALUETYPE_
{
	VALUETYPE_BOOL,
	VALUETYPE_INT,
	VALUETYPE_FLOAT,
	VALUETYPE_STRING,
	VALUETYPE_OBJECT,
	VALUETYPE_ARRAY,
	VALUETYPE_NULL,
	VALUETYPE_ENDOFLINE,

	VALUETYPE_COUNT,
} VALUETYPE;

typedef struct INIENTRY
{
	NAME Group;
	NAME Key;
	VALUETYPE ValueType;
	union
	{
		BOOL Boolean;
		F32  Float;
		I32  Int;
		NAME String;
	} Value;
} INIENTRY, *PINIENTRY;

typedef struct JSONENTRY
{
	NAME      Key;
	VALUETYPE ValueType;
	U32       Depth;
	union
	{
		BOOL Boolean;
		F32  Float;
		I32  Int;
		NAME String;
	} Value;
} JSONENTRY, *PJSONENTRY;

typedef struct CSVENTRY
{
	VALUETYPE ValueType;
	union
	{
		BOOL Boolean;
		F32  Float;
		I32  Int;
		NAME String;
	} Value;
} CSVENTRY, *PCSVENTRY;

typedef struct MSGPACKENTRY
{
	VALUETYPE ValueType;
	union
	{
		BOOL Boolean;
		F32  Float;
		I32  Int;
		NAME String;
		U32  Count; // For arrays and maps
	} Value;
} MSGPACKENTRY, *PMSGPACKENTRY;

////////////////////////////////////////
// FILE INTERFACE                     //
////////////////////////////////////////
// / / / / / / / / / / / / / / / / / / / 
// GENERAL FILE I/O                    /
// / / / / / / / / / / / / / / / / / / /
RESULT BeginFile(ALLOCATOR allocator, FSTR path, FILEMODE mode, U64 initial_capacity, PFILEDATA file);
RESULT FileOpen(FSTR path, FILEMODE mode, PFILEDATA file);
RESULT FileReopen(PFILEDATA file, FILEMODE mode);
RESULT FileWriteAll(PFILEDATA file, BOOL as_text, FSTR content);
RESULT FileReadAll(PFILEDATA file, BOOL as_text, PFSTR content);
RESULT FileEnsureCapacity(PFILEDATA file, U64 capacity);
RESULT FileReadLine(PFILEDATA file, CHAR terminator, CHAR terminator_replacement, BOOL as_text, PFSTR content);
RESULT FileWrite(PFILEDATA file, FSTR content, BOOL as_text);
VOID   FileRewind(PFILEDATA file);
RESULT FileClose(PFILEDATA file);
VOID   EndFile(PFILEDATA file);
// / / / / / / / / / / / / / / / / / / / 
// INI FILE                            /
// / / / / / / / / / / / / / / / / / / /
//// WRITING ///////////////////////////
VOID   IniGroup(PFILEDATA filedata, FSTR group);
VOID   IniInteger(PFILEDATA filedata, FSTR key, I32 value);
VOID   IniFloat(PFILEDATA filedata, FSTR key, F32 value);
VOID   IniBool(PFILEDATA filedata, FSTR key, bool value);
VOID   IniString(PFILEDATA filedata, FSTR key, FSTR value);
//// READING ///////////////////////////
RESULT IniReadNextEntry(PFILEDATA filedata, PINIENTRY entry);
// / / / / / / / / / / / / / / / / / / / 
// JSON FILE                           /
// / / / / / / / / / / / / / / / / / / /
//// WRITING ///////////////////////////
VOID JsonBegin(PFILEDATA filedata);
VOID JsonEnd(PFILEDATA filedata);
VOID JsonObjectBegin(PFILEDATA filedata, FSTR key);
VOID JsonObjectEnd(PFILEDATA filedata);
VOID JsonArrayBegin(PFILEDATA filedata, FSTR key);
VOID JsonArrayEnd(PFILEDATA filedata);
VOID JsonInteger(PFILEDATA filedata, FSTR key, I32 value);
VOID JsonFloat(PFILEDATA filedata, FSTR key, F32 value);
VOID JsonBool(PFILEDATA filedata, FSTR key, BOOL value);
VOID JsonString(PFILEDATA filedata, FSTR key, FSTR value);
VOID JsonNull(PFILEDATA filedata, FSTR key);
//// READING ///////////////////////////
VOID JsonReadReset(PFILEDATA filedata);
RESULT JsonReadNextEntry(PFILEDATA filedata, PJSONENTRY entry);
// / / / / / / / / / / / / / / / / / / / 
// CSV FILE                            /
// / / / / / / / / / / / / / / / / / / /
//// WRITING ///////////////////////////
VOID CsvBegin(PFILEDATA filedata);
VOID CsvEnd(PFILEDATA filedata);
VOID CsvRow(PFILEDATA filedata);
VOID CsvInteger(PFILEDATA filedata, I32 value);
VOID CsvFloat(PFILEDATA filedata, F32 value);
VOID CsvBool(PFILEDATA filedata, BOOL value);
VOID CsvString(PFILEDATA filedata, FSTR value);
//// READING ///////////////////////////
RESULT CsvReadNextValue(PFILEDATA filedata, PCSVENTRY entry);
// / / / / / / / / / / / / / / / / / / / 
// MESSAGEPACK                         /
// / / / / / / / / / / / / / / / / / / /
//// WRITING ///////////////////////////
VOID MsgPackInteger(PFILEDATA filedata, I32 value);
VOID MsgPackFloat(PFILEDATA filedata, F32 value);
VOID MsgPackBool(PFILEDATA filedata, BOOL value);
VOID MsgPackString(PFILEDATA filedata, FSTR value);
VOID MsgPackNull(PFILEDATA filedata);
VOID MsgPackArrayBegin(PFILEDATA filedata, U32 count);
VOID MsgPackMapBegin(PFILEDATA filedata, U32 count);
//// READING ///////////////////////////
VOID MsgPackReadReset(PFILEDATA filedata);
RESULT MsgPackReadNextEntry(PFILEDATA filedata, PMSGPACKENTRY entry);

#ifdef __cplusplus
}
#endif