#pragma once

#include <TOOLKIT/TYPES.H>

#ifdef __cplusplus
extern "C" {
#endif

enum LOGGING_
{
    LOGGING_SUCCESS,
    LOGGING_NOTE,
    LOGGING_INFO,
    LOGGING_SPECIAL,
    LOGGING_WARNING,
    LOGGING_ERROR,
    LOGGING_FATAL,
    LOGGING_DEBUG,
    LOGGING_DEBUGERROR,
    LOGGING_VERBOSE,

    LOGGING_TYPECOUNT,

    LOGGING_TYPEBITS = 63, // Reserved bits for the logging type

    LOGGING_NEWLINE = 64,
};
typedef U32 LOGGING;

typedef enum ARGTYPE_
{
    ARGTYPE_BOOL,
    ARGTYPE_I32,
    ARGTYPE_U32,
    ARGTYPE_F32,
    ARGTYPE_F64,
    ARGTYPE_CSTR,
    ARGTYPE_PATH, // @TODO: Just like string but turning a path (File or Folder) into absolute path.

    ARGTYPE_COUNT,
} ARGTYPE;

typedef struct ARGOPTION
{
    CSTR    option;
    U64     type_count;
    ARGTYPE type;
    PVOID   data;
} ARGOPTION, *PARGOPTION;

makeopaque(PIPE);

typedef enum PROCESSMODE_
{
    PROCESSMODE_NONE,
    PROCESSMODE_READ,
    PROCESSMODE_WRITE,
    PROCESSMODE_READWRITE,
} PROCESSMODE;

typedef enum PROCESSSTATUS_
{
    PROCESSSTATUS_RUNNING,
    PROCESSSTATUS_EXITED,
    PROCESSSTATUS_FAILED,
} PROCESSSTATUS;

typedef struct PROCESS
{
    ALLOCATOR   Allocator;
    U64         Id;
    PPIPE       ReadOut;      // Pipe to read from child stdout.
    PPIPE       WriteIn;      // Pipe to write to child stdin.
    PVOID       ProcessHandle;
    PVOID       ThreadHandle;
    U64         ProcessId;
    U64         ThreadId;
    PROCESSMODE Mode;
    I32         ExitCode;
} PROCESS; makeopaque(PROCESS);

// / / / / / / / / / / / / / / / / / / / 
// PRINT                               /
// / / / / / / / / / / / / / / / / / / /
#define succ(fmt, ...)  Log(LOGGING_SUCCESS, fmt, ##__VA_ARGS__)
#define note(fmt, ...)  Log(LOGGING_NOTE, fmt, ##__VA_ARGS__)
#define info(fmt, ...)  Log(LOGGING_INFO, fmt, ##__VA_ARGS__)
#define spec(fmt, ...)  Log(LOGGING_SPECIAL, fmt, ##__VA_ARGS__)
#define warn(fmt, ...)  Log(LOGGING_WARNING, fmt, ##__VA_ARGS__)
#define err(fmt, ...)   Log(LOGGING_ERROR, fmt, ##__VA_ARGS__)
#define fatal(fmt, ...) Log(LOGGING_FATAL, fmt, ##__VA_ARGS__)
#define debug(fmt, ...) Log(LOGGING_DEBUG, fmt, ##__VA_ARGS__)
#define derr(fmt, ...)  Log(LOGGING_DEBUGERROR, fmt, ##__VA_ARGS__)
#define verb(fmt, ...)  Log(LOGGING_VERBOSE, fmt, ##__VA_ARGS__)
// / / / / / / / / / / / / / / / / / / / 
// PRINTLN                             /
// / / / / / / / / / / / / / / / / / / /
#define succln(fmt, ...)  Log(LOGGING_SUCCESS | LOGGING_NEWLINE, fmt, ##__VA_ARGS__)
#define noteln(fmt, ...)  Log(LOGGING_NOTE | LOGGING_NEWLINE, fmt, ##__VA_ARGS__)
#define infoln(fmt, ...)  Log(LOGGING_INFO | LOGGING_NEWLINE, fmt, ##__VA_ARGS__)
#define specln(fmt, ...)  Log(LOGGING_SPECIAL | LOGGING_NEWLINE, fmt, ##__VA_ARGS__)
#define warnln(fmt, ...)  Log(LOGGING_WARNING | LOGGING_NEWLINE, fmt, ##__VA_ARGS__)
#define errln(fmt, ...)   Log(LOGGING_ERROR | LOGGING_NEWLINE, fmt, ##__VA_ARGS__)
#define fatalln(fmt, ...) Log(LOGGING_FATAL | LOGGING_NEWLINE, fmt, ##__VA_ARGS__)
#define debugln(fmt, ...) Log(LOGGING_DEBUG | LOGGING_NEWLINE, fmt, ##__VA_ARGS__)
#define derrln(fmt, ...)  Log(LOGGING_DEBUGERROR | LOGGING_NEWLINE, fmt, ##__VA_ARGS__)
#define verbln(fmt, ...)  Log(LOGGING_VERBOSE | LOGGING_NEWLINE, fmt, ##__VA_ARGS__)
void Log(LOGGING type, CSTR fmt, ...);
void SetMaxLog(LOGGING type); // limit <= (type & LOGGING_TYPEBITS)
void SetStream(PVOID stream); // Set output stream
// / / / / / / / / / / / / / / / / / / / 
// PROGRESS                            /
// / / / / / / / / / / / / / / / / / / /
void ProgressUTF(LOGGING color, I32 progress, CSTR text); // 
void Progress(LOGGING color, I32 progress, CSTR text);
// / / / / / / / / / / / / / / / / / / / 
// ARGS                                /
// / / / / / / / / / / / / / / / / / / /
BOOL LoadArgs(int argc, PPCHAR argv, U64 options_count, PARGOPTION options);
// / / / / / / / / / / / / / / / / / / / 
// COMMANDS                            /
// / / / / / / / / / / / / / / / / / / /
RESULT Run(PALLOCATOR allocator, U32 args_count, PFSTR args);
// / / / / / / / / / / / / / / / / / / / 
// PROCESS                             /
// / / / / / / / / / / / / / / / / / / /
PPROCESS      LoadProcess(PALLOCATOR allocator, PROCESSMODE mode, U32 args_count, PFSTR args);
VOID          FreeProcess(PPROCESS process);
RESULT        ProcessRead(PPROCESS process, PVOID buffer, U64 size, U64 *bytes_read);
RESULT        ProcessWrite(PPROCESS process, PVOID buffer, U64 size, U64 *bytes_written);
PROCESSSTATUS ProcessGetStatus(PPROCESS process);
RESULT        ProcessWait(PPROCESS process, I32 *exit_code);
RESULT        ProcessKill(PPROCESS process);

#ifdef __cplusplus
}
#endif
