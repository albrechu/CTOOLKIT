#pragma once

#include <TOOLKIT/TYPES.H>
#include <TOOLKIT/ALLOCATOR.H>

#ifdef __cplusplus
extern "C" {
#endif

makeopaque(FILESYSTEM), **PPFILESYSTEM;

typedef struct DIRECTORYENTRY
{
    FSTR name;
    BOOL is_dir;
} DIRECTORYENTRY, *PDIRECTORYENTRY, DET, *PDET;

PFILESYSTEM LoadFilesystem(ALLOCATOR allocator, FSTR path);
VOID        FreeFilesystem(PFILESYSTEM fs);
PDET        FilesystemEntries(PFILESYSTEM fs, U64 *count);
PDET        FilesystemFiles(PFILESYSTEM fs, U64 *count);
PDET        FilesystemDirs(PFILESYSTEM fs, U64 *count);
VOID        FilesystemClear(PFILESYSTEM fs);
BOOL        FilesystemGoto(PFILESYSTEM fs, FSTR relative);
BOOL        FilesystemUp(PFILESYSTEM fs);
FSTR        FilesystemPath(PFILESYSTEM fs, OPATH path);
FSTR        FilesystemAbs(PFILESYSTEM fs, FSTR relative, OPATH absolute);

FSTR OpenFileDialog(CSTR filter, IOPATH file);
FSTR SaveFileDialog(CSTR filter, IOPATH file);
FSTR OpenDirDialog(OPATH file);
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

BOOL IsPathDir(FSTR path);
BOOL IsPathFile(FSTR path);
BOOL IsPathAbs(FSTR path);
BOOL IsPathRel(FSTR path);
FSTR PathFilename(FSTR path);
FSTR PathFileext(FSTR path);
FSTR PathStem(FSTR path);
FSTR PathAbs(FSTR path, OPATH absolute);
U64  PathTime(FSTR path);
FSTR PathExecutable(OPATH path);
FSTR PathExecutableDirectory(OPATH path);
BOOL PathSetWorkingDir(FSTR path);
BOOL PathSetWorkingDirToExecutable(VOID);
FSTR PathParent(FSTR path);
FSTR PathJoin(PALLOCATOR allocator, U32 strings_count, PFSTR strings);

#ifdef __cplusplus
}
#endif
