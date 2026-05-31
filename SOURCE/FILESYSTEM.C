#include <TOOLKIT/FILESYSTEM.H>
#include <TOOLKIT/COMPRESSION.H>
#include <TOOLKIT/STRING.H>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <combaseapi.h>
#else
#endif

typedef struct DIRECTORYQUERY
{
   U64  PathHash;
   U64  PathTime;
   U64  EntriesCount;
   PDET Entries;
} DIRECTORYQUERY, *PDIRECTORYQUERY;

struct FILESYSTEM
{
   ALLOCATOR       Parent;
   PATH            Path;
   SV              PathView;
   U64             PathHash;
   U64             QueriesCount;
   PDIRECTORYQUERY Queries;
};
static int FilesystemQuerySort(const VOID *l, const VOID *r)
{
   const PDET a = (const PDET)l, b = (const PDET)r;
   return b->IsDir - a->IsDir;
}
PFILESYSTEM LoadFilesystem(PALLOCATOR allocator, SV path)
{
   if (not IsPathDir(path)) return null;
   if (not allocator) allocator = DefaultAllocator();
   PFILESYSTEM fs = (PFILESYSTEM)Calloc(allocator, sizeof(FILESYSTEM), alignof(FILESYSTEM));
   if (not fs) return null;
   fs->Parent   = *allocator;
   fs->PathView = PathAbs(path, fs->Path);
   fs->PathHash = Hash64(dv(fs->PathView.String, fs->PathView.Size), 0);
   return fs;
}
VOID FreeFilesystem(PFILESYSTEM fs)
{
   FilesystemClear(fs);
   Free(&fs->Parent, fs);
}
static DIRECTORYQUERY FilesystemScanDirectory(PFILESYSTEM fs)
{
   DIRECTORYQUERY query =
   {
	   .PathHash = fs->PathHash,
	   .PathTime = PathTime(fs->PathView),
   };
#if _WIN32
   PATH search_path;
   snprintf(search_path, sizeof(search_path), "%s\\*", fs->Path);

   WIN32_FIND_DATAA find_data;
   HANDLE hFind = FindFirstFileA(search_path, &find_data);
   if (hFind == INVALID_HANDLE_VALUE)
	  return query;

   do
   {
	  PCHAR name = find_data.cFileName;
	  if (strcmp(name, ".") and strcmp(name, ".."))
	  {
		 query.Entries = (PDET)Realloc(&fs->Parent, query.Entries, sizeof(DET) * (query.EntriesCount + 1), alignof(DET));

		 PDET e   = &query.Entries[query.EntriesCount++];
		 e->Name  = SVStrdup(&fs->Parent, svcstr(name));
		 e->IsDir = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? true : false;
	  }
   } while (FindNextFileA(hFind, &find_data));
   FindClose(hFind);
#else
   DIR *dir = opendir(fs->Path);
   if (!dir)
	  return query;
   query.PathHash = fs->PathHash;

   struct dirent *ent;
   while ((ent = readdir(dir)))
   {
	  if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
		 continue;

	  PATH full;
	  snprintf(full, sizeof(full), "%s/%s", fs->Path, ent->d_name);

	  struct stat st;
	  if (stat(full, &st) != 0)
		 continue;

	  query.entries = (PDET)Realloc(&fs->Parent, query.entries, sizeof(DET) * (query.EntriesCount + 1), alignof(DET));

	  PDET e = &query.entries[query.EntriesCount++];
	  e->name = Strdup(&fs->Parent, fstr(ent->d_name, strlen(ent->d_name)));
	  e->IsDir = S_ISREG(st.st_mode) ? 0 : 1;
   }
   closedir(dir);
#endif
   if (query.EntriesCount)
	  qsort(query.Entries, query.EntriesCount, sizeof(DET), FilesystemQuerySort);
   
   return query;
}
static PDIRECTORYQUERY QueryFilesystemPath(PFILESYSTEM fs)
{
   for (U32 i = 0; i < fs->QueriesCount; i++) // Check cache
   {
	  PDIRECTORYQUERY query = &fs->Queries[i];
	  if (query->PathHash == fs->PathHash)
	  {
		 if (query->PathTime != PathTime(fs->PathView))
		 {
			for (U64 i = 0; i < query->EntriesCount; i++)
			   Free(&fs->Parent, query->Entries[i].Name.String);

			Free(&fs->Parent, query->Entries);
			*query = FilesystemScanDirectory(fs);
		 }
		 return query;
	  }
   }
   DIRECTORYQUERY new_query = FilesystemScanDirectory(fs);
   fs->Queries = (PDIRECTORYQUERY)Realloc(&fs->Parent, fs->Queries, sizeof(DIRECTORYQUERY) * (fs->QueriesCount + 1), alignof(DIRECTORYQUERY));
   fs->Queries[fs->QueriesCount] = new_query;
   return &fs->Queries[fs->QueriesCount++];
}
PDET FilesystemEntries(PFILESYSTEM fs, U64 *count)
{
   PDIRECTORYQUERY q = QueryFilesystemPath(fs);
   *count = q->EntriesCount;
   return q->Entries;
}
PDET FilesystemFiles(PFILESYSTEM fs, U64 *count)
{
   PDIRECTORYQUERY q = QueryFilesystemPath(fs);
   for (U64 i = 0; i < q->EntriesCount; i++)
   {
	  if (q->Entries[i].IsDir == 0)
	  {
		 *count = q->EntriesCount - i;
		 return &q->Entries[i];
	  }
   }
   *count = 0;
   return null;
}
PDET FilesystemDirs(PFILESYSTEM fs, U64 *count)
{
   PDIRECTORYQUERY q = QueryFilesystemPath(fs);
   for (U64 i = 0; i < q->EntriesCount; i++)
   {
	  if (q->Entries[i].IsDir == 0)
	  {
		 *count = i;
		 return &q->Entries[0];
	  }
   }
   *count = 0;
   return null;
}
VOID FilesystemClear(PFILESYSTEM fs)
{
   PALLOCATOR alloc = &fs->Parent;
   for (U32 i = 0; i < fs->QueriesCount; i++)
   {
	  PDIRECTORYQUERY query = &fs->Queries[i];
	  for (U32 j = 0; j < query->EntriesCount; j++)
		 SVFree(alloc, query->Entries[j].Name);
	  Free(alloc, query->Entries);
	  query->EntriesCount = 0;
   }
   Free(alloc, fs->Queries);
   fs->Queries = null;
   fs->QueriesCount = 0;
}
BOOL FilesystemGoto(PFILESYSTEM fs, SV path)
{
   if (IsPathAbs(path))
   {
	  if (!IsPathDir(path))
		 return false;
	  memcpy_s(fs->Path, sizeof(fs->Path) - 1, path.String, path.Size);
	  fs->Path[path.Size] = '\0';
	  fs->PathView = sv(fs->Path, path.Size);
   }
   else
   {
	  PATH new_path;
	  I64 len = snprintf(new_path, sizeof(new_path), "%s/%s", fs->Path, path.String);
	  if ((len <= 0) or not IsPathDir(sv(new_path, len)))
		 return false;
	  memcpy_s(fs->Path, sizeof(fs->Path), new_path, len + 1);
	  fs->PathView = sv(fs->Path, len);
   }
   fs->PathHash = Hash64(dv(fs->PathView.String, fs->PathView.Size), 0);
   return true;
}
BOOL FilesystemUp(PFILESYSTEM fs)
{
   return FilesystemGoto(fs, svlit(".."));
}
SV FilesystemPath(PFILESYSTEM fs, OPATH path)
{
   strncpy_s(path, sizeof(PATH) - 1, fs->PathView.String, fs->PathView.Size);
   path[fs->PathView.Size] = '\0';
   return fs->PathView;
}
SV FilesystemAbs(PFILESYSTEM fs, SV relative, OPATH absolute)
{
   if (!fs || !relative.String)
	  return SV_INVALID;
   absolute[0] = '\0';
#ifdef _WIN32
   if (IsPathAbs(relative))
   {
	  if (!_fullpath(absolute, relative.String, sizeof(PATH)))
		 return SV_INVALID;
   }
   else
   {
	  if (snprintf(absolute, sizeof(PATH), "%s\\%s", fs->Path, relative.String) < 0)
		 return SV_INVALID;
	  if (!_fullpath(absolute, absolute, sizeof(PATH)))
		 return SV_INVALID;
   }
#else
   if (IsPathAbs(relative))
   {
	  if (!realpath(relative.Str, absolute))
		 return SV_INVALID;
   }
   else
   {
	  if (snprintf_portable(absolute, sizeof(absolute), "%s/%s", file_system->path, relative.Str) < 0)
		 return SV_INVALID;
	  if (!realpath(absolute, absolute))
		 return SV_INVALID;
   }
#endif
   return svcstr(absolute);
}


