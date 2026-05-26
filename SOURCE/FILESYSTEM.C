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
    U64  path_hash;
    U64  path_time;
    U64  entries_count;
    PDET entries;
} DIRECTORYQUERY, *PDIRECTORYQUERY;

struct FILESYSTEM
{
    ALLOCATOR       allocator;
	PATH            path;
    FSTR            path_view;
	U64             path_hash;
    U64             queries_count;
    PDIRECTORYQUERY queries;
};
static int FilesystemQuerySort(const VOID *l, const VOID *r)
{
    const PDET a = (const PDET)l, b = (const PDET)r;
    return b->is_dir - a->is_dir;
}
PFILESYSTEM LoadFilesystem(ALLOCATOR allocator, FSTR path)
{
    if (!IsPathDir(path))
        return null;
    PFILESYSTEM fs = (PFILESYSTEM)allocator.alloc(allocator.user_data, sizeof(FILESYSTEM), alignof(FILESYSTEM));
    if (fs)
    {
        memzero(fs);
        fs->allocator = allocator;
        fs->path_view = PathAbs(path, fs->path);
        fs->path_hash = Hash64(fdata(fs->path_view.str, fs->path_view.size), 0);
    }
    return fs;
}
VOID FreeFilesystem(PFILESYSTEM fs)
{
    FilesystemClear(fs);
    Free(&fs->allocator, fs);
}
static DIRECTORYQUERY FilesystemScanDirectory(PFILESYSTEM fs)
{
    DIRECTORYQUERY query =
    {
        .path_hash = fs->path_hash,
        .path_time = PathTime(fs->path_view),
    };
#if _WIN32
    PATH search_path;
    snprintf(search_path, sizeof(search_path), "%s\\*", fs->path);

    WIN32_FIND_DATAA find_data;
    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE)
        return query;

    do
    {
        PCHAR name = find_data.cFileName;
        if (strcmp(name, ".") and strcmp(name, ".."))
        {
            query.entries = (PDET)Realloc(&fs->allocator, query.entries, sizeof(DET) * (query.entries_count + 1), alignof(DET));
            
            PDET e    = &query.entries[query.entries_count++];
            e->name   = FstrStrdup(&fs->allocator, fstr(name, strlen(name)));
            e->is_dir = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? true : false;
        }
    } while (FindNextFileA(hFind, &find_data));
    FindClose(hFind);
#else
    DIR *dir = opendir(fs->path);
    if (!dir)
        return query;
    query.path_hash = fs->path_hash;

    struct dirent *ent;
    while ((ent = readdir(dir)))
    {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        PATH full;
        snprintf(full, sizeof(full), "%s/%s", fs->path, ent->d_name);

        struct stat st;
        if (stat(full, &st) != 0)
            continue;

        query.entries = (PDET)Realloc(&fs->allocator, query.entries, sizeof(DET) * (query.entries_count + 1), alignof(DET));

        PDET e    = &query.entries[query.entries_count++];
        e->name   = Strdup(&fs->allocator, fstr(ent->d_name, strlen(ent->d_name)));
        e->is_dir = S_ISREG(st.st_mode) ? 0 : 1;
    }
    closedir(dir);
