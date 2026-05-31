#pragma once

#include <TOOLKIT/TYPES.H>
#include <TOOLKIT/ALLOCATOR.H>

#ifdef __cplusplus
extern "C" {
#endif

makeopaque(FILESYSTEM), **PPFILESYSTEM;

typedef struct DIRECTORYENTRY
{
    SV   Name;
    BOOL IsDir;
} DIRECTORYENTRY, *PDIRECTORYENTRY, DET, *PDET;

PFILESYSTEM LoadFilesystem(PALLOCATOR allocator, SV path);
VOID        FreeFilesystem(PFILESYSTEM fs);
PDET        FilesystemEntries(PFILESYSTEM fs, U64 *count);
PDET        FilesystemFiles(PFILESYSTEM fs, U64 *count);
PDET        FilesystemDirs(PFILESYSTEM fs, U64 *count);
VOID        FilesystemClear(PFILESYSTEM fs);
BOOL        FilesystemGoto(PFILESYSTEM fs, SV relative);
BOOL        FilesystemUp(PFILESYSTEM fs);
SV        FilesystemPath(PFILESYSTEM fs, OPATH path);
SV        FilesystemAbs(PFILESYSTEM fs, SV relative, OPATH absolute);

SV OpenFileDialog(CSTR filter, IOPATH file);
SV SaveFileDialog(CSTR filter, IOPATH file);
SV OpenDirDialog(OPATH file);
BOOL MessageDialog(CSTR title, CSTR message);
VOID NotifyDialog(CSTR title, CSTR message);
BOOL QueryBreakpoint(CSTR title, CSTR description);

////////////////////////////////////////
// ASSERT                             //
////////////////////////////////////////
#ifdef NDEBUG
#define Assert(condition, msg) ((void)0)
#else
#define Assert(condition, msg) do { \
	    if (not (condition)) \
		{ \
			if (QueryBreakpoint("ASSERTION FAILED", "Condition " #condition " failed at " \
				__FILE__ ":" LINE_STRING " with message:\n\"" \
				msg "\"\nDo you want to break?") \
			) \
			{ \
				breakpoint(); \
			} \
	    } \
	} while(0)
#endif

BOOL IsPathDir(SV path);
BOOL IsPathFile(SV path);
BOOL IsPathAbs(SV path);
BOOL IsPathRel(SV path);
SV PathFilename(SV path);
SV PathFileext(SV path);
SV PathStem(SV path);
SV PathAbs(SV path, OPATH absolute);
U64  PathTime(SV path);
SV PathExecutable(OPATH path);
SV PathExecutableDirectory(OPATH path);
BOOL PathSetWorkingDir(SV path);
BOOL PathSetWorkingDirToExecutable(VOID);
SV PathParent(SV path);
SV PathJoin(PALLOCATOR allocator, U32 strings_count, PSV strings);

#ifdef __cplusplus
}
#endif