SV OpenFileDialog(CSTR filter, IOPATH file)
{
#if defined(_WIN32)
   OPENFILENAMEA ofn = { 0 };
   ofn.lStructSize = sizeof(ofn);
   ofn.lpstrFile = file;
   ofn.nMaxFile = sizeof(PATH);
   ofn.lpstrFilter = filter ? filter : "All Files\0*.*\0";
   ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
   if (GetOpenFileNameA(&ofn))
	  return svcstr(file);
   return SV_INVALID;
#elif defined(__linux__)
   // Using Zenity via popen for a zero-dependency GTK-style dialog
   char command[1024];
   snprintf(command, sizeof(command), "zenity --file-selection --title=\"Open File\"");

   FILE *f = popen(command, "r");
   if (!f)
	  return 0;

   if (fgets(file, max_length, f))
   {
	  size_t len = strlen(file);
	  if (len > 0 and file[len - 1] == '\n') {
		 file[len - 1] = '\0';
		 len--;
	  }
	  pclose(f);
	  return (U32)len;
   }
   pclose(f);
   return 0;
#elif defined(__APPLE__)
   // This requires linking with -framework AppKit and potentially naming the file .m
   // This is a simplified C-to-ObjC logic bridge
   VOID *panel = ((id(*)(id, SEL))objc_msgSend)((id)objc_getClass("NSOpenPanel"), sel_registerName("openPanel"));
   ((VOID(*)(id, SEL, BOOL))objc_msgSend)((id)panel, sel_registerName("setCanChooseFiles:"), YES);

   NSInteger result = ((NSInteger(*)(id, SEL))objc_msgSend)((id)panel, sel_registerName("runModal"));

   if (result == 1) // NSOKButton 
   {
	  id url = ((id(*)(id, SEL))objc_msgSend)((id)panel, sel_registerName("URL"));
	  CSTR path = ((CSTR(*)(id, SEL))objc_msgSend)((id)url, sel_registerName("fileSystemRepresentation"));
	  U32 len = (U32)strlen(path);
	  if (len < max_length) {
		 strncpy(file, path, max_length);
		 return len;
	  }
   }
   return 0;
#else
   return 0;
#endif
}
SV SaveFileDialog(CSTR filter, IOPATH file)
{
#if defined(_WIN32)
   OPENFILENAMEA ofn = { 0 };
   ofn.lStructSize = sizeof(ofn);
   ofn.lpstrFile = file;
   ofn.nMaxFile = sizeof(PATH);
   ofn.lpstrFilter = filter ? filter : "All Files\0*.*\0";
   ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
   if (GetSaveFileNameA(&ofn))
	  return svcstr(file);
   return SV_INVALID;
#elif defined(__linux__)
   // Zenity save mode with overwrite confirmation
   char command[1024];
   snprintf(command, sizeof(command), "zenity --file-selection --save --confirm-overwrite --title=\"Save File\"");

   FILE *f = popen(command, "r");
   if (!f) return 0;

   if (fgets(file, max_length, f)) 
   {
	  size_t len = strlen(file);
	  // Remove trailing newline
	  if (len > 0 and file[len - 1] == '\n') 
	  {
		 file[len - 1] = '\0';
		 len--;
	  }
	  pclose(f);
	  return (U32)len;
   }
   pclose(f);
   return SV_INVALID;
#elif defined(__APPLE__)
   // NSSavePanel is the standard for macOS save dialogs
   // Requires AppKit framework (-framework AppKit)
   id savePanel = ((id(*)(id, SEL))objc_msgSend)((id)objc_getClass("NSSavePanel"), sel_registerName("savePanel"));

   // Set typical save behaviors
   ((VOID(*)(id, SEL, BOOL))objc_msgSend)(savePanel, sel_registerName("setCanCreateDirectories:"), YES);

   NSInteger result = ((NSInteger(*)(id, SEL))objc_msgSend)(savePanel, sel_registerName("runModal"));

   if (result == 1) { // NSOKButton
	  id url = ((id(*)(id, SEL))objc_msgSend)(savePanel, sel_registerName("URL"));
	  CSTR path = ((CSTR(*)(id, SEL))objc_msgSend)(url, sel_registerName("fileSystemRepresentation"));

	  U32 len = (U32)strlen(path);
	  if (len < max_length) {
		 strncpy(file, path, max_length);
		 return sv(path, len);
	  }
   }
   return SV_INVALID;
#else
   return 0;
#endif
}
SV OpenDirDialog(OPATH file)
{
   file[0] = 0;
#if defined(_WIN32)
   IFileOpenDialog *dialog = null;
   IShellItem *item = null;
   PWSTR wide_path = null;

   HRESULT hr = CoInitializeEx(null, COINIT_APARTMENTTHREADED);
   if (FAILED(hr) and hr != RPC_E_CHANGED_MODE)
	  return SV_INVALID;

#ifdef __cplusplus
   hr = CoCreateInstance(CLSID_FileOpenDialog, null, CLSCTX_ALL, IID_IFileOpenDialog, (PPVOID)&dialog);

   if (FAILED(hr))
	  goto cleanup;

   DWORD options;
   dialog->GetOptions(&options);
   dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

   hr = dialog->Show(null);
   if (FAILED(hr))
	  goto cleanup;

   hr = dialog->GetResult(&item);
   if (FAILED(hr))
	  goto cleanup;

   hr = item->GetDisplayName(SIGDN_FILESYSPATH, &wide_path);
   if (FAILED(hr))
	  goto cleanup;
#else
   hr = CoCreateInstance(&CLSID_FileOpenDialog, null, CLSCTX_ALL, &IID_IFileOpenDialog, (PPVOID)&dialog);
   if (FAILED(hr))
	  goto cleanup;

   DWORD options;
   dialog->lpVtbl->GetOptions(dialog, &options);
   dialog->lpVtbl->SetOptions(dialog, options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

   hr = dialog->lpVtbl->Show(dialog, null);
   if (FAILED(hr))
	  goto cleanup;

   hr = dialog->lpVtbl->GetResult(dialog, &item);
   if (FAILED(hr))
	  goto cleanup;

   hr = item->lpVtbl->GetDisplayName(item, SIGDN_FILESYSPATH, &wide_path);
   if (FAILED(hr))
	  goto cleanup;
#endif

   int required = WideCharToMultiByte(CP_UTF8, 0, wide_path, -1, null, 0, null, null);
   if (required <= 0 || (U32)required > sizeof(PATH))
	  goto cleanup;

   WideCharToMultiByte(CP_UTF8, 0, wide_path, -1, file, (int)sizeof(PATH), null, null);
   file[sizeof(PATH) - 1] = '\0';
cleanup:
   if (wide_path)
	  CoTaskMemFree(wide_path);

#ifdef __cplusplus
   if (item)
	  item->Release();

   if (dialog)
	  dialog->Release();
#else
   if (item)
	  item->lpVtbl->Release(item);

   if (dialog)
	  dialog->lpVtbl->Release(dialog);
#endif

   CoUninitialize();
   if (file[0] != '\0')
	  return svcstr(file);
   return  SV_INVALID;
#elif defined(__linux__)
   // Zenity directory selection
   char command[1024];
   snprintf(command, sizeof(command), "zenity --file-selection --directory --title=\"Select Folder\"");

   FILE *f = popen(command, "r");
   if (!f)
	  return SV_INVALID;

   if (fgets(file, max_length, f))
   {
	  size_t len = strlen(file);
	  if (len > 0 and file[len - 1] == '\n')
	  {
		 file[len - 1] = '\0';
		 len--;
	  }
	  pclose(f);
	  return sv(file, len);
   }
   pclose(f);
   return SV_INVALID;
#elif defined(__APPLE__)
   // macOS OpenPanel configured for folders
   id panel = ((id(*)(id, SEL))objc_msgSend)((id)objc_getClass("NSOpenPanel"), sel_registerName("openPanel"));

   ((VOID(*)(id, SEL, BOOL))objc_msgSend)(panel, sel_registerName("setCanChooseDirectories:"), YES);
   ((VOID(*)(id, SEL, BOOL))objc_msgSend)(panel, sel_registerName("setCanChooseFiles:"), NO);
   ((VOID(*)(id, SEL, BOOL))objc_msgSend)(panel, sel_registerName("setAllowsMultipleSelection:"), NO);

   NSInteger result = ((NSInteger(*)(id, SEL))objc_msgSend)(panel, sel_registerName("runModal"));

   if (result == 1) // NSOKButton
   {
	  id url = ((id(*)(id, SEL))objc_msgSend)(panel, sel_registerName("URL"));
	  CSTR path = ((CSTR(*)(id, SEL))objc_msgSend)(url, sel_registerName("fileSystemRepresentation"));

	  U32 len = (U32)strlen(path);
	  if (len < max_length)
	  {
		 strncpy(file, path, max_length);
		 return sv(file, len);
	  }
   }
   return SV_INVALID;
#else
   return SV_INVALID;
#endif
}
BOOL MessageDialog(CSTR title, CSTR message)
{
#if defined(_WIN32)
   return (BOOL)MessageBoxA(null, message, title, MB_OKCANCEL | MB_ICONINFORMATION) == IDOK;
#elif defined(__linux__)
   char cmd[1024];
   snprintf(cmd, sizeof(cmd), "zenity --question --title=\"%s\" --text=\"%s\"", title, message);
   return system(cmd) == 0;
#elif defined(__APPLE__)
   char cmd[1024];
   snprintf(cmd, sizeof(cmd), "osascript -e 'display dialog \"%s\" with title \"%s\"'", message, title);
   return system(cmd) == 0;
#else
   return 0;
#endif
}
VOID NotifyDialog(CSTR title, CSTR message)
{
#if defined(_WIN32)
   MessageBoxA(null, message, title, MB_OK | MB_ICONINFORMATION);
#elif defined(__linux__)
   char cmd[1024];
   snprintf(cmd, sizeof(cmd), "zenity --info --title=\"%s\" --text=\"%s\"", title, message);
   system(cmd);
#elif defined(__APPLE__)
   char cmd[1024];
   snprintf(cmd, sizeof(cmd), "osascript -e 'display notification \"%s\" with title \"%s\"'", message, title);
   system(cmd);
#endif
}
BOOL QueryBreakpoint(CSTR title, CSTR description)
{
   return MessageDialog(title, description) == true;
}

BOOL IsPathDir(SV path)
{
   PATH p;
   memcpy_s(p, sizeof(p) - 1, path.String, path.Size);
   p[path.Size] = '\0';
#if defined(_WIN32)
   DWORD attr = GetFileAttributesA(p);
   return (attr != INVALID_FILE_ATTRIBUTES) and (attr & FILE_ATTRIBUTE_DIRECTORY);
#else
   struct stat path_stat;
   if (stat(p, &path_stat) != 0) // Imaginary
	  return false;
   return !S_ISREG(path_stat.st_mode);
#endif
}
BOOL IsPathFile(SV path)
{
   PATH p;
   memcpy_s(p, sizeof(p) - 1, path.String, path.Size);
   p[path.Size] = '\0';
#if defined(_WIN32)
   DWORD attr = GetFileAttributesA(p);
   return (attr != INVALID_FILE_ATTRIBUTES) and !(attr & FILE_ATTRIBUTE_DIRECTORY);
#else
   struct stat path_stat;
   if (stat(p, &path_stat) != 0) // Imaginary
	  return false;
   return S_ISREG(path_stat.st_mode);
#endif
}
BOOL IsPathAbs(SV path)
{
#if defined(_WIN32)
   return (path.String[0] and path.String[1] == ':' and (path.String[2] == '\\' or path.String[2] == '/'));
#else
   return (path.Size and path.Str[0] == '/');
#endif
}
BOOL IsPathRel(SV path)
{
   return !IsPathAbs(path);
}
SV PathFilename(SV path)
{
   if (svinvalid(path) or *path.String == '\0')
	  return SV_INVALID;

   CSTR end = path.String + path.Size;
   CSTR p = end - 1;
   while (p > path.String and (*p == '/' or *p == '\\'))
	  p--;

   CSTR name_end = p + 1;
   while (p > path.String and *p != '/' and *p != '\\')
	  p--;

   CSTR filename = (*p == '/' || *p == '\\') ? p + 1 : p;
   return sv(filename, (U64)(name_end - filename));
}
SV PathFileext(SV path)
{
   SV filename = PathFilename(path);
   if (svinvalid(filename))
	  return SV_INVALID;
   CSTR dot = filename.String + filename.Size;
   while (dot > filename.String)
   {
	  if (*--dot == '.')
		 return sv(dot, (U64)(filename.String + filename.Size - dot));
   }
   return SV_INVALID;
}
SV PathStem(SV path)
{
   SV filename = PathFilename(path);
   if (svinvalid(filename))
	  return SV_INVALID;
   CSTR dot = filename.String + filename.Size;
   while (dot > filename.String)
   {
	  if (*--dot == '.')
		 return sv(filename.String, (U64)(dot - filename.String));
   }
   return filename;
}
SV PathAbs(SV path, OPATH absolute)
{
#if defined(_WIN32)
   if (_fullpath(absolute, path.String, sizeof(PATH) - 1) == NULL)
	  return SV_INVALID;
   return svcstr(absolute);
#else
   if (realpath(path.Str, absolute))
	  return SV_INVALID;
   strncpy(absolute, path, sizeof(absolute) - 1);
   absolute_path[sizeof(absolute) - 1] = '\0';
   return svcstr(absolute);
#endif
}
U64 PathTime(SV path)
{
#ifdef _WIN32
   WIN32_FILE_ATTRIBUTE_DATA data;
   if (!GetFileAttributesExA(path.String, GetFileExInfoStandard, &data))
	  return 0;
   ULARGE_INTEGER t;
   t.LowPart = data.ftLastWriteTime.dwLowDateTime;
   t.HighPart = data.ftLastWriteTime.dwHighDateTime;
   return t.QuadPart;
#else
   struct stat st;
   if (stat(path.Str, &st) != 0)
	  return 0;
   return (U64)st.st_mtime;
#endif
}
SV PathExecutable(OPATH path)
{
#ifdef _WIN32
   DWORD len = GetModuleFileNameA(null, path, (DWORD)(sizeof(PATH) - 1));
   if (len == 0 || len == (sizeof(PATH) - 1))
	  return SV_INVALID;
   return sv(path, (U64)len);
#elif __APPLE__
   U32 size = (U32)(sizeof(path) - 1);
   if (_NSGetExecutablePath(path, &size) != 0)
	  return SV_INVALID;
   return sv(path, size);
#else  // Linux
   const U32 len = readlink("/proc/self/exe", path, max_len - 1);
   if (len <= 0 || (uint32_t)len >= max_len)
	  return SV_INVALID;
   path[len] = '\0';
   return svcstr(path);
#endif
}
SV PathExecutableDirectory(OPATH path)
{
   SV p = PathExecutable(path);
   if (svinvalid(p)) return SV_INVALID;
   I64 last = SVChr(p, '\\');
   if (last < 0) return SV_INVALID;
   last = SVChr(p, '/');
   if (last < 0) return SV_INVALID;
   p.Size -= (U64)(p.String + p.Size) - last;
   return p;
}
BOOL PathSetWorkingDir(SV path)
{
#ifdef _WIN32
   return (SetCurrentDirectoryA(path.String) != 0);
#else
   return (chdir(path.Str) == 0);
#endif
}
BOOL PathSetWorkingDirToExecutable(VOID)
{
   PATH path;
   SV s = PathExecutableDirectory(path);
   return svinvalid(s) ? false : PathSetWorkingDir(s);
}
static BOOL CharIsSlash(CHAR c)
{
   return (c == '/' or c == '\\');
}
SV PathParent(SV path)
{
   if (!path.String)
	  return SV_INVALID;

   while (path.Size > 0 and CharIsSlash(path.String[path.Size - 1])) // Remove trailing slashes
	  path.Size--;

   if (path.Size == 0)
	  return SV_INVALID;

   for (U64 i = path.Size; i > 0;) // Find last slash
   {
	  if (CharIsSlash(path.String[--i]))
	  {
		 path.Size = i;
		 return path.Size > 0 ? path : SV_INVALID;
	  }
   }
   return SV_INVALID;
}
SV PathJoin(PALLOCATOR allocator, U32 strings_count, PSV strings)
{
   return SVJoinList(allocator,
#ifdef _WIN32
	  '\\',
#else
	  '/',
#endif
	  strings_count, strings
   );
}