#endif
    if (query.entries_count)
    {
        qsort(query.entries, query.entries_count, sizeof(DET), FilesystemQuerySort);
    }
    return query;
}
static PDIRECTORYQUERY QueryFilesystemPath(PFILESYSTEM fs)
{
    for (U32 i = 0; i < fs->queries_count; i++) // Check cache
    {
        PDIRECTORYQUERY query = &fs->queries[i];
        if (query->path_hash == fs->path_hash)
        {
            if (query->path_time != PathTime(fs->path_view))
            {
                for (U64 i = 0; i < query->entries_count; i++)
                    Free(&fs->allocator, query->entries[i].name.str);

                Free(&fs->allocator, query->entries);
                *query = FilesystemScanDirectory(fs);
            }
            return query;
        }
    }
    DIRECTORYQUERY new_query = FilesystemScanDirectory(fs);
    fs->queries = (PDIRECTORYQUERY)Realloc(&fs->allocator, fs->queries, sizeof(DIRECTORYQUERY) * (fs->queries_count + 1), alignof(DIRECTORYQUERY));
    fs->queries[fs->queries_count] = new_query;
    return &fs->queries[fs->queries_count++];
}
PDET FilesystemEntries(PFILESYSTEM fs, U64 *count)
{
    PDIRECTORYQUERY q = QueryFilesystemPath(fs);
    *count = q->entries_count;
    return q->entries;
}
PDET FilesystemFiles(PFILESYSTEM fs, U64 *count)
{
    PDIRECTORYQUERY q = QueryFilesystemPath(fs);
    for (U64 i = 0; i < q->entries_count; i++)
    {
        if (q->entries[i].is_dir == 0)
        {
            *count = q->entries_count - i;
            return &q->entries[i];
        }
    }
    *count = 0;
    return null;
}
PDET FilesystemDirs(PFILESYSTEM fs, U64 *count)
{
    PDIRECTORYQUERY q = QueryFilesystemPath(fs);
    for (U64 i = 0; i < q->entries_count; i++)
    {
        if (q->entries[i].is_dir == 0)
        {
            *count = i;
            return &q->entries[0];
        }
    }
    *count = 0;
    return null;
}
VOID FilesystemClear(PFILESYSTEM fs)
{
    for (U32 i = 0; i < fs->queries_count; i++)
    {
        PDIRECTORYQUERY query = &fs->queries[i];
        for (U32 j = 0; j < query->entries_count; j++)
            Free(&fs->allocator, (PCHAR)query->entries[j].name.str);
        Free(&fs->allocator, (PCHAR)query->entries);
        query->entries_count = 0;
    }
    Free(&fs->allocator, (PCHAR)fs->queries);
    fs->queries       = null;
    fs->queries_count = 0;
}
BOOL FilesystemGoto(PFILESYSTEM fs, FSTR path)
{
    if (IsPathAbs(path))
    {
        if (!IsPathDir(path))
            return false;
        memcpy_s(fs->path, sizeof(fs->path)-1, path.str, path.size);
        fs->path[path.size] = '\0';
        fs->path_view = LIT(FSTR, .str = fs->path, .size = path.size);
    }
    else
    {
        PATH new_path;
        I64 len = snprintf(new_path, sizeof(new_path), "%s/%s", fs->path, path.str);
        if ((len <= 0) || !IsPathDir(LIT(FSTR, .str = new_path, .size = len)))
            return false;
        memcpy_s(fs->path, sizeof(fs->path), new_path, len + 1);
        fs->path_view = LIT(FSTR, .str = fs->path, .size = len);
    }
    fs->path_hash = Hash64(fdata(fs->path_view.str, fs->path_view.size), 0);
    return true;
}
BOOL FilesystemUp(PFILESYSTEM fs)
{
    return FilesystemGoto(fs, LIT(FSTR, .str = "..", .size = 2));
}
FSTR FilesystemPath(PFILESYSTEM fs, OPATH path)
{
    strncpy_s(path, sizeof(PATH) - 1, fs->path_view.str, fs->path_view.size);
    path[fs->path_view.size] = '\0';
    return fs->path_view;
}
FSTR FilesystemAbs(PFILESYSTEM fs, FSTR relative, OPATH absolute)
{
    if (!fs || !relative.str)
        return FSTR_INVALID;
    absolute[0] = '\0';
#ifdef _WIN32
    if (IsPathAbs(relative))
    {
        if (!_fullpath(absolute, relative.str, sizeof(PATH)))
            return FSTR_INVALID;
    }
    else
    {
        if (snprintf(absolute, sizeof(PATH), "%s\\%s", fs->path, relative.str) < 0)
            return FSTR_INVALID;
        if (!_fullpath(absolute, absolute, sizeof(PATH)))
            return FSTR_INVALID;
    }
#else
    if (IsPathAbs(relative))
    {
        if (!realpath(relative.str, absolute))
            return FSTR_INVALID;
    }
    else
    {
        if (snprintf_portable(absolute, sizeof(absolute), "%s/%s", file_system->path, relative.str) < 0)
            return FSTR_INVALID;
        if (!realpath(absolute, absolute))
            return FSTR_INVALID;
    }
#endif
    U64 len = strlen(absolute);
    return LIT(FSTR, .str = absolute, .size = len);
}


FSTR OpenFileDialog(CSTR filter, IOPATH file)
{
#if defined(_WIN32)
    OPENFILENAMEA ofn = { 0 };
    ofn.lStructSize   = sizeof(ofn);
    ofn.lpstrFile     = file;
    ofn.nMaxFile      = sizeof(PATH);
    ofn.lpstrFilter   = filter ? filter : "All Files\0*.*\0";
    ofn.Flags         = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileNameA(&ofn))
    {
        U64 len = strlen(file);
        return fstr(file, len);
    }
    return FSTR_INVALID;
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
    ((VOID (*)(id, SEL, BOOL))objc_msgSend)((id)panel, sel_registerName("setCanChooseFiles:"), YES);

    NSInteger result = ((NSInteger(*)(id, SEL))objc_msgSend)((id)panel, sel_registerName("runModal"));

    if (result == 1) // NSOKButton 
    {
        id url = ((id(*)(id, SEL))objc_msgSend)((id)panel, sel_registerName("URL"));
        CSTR path = ((CSTR (*)(id, SEL))objc_msgSend)((id)url, sel_registerName("fileSystemRepresentation"));
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
FSTR SaveFileDialog(CSTR filter, IOPATH file)
{
#if defined(_WIN32)
    OPENFILENAMEA ofn = { 0 };
    ofn.lStructSize   = sizeof(ofn);
    ofn.lpstrFile     = file;
    ofn.nMaxFile      = sizeof(PATH);
    ofn.lpstrFilter   = filter ? filter : "All Files\0*.*\0";
    ofn.Flags         = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    if (GetSaveFileNameA(&ofn))
    {
        U64 len = strlen(file);
        return fstr(file, len);
    }
    return FSTR_INVALID;
#elif defined(__linux__)
    // Zenity save mode with overwrite confirmation
    char command[1024];
    snprintf(command, sizeof(command), "zenity --file-selection --save --confirm-overwrite --title=\"Save File\"");

    FILE *f = popen(command, "r");
    if (!f) return 0;

    if (fgets(file, max_length, f)) {
        size_t len = strlen(file);
        // Remove trailing newline
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
    // NSSavePanel is the standard for macOS save dialogs
    // Requires AppKit framework (-framework AppKit)
    id savePanel = ((id(*)(id, SEL))objc_msgSend)((id)objc_getClass("NSSavePanel"), sel_registerName("savePanel"));

    // Set typical save behaviors
    ((VOID (*)(id, SEL, BOOL))objc_msgSend)(savePanel, sel_registerName("setCanCreateDirectories:"), YES);

    NSInteger result = ((NSInteger(*)(id, SEL))objc_msgSend)(savePanel, sel_registerName("runModal"));

    if (result == 1) { // NSOKButton
        id url = ((id(*)(id, SEL))objc_msgSend)(savePanel, sel_registerName("URL"));
        CSTR path = ((CSTR (*)(id, SEL))objc_msgSend)(url, sel_registerName("fileSystemRepresentation"));

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
FSTR OpenDirDialog(OPATH file)
{
    file[0] = 0;
#if defined(_WIN32)
    IFileOpenDialog *dialog = null;
    IShellItem *item = null;
    PWSTR wide_path = null;

    HRESULT hr = CoInitializeEx(null, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) and hr != RPC_E_CHANGED_MODE)
        return FSTR_INVALID;

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
        return fstr(file, strlen(file));
    return FSTR_INVALID;
#elif defined(__linux__)
    // Zenity directory selection
    char command[1024];
    snprintf(command, sizeof(command), "zenity --file-selection --directory --title=\"Select Folder\"");

    FILE *f = popen(command, "r");
    if (!f) 
        return FSTR_INVALID;

    if (fgets(file, max_length, f)) 
    {
        size_t len = strlen(file);
        if (len > 0 and file[len - 1] == '\n') 
        {
            file[len - 1] = '\0';
            len--;
        }
        pclose(f);
        return fstr(file, len);
    }
    pclose(f);
    return FSTR_INVALID;
#elif defined(__APPLE__)
    // macOS OpenPanel configured for folders
    id panel = ((id(*)(id, SEL))objc_msgSend)((id)objc_getClass("NSOpenPanel"), sel_registerName("openPanel"));

    ((VOID (*)(id, SEL, BOOL))objc_msgSend)(panel, sel_registerName("setCanChooseDirectories:"), YES);
    ((VOID (*)(id, SEL, BOOL))objc_msgSend)(panel, sel_registerName("setCanChooseFiles:"), NO);
    ((VOID (*)(id, SEL, BOOL))objc_msgSend)(panel, sel_registerName("setAllowsMultipleSelection:"), NO);

    NSInteger result = ((NSInteger(*)(id, SEL))objc_msgSend)(panel, sel_registerName("runModal"));

    if (result == 1) // NSOKButton
    { 
        id url = ((id(*)(id, SEL))objc_msgSend)(panel, sel_registerName("URL"));
        CSTR path = ((CSTR (*)(id, SEL))objc_msgSend)(url, sel_registerName("fileSystemRepresentation"));

        U32 len = (U32)strlen(path);
        if (len < max_length) 
        {
            strncpy(file, path, max_length);
            return fstr(file, len);
        }
    }
    return FSTR_INVALID;
#else
    return FSTR_INVALID;
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

BOOL IsPathDir(FSTR path)
{
    PATH p;
    memcpy_s(p, sizeof(p) - 1, path.str, path.size);
    p[path.size] = '\0';
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
BOOL IsPathFile(FSTR path)
{
    PATH p;
    memcpy_s(p, sizeof(p) - 1, path.str, path.size);
    p[path.size] = '0';
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
BOOL IsPathAbs(FSTR path)
{
#if defined(_WIN32)
    return (path.str[0] and path.str[1] == ':' and (path.str[2] == '\\' or path.str[2] == '/'));
#else
    return (path.size and path.str[0] == '/');
#endif
}
BOOL IsPathRel(FSTR path)
{
    return !IsPathAbs(path);
}
FSTR PathFilename(FSTR path)
{
    if (fstrinvalid(path) or *path.str == '\0') 
        return FSTR_INVALID;

    CSTR end = path.str + path.size;
    CSTR p = end - 1;
    while (p > path.str and (*p == '/' or *p == '\\')) 
        p--;

    CSTR name_end = p + 1;
    while (p > path.str and *p != '/' and *p != '\\') 
        p--;

    CSTR filename = (*p == '/' || *p == '\\') ? p + 1 : p;
    return fstr(filename, (U64)(name_end - filename));
}
FSTR PathFileext(FSTR path)
{
	FSTR filename = PathFilename(path);
	if (fstrinvalid(filename))
		return FSTR_INVALID;
	CSTR dot = filename.str + filename.size;
	while (dot > filename.str)
	{
		if (*--dot == '.')
			return fstr(dot, (U64)(filename.str + filename.size - dot));
	}
	return FSTR_INVALID;
}
FSTR PathStem(FSTR path)
{
	FSTR filename = PathFilename(path);
	if (fstrinvalid(filename))
		return FSTR_INVALID;
	CSTR dot = filename.str + filename.size;
	while (dot > filename.str)
	{
		if (*--dot == '.')
			return fstr(filename.str, (U64)(dot - filename.str));
	}
	return filename;
}
FSTR PathAbs(FSTR path, OPATH absolute)
{
#if defined(_WIN32)
    if (_fullpath(absolute, path.str, sizeof(PATH) - 1) == NULL)
        return FSTR_INVALID;
    return LIT(FSTR, .str = absolute, .size = strlen(absolute));
#else
    if (realpath(path.str, absolute))
        return FSTR_INVALID;
    strncpy(absolute, path, sizeof(absolute) - 1);
    absolute_path[sizeof(absolute) - 1] = '\0';
    return LIT(FSTR, .str = absolute, .size = strlen(absolute));
#endif
}
U64 PathTime(FSTR path)
{
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExA(path.str, GetFileExInfoStandard, &data))
        return 0;
    ULARGE_INTEGER t;
    t.LowPart = data.ftLastWriteTime.dwLowDateTime;
    t.HighPart = data.ftLastWriteTime.dwHighDateTime;
    return t.QuadPart;
#else
    struct stat st;
    if (stat(path.str, &st) != 0)
        return 0;
    return (U64)st.st_mtime;
#endif
}
FSTR PathExecutable(OPATH path)
{
#ifdef _WIN32
    DWORD len = GetModuleFileNameA(null, path, (DWORD)(sizeof(PATH) - 1));
    if (len == 0 || len == (sizeof(PATH) - 1))
        return FSTR_INVALID;
    return LIT(FSTR, .str = path, .size = len);
#elif __APPLE__
    U32 size = (U32)(sizeof(path) - 1);
    if (_NSGetExecutablePath(path, &size) != 0)
        return FSTR_INVALID;
    return LIT(FSTR, .str = path, .size = size);
#else  // Linux
    const U32 len = readlink("/proc/self/exe", path, max_len - 1);
    if (len <= 0 || (uint32_t)len >= max_len)
        return FSTR_INVALID;
    path[len] = '\0';
    return strlen(path);
#endif
}
FSTR PathExecutableDirectory(OPATH path)
{
    FSTR fstr = PathExecutable(path);
    if (!fstr.str)
        return FSTR_INVALID;
#ifdef _WIN32
    PCHAR last = strrchr(fstr.str, '\\');
#else
    PCHAR last = strrchr(fstr.str, '/');
#endif
    if (!last)
        return FSTR_INVALID;
    fstr.size -= (fstr.str + fstr.size) - last;
    return fstr;
}
BOOL PathSetWorkingDir(FSTR path)
{
#ifdef _WIN32
    return (SetCurrentDirectoryA(path.str) != 0);
#else
    return (chdir(path.str) == 0);
#endif
}
BOOL PathSetWorkingDirToExecutable(VOID)
{
    PATH path;
    FSTR s = PathExecutableDirectory(path);
    return fstrinvalid(s) ? false : PathSetWorkingDir(s);
}
static BOOL CharIsSlash(CHAR c)
{
    return (c == '/' or c == '\\');
}
FSTR PathParent(FSTR path)
{
    if (!path.str)
        return FSTR_INVALID;

    while (path.size > 0 and CharIsSlash(path.str[path.size - 1])) // Remove trailing slashes
        path.size--;

    if (path.size == 0)
        return FSTR_INVALID;

    PCHAR last = NULL;
    for (U64 i = path.size; i > 0;) // Find last slash
    {
        if (CharIsSlash(path.str[--i]))
        {
            last = &path.str[i];
            break;
        }
    }
    return last ? path : FSTR_INVALID;
}
