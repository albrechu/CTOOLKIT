#include <TOOLKIT/CLI.H>
#include <TOOLKIT/FILESYSTEM.H>

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif __unix__ || defined(__APPLE__)
#include <unistd.h>    // fork, pipe, close, dup2, execvp, read, write
#include <sys/types.h> // pid_t
#include <sys/wait.h>  // waitpid, WIFEXITED, WEXITSTATUS, WNOHANG
#include <signal.h>    // kill, SIGKILL
#endif


// / / / / / / / / / / / / / / / / / / / 
// PRINT                               /
// / / / / / / / / / / / / / / / / / / /
#ifdef _DEBUG
static LOGGING log_max_level = LOGGING_VERBOSE;
#else
static LOGGING log_max_level = LOGGING_FATAL;
#endif
static FILE *log_stream = NULL;
// Setters
void SetMaxLog(LOGGING type) { log_max_level = type; }
void SetStream(PVOID stream) { log_stream    = (FILE *)stream; }

static const CSTR LOG_COLORS[] = 
{
    /*[LOGGING_SUCCESS]    */ "\033[0;1;92m",
    /*[LOGGING_NOTE]       */ "\033[0;1m",
    /*[LOGGING_INFO]       */ "\033[0m",
    /*[LOGGING_SPECIAL]    */ "\033[0;1;95m",
    /*[LOGGING_WARNING]    */ "\033[0;1;33m",
    /*[LOGGING_ERROR]      */ "\033[0;1;31m",
    /*[LOGGING_FATAL]      */ "\033[0;1;37;41m",
    /*[LOGGING_DEBUG]      */ "\033[0;1;36m",
    /*[LOGGING_DEBUGERROR] */ "\033[0;1;36;41m",
    /*[LOGGING_VERBOSE]    */ "\033[0;1;90m"
};
void Log(LOGGING type, CSTR fmt, ...)
{
    I32 color = type & LOGGING_TYPEBITS;
    if (color > log_max_level) return;
    if (color < 0 or color >= arrsize(LOG_COLORS)) unreachable();

    FILE *stream = log_stream ? log_stream : stdout; // Possibly lock and unlock here.
    va_list args;
    va_start(args, fmt);
    fputs(LOG_COLORS[color], stream);
    vfprintf(stream, fmt, args);
    va_end(args);
    fputs(type & LOGGING_NEWLINE ? "\033[0m\033[K\n" : "\033[0m\033[K", stream);
}
// / / / / / / / / / / / / / / / / / / / 
// PROGRESS                            /
// / / / / / / / / / / / / / / / / / / /
void ProgressF(CSTR blocks[9], I32 block_size, LOGGING type, I32 progress, CSTR text)
{
    I32 color = type & LOGGING_TYPEBITS;
    if (color > log_max_level)
        return;

#define BAR_WIDTH 60
    progress = progress < 0 ? 0 : (progress > 100 ? 100 : progress); // Clamp
    // (progress * width * 8) / 100
    I32 total_units = (progress * BAR_WIDTH * 8 + 50) / 100; // Add 50 before dividing by 100 to achieve rounding instead of truncation
    I32 full = total_units >> 3;  // Divide by 8
    I32 frac = total_units & 7;   // Remainder (modulo 8)

    CHAR bar[BAR_WIDTH * 4 + 1]; // Each block can be up to 4 bytes (for UTF-8), plus null terminator
    PCHAR p = &bar[0];
    for (I32 i = 0; i < full; ++i) // Fill full blocks
    {
        memcpy(p, blocks[8], block_size);
        p += block_size;
    }

    if (frac > 0 && full < BAR_WIDTH) // Fill fractional block
    {
        memcpy(p, blocks[frac], block_size);
        p += block_size;
    }

    I32 current_blocks = full + (frac > 0 ? 1 : 0);
    for (I32 i = current_blocks; i < BAR_WIDTH; ++i) // Fill empty blocks
    {
        memcpy(p, blocks[0], block_size);
        p += block_size;
    }
    *p = '\0';

    FILE *stream = log_stream ? log_stream : stdout;
    fputc('\r', stream);
    Log((LOGGING)color, "|%s| %3d%% %s", bar, progress, text);
    if (progress > 99)
        fputs("\n", stdout);

    fflush(stdout);
}
void ProgressUTF(LOGGING color, I32 progress, CSTR text)
{
    static CSTR BLOCKS[] = 
    {
        (CSTR)u8"\u2591", (CSTR)u8"\u250F", (CSTR)u8"\u2512",
        (CSTR)u8"\u2513", (CSTR)u8"\u2514", (CSTR)u8"\u2515",
        (CSTR)u8"\u2516", (CSTR)u8"\u2517", (CSTR)u8"\u2588"
    };
    ProgressF(BLOCKS, 3, color, progress, text);
}
void Progress(LOGGING color, I32 progress, CSTR text)
{
    static CSTR BLOCKS[] = { " ", ".", "-", "+", "o", "x", "X", "H", "#" };
    ProgressF(BLOCKS, 1, color, progress, text);
}

// / / / / / / / / / / / / / / / / / / / 
// ARGS                                /
// / / / / / / / / / / / / / / / / / / /
static const U64 argument_type_size[] =
{
    [ARGTYPE_BOOL] = sizeof(BOOL),
    [ARGTYPE_I32] = sizeof(I32),
    [ARGTYPE_U32] = sizeof(U32),
    [ARGTYPE_F32] = sizeof(F32),
    [ARGTYPE_F64] = sizeof(F64),
    [ARGTYPE_PATH] = sizeof(PATH),
    [ARGTYPE_CSTR] = sizeof(CSTR),
};
static BOOL ParseValue(ARGTYPE type, PCHAR string, PVOID value)
{
    PCHAR end = NULL;
    errno = 0;
    switch (type)
    {
    case ARGTYPE_BOOL:
    {
        BOOL *bvalue = (BOOL *)value;
        if (not string)
        {
            *bvalue = true;
            return true;
        }
        if (not strcmp(string, "true") or not strcmp(string, "1"))
        {
            *bvalue = true;
            return true;
        }
        if (not strcmp(string, "false") or not strcmp(string, "0"))
        {
            *bvalue = false;
            return true;
        }
        return false;
    }
    case ARGTYPE_I32:
    {
        if (not string)
        {
            *(I32 *)value = 1;
            return true;
        }
        long v = strtol(string, &end, 10);
        if (end == string or *end != '\0' or errno == ERANGE)
            return false;

        *(I32 *)value = (I32)v;
        return true;
    }
    case ARGTYPE_U32:
    {
        if (not string)
        {
            *(U32 *)value = 1;
            return true;
        }

        unsigned long v = strtoul(string, &end, 10);
        if (end == string or *end != '\0' or errno == ERANGE)
            return false;

        *(U32 *)value = (U32)v;
        return true;
    }
    case ARGTYPE_F32:
    {
        if (!string)
        {
            *(F32 *)value = 1.0f;
            return true;
        }

        F32 v = strtof(string, &end);
        if (end == string or *end != '\0' or errno == ERANGE)
            return false;

        *(F32 *)value = v;
        return true;
    }
    case ARGTYPE_F64:
    {
        if (!string)
        {
            *(F64 *)value = 1.0;
            return true;
        }
        F64 v = strtod(string, &end);
        if (end == string or *end != '\0' or errno == ERANGE)
            return false;
        *(F64 *)value = v;
        return true;
    }
    case ARGTYPE_PATH:
    {
        FSTR fstr = LIT(FSTR, .str = (PCHAR)string, .size = strlen(string));
        fstr = PathAbs(fstr, value);
        return fstr.str != NULL;
    }
    case ARGTYPE_CSTR:
    {
        CHAR delimiters[2] = { '\'', '\"' };
        for (U64 i = 0; i < 2; i++)
        {
            CHAR delimiter = delimiters[i];
            if (*string == delimiter)
            {
                PCHAR end = strrchr((PCHAR)string + 1, delimiter);
                if (not end)
                {
                    errln("Missing symbol %c at the end of string %s", delimiter, string);
                    return false;
                }
                string++;
                *end = '\0';
            }
        }
        *(PCHAR *)value = (PCHAR)string;
        return true;
    }
    default:
        return false;
    }
}
static BOOL ParseList(ARGTYPE type, PCHAR value, PVOID data, U64 max_count, U64 *count)
{
    BOOL success = true;
    U64 i = 0;
    U64 type_size = argument_type_size[type];
    PCHAR token = strtok(value, ",");
    while (token and i < max_count)
    {
        PVOID dst = (PCHAR)data + i * type_size;
        success &= ParseValue(type, token, dst);
        i++;
        token = strtok(null, ",");
    }
    *count = i;
    return success and i;
}
BOOL LoadArgs(int argc, PPCHAR argv, U64 options_count, PARGOPTION options)
{
    BOOL success = true;
    for (int i = 1; i < argc; i++)
    {
        PCHAR arg = argv[i];

        if (arg[0] != '-') // Not an option
            continue;
        PCHAR name = arg + 1 + (arg[1] == '-');

        PCHAR value = strchr(name, '=');
        if (value)
        {
            *value = '\0';
            value++;
        }

        bool known_option = false;
        bool parsed = false;
        for (U64 j = 0; j < options_count; j++)
        {
            PARGOPTION opt = &options[j];
            if (strcmp(name, opt->option) != 0) continue;

            known_option = true;
            if (opt->type >= ARGTYPE_COUNT) unreachable();
            if (opt->type_count == 0)
            {
                parsed = ParseValue(opt->type, value, opt->data);
                break;
            }

            if (value)
            {
                if (strchr(value, ','))
                {
                    U64 count;
                    parsed = ParseList(opt->type, value, opt->data, opt->type_count, &count);
                }
                else
                {
                    parsed = ParseValue(opt->type, value, opt->data);
                }
            }
            else
            {
                U64 type_size = argument_type_size[opt->type];
                parsed = true;
                for (U64 k = 0; k < opt->type_count; k++)
                {
                    if (i + 1 >= argc)
                        break;

                    void *dst = (PCHAR)opt->data + k * type_size;
                    PCHAR current_arg = argv[++i];
                    if (!ParseValue(opt->type, current_arg, dst))
                    {
                        errln("Failed to parse --%s argument '%s'", name, current_arg);
                        parsed = false;
                    }
                }
                goto next;
            }
            break;
        }
        if (!known_option)
            warnln("Unknown parameter argument --%s", name);
        else if (!parsed)
            errln("Failed to parse --%s value '%s'", name, value);
    next:
        success &= parsed;
    }
    return success;
}

// / / / / / / / / / / / / / / / / / / / 
// COMMANDS                            /
// / / / / / / / / / / / / / / / / / / /
RESULT Run(PALLOCATOR allocator, U32 args_count, PFSTR args)
{
    FSTR cmd = JoinFStrList(allocator, ' ', args_count, args);
	int result = system(cmd.str);
	Free(allocator, (PVOID)cmd.str);
    switch (result)
    {
	case 0:
		return OK;
    case -1:
        return UNDEFINED_ERROR;
	default:
        return UNDEFINED_ERROR;
    }
}

// / / / / / / / / / / / / / / / / / / / 
// PROCESS                             /
// / / / / / / / / / / / / / / / / / / /
PPROCESS LoadProcess(PALLOCATOR allocator, PROCESSMODE mode, U32 args_count, PFSTR args)
{
    errdfs(16);
    // Join Arguments
    FSTR cmd = JoinFStrList(allocator, ' ', args_count, args);
    if (fstrinvalid(cmd)) return null;
    errdf(Free, allocator, (PVOID)cmd.str);
    // Allocate process
    PPROCESS process = (PPROCESS)Calloc(allocator, sizeof(PROCESS), alignof(PROCESS));
    if (not process)
        return errdfflush(), null;
    errdf(Free, allocator, process);
    process->Allocator = *allocator;
    process->Mode = mode;
    process->ExitCode = -1;

#ifdef _WIN32
    SECURITY_ATTRIBUTES sa = { 0 };
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = true;

    PROCESS_INFORMATION pi = { 0 };
    STARTUPINFOA si = { 0 };
    si.cb = sizeof(STARTUPINFOA);

    PPIPE write_out = null; // Pipe that child process writes to internally.
    PPIPE read_in = null;   // Pipe that child process reads from internally.
    // Reading pipes
    if (mode == PROCESSMODE_READ or mode == PROCESSMODE_READWRITE)
    {
        if (!CreatePipe(&process->ReadOut, &write_out, &sa, 0))
            return errdfflush(), null;
        errdf(CloseHandle, write_out);
        errdf(CloseHandle, process->ReadOut);
        if (not SetHandleInformation(process->ReadOut, HANDLE_FLAG_INHERIT, 0)) // Read end non-inheritable
            return errdfflush(), null;
        
        si.hStdError = write_out;
        si.hStdOutput = write_out;
        si.dwFlags |= STARTF_USESTDHANDLES;
    }
    // Writing pipes
    if (mode == PROCESSMODE_WRITE or mode == PROCESSMODE_READWRITE)
    {
        if (!CreatePipe(&read_in, &process->WriteIn, &sa, 0))
            return errdfflush(), null;
        errdf(CloseHandle, process->WriteIn);
        errdf(CloseHandle, read_in);
        if (not SetHandleInformation(process->WriteIn, HANDLE_FLAG_INHERIT, 0)) // Write end non-inheritable
            return errdfflush(), null;

        si.hStdInput = read_in;
        si.dwFlags |= STARTF_USESTDHANDLES;
    }

    if (not CreateProcessA(null, cmd.str, null, null, true, 0, null, null, &si, &pi))
    {
        DWORD error = GetLastError();
        CHAR error_msg[256];
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, null, error, 0, error_msg, sizeof(error_msg), null);
        debugln("CreateProcessA failed with error '%s' (%lu)", error_msg, error);
        return errdfflush(), null;
    }

    process->ProcessHandle = pi.hProcess;
    process->ThreadHandle  = pi.hThread;
    process->ProcessId     = pi.dwProcessId;
    process->ThreadId      = pi.dwThreadId;

    if (write_out) CloseHandle(write_out); // It should be internal to the child, so don't write to it.
    if (read_in)   CloseHandle(read_in);   // Same here.

#elif defined(__unix__) || defined(__APPLE__)
    int stdout_pipe[2] = { -1, -1 };
    int stdin_pipe[2] = { -1, -1 };

    if (mode == PROCESSMODE_READ or mode == PROCESSMODE_READWRITE)
    {
        if (pipe(stdout_pipe) == -1)
            return errdfflush(), null;
    }

    if (mode == PROCESSMODE_WRITE or mode == PROCESSMODE_READWRITE)
    {
        if (pipe(stdin_pipe) == -1)
        {
            if (stdout_pipe[0] != -1) close(stdout_pipe[0]);
            if (stdout_pipe[1] != -1) close(stdout_pipe[1]);
            return errdfflush(), null;
        }
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        if (stdout_pipe[0] != -1) close(stdout_pipe[0]);
        if (stdout_pipe[1] != -1) close(stdout_pipe[1]);
        if (stdin_pipe[0] != -1) close(stdin_pipe[0]);
        if (stdin_pipe[1] != -1) close(stdin_pipe[1]);
        return errdfflush(), null;
    }

    if (pid == 0) // Child process
    {
        // Redirect read
        if (mode == PROCESSMODE_READ or mode == PROCESSMODE_READWRITE)
        {
            close(stdout_pipe[0]); // Close read end
            dup2(stdout_pipe[1], STDOUT_FILENO);
            dup2(stdout_pipe[1], STDERR_FILENO);
            close(stdout_pipe[1]);
        }
        // Redirect write
        if (mode == PROCESSMODE_WRITE or mode == PROCESSMODE_READWRITE)
        {
            close(stdin_pipe[1]); // Close write end
            dup2(stdin_pipe[0], STDIN_FILENO);
            close(stdin_pipe[0]);
        }

        // Build argv array
        PPCHAR argv = (PPCHAR)Alloc(sizeof(PCHAR) * (args_count + 1), alignof(PCHAR));
        if (not argv)
            exit(1); // OOM, exit child.
        for (U32 i = 0; i < args_count; i++)
        {
            argv[i] = (char *)args[i].str;
        }
        argv[args_count] = null;

        execvp(argv[0], argv);
        Free(allocator, argv);
        exit(1); // execvp failed, exit child.
    }

    // Parent process
    process->ProcessId = pid;
    if (mode == PROCESSMODE_READ or mode == PROCESSMODE_READWRITE)
    {
        close(stdout_pipe[1]); // It should be internal to the child, so don't write to it.
        process->ReadOut = (PPIPE)(intptr_t)stdout_pipe[0];
    }
    if (mode == PROCESSMODE_WRITE or mode == PROCESSMODE_READWRITE)
    {
        close(stdin_pipe[0]); // Same here
        process->WriteIn = (PPIPE)(intptr_t)stdin_pipe[1];
    }
#endif
    Free(allocator, (PVOID)cmd.str);
    return process;
}
RESULT ProcessRead(PPROCESS process, PVOID buffer, U64 size, U64 *bytes_read)
{
    if (!process or !buffer or !bytes_read) return INVALID_PARAMETER;
    if (process->Mode != PROCESSMODE_READ && process->Mode != PROCESSMODE_READWRITE)
        return INVALID_OPERATION;

#ifdef _WIN32
    DWORD read;
    if (not ReadFile(process->ReadOut, buffer, (DWORD)size, &read, null))
    {
        *bytes_read = read;
        DWORD ec = GetLastError();
        switch (ec)
        {
        case ERROR_BROKEN_PIPE: return END_OF_FILE;
        case ERROR_IO_PENDING: return IO_PENDING;
        default: return ec < 0 ? UNDEFINED_ERROR : UNDEFINED_NONERROR;
        }
    }
    *bytes_read = read;
#else
    ssize_t result = read((int)(intptr_t)process->ReadOut, buffer, size);
    if (result == -1) return UNDEFINED_ERROR;
    if (result == 0) return END_OF_FILE;
    *bytes_read = result;
#endif
    return OK;
}
RESULT ProcessWrite(PPROCESS process, PVOID buffer, U64 size, U64 *bytes_written)
{
    if (!process or !buffer or !bytes_written) return INVALID_PARAMETER;
    if (process->Mode != PROCESSMODE_WRITE && process->Mode != PROCESSMODE_READWRITE)
        return INVALID_OPERATION;

#ifdef _WIN32
    DWORD written;
    if (!WriteFile(process->WriteIn, buffer, (DWORD)size, &written, null))
    {
        *bytes_written = written;
        DWORD ec = GetLastError();
        return ec < 0 ? UNDEFINED_ERROR : UNDEFINED_NONERROR;
    }
    *bytes_written = written;
#else
    ssize_t result = write((int)(intptr_t)process->WriteIn, buffer, size);
    if (result == -1) return UNDEFINED_ERROR;
    *bytes_written = result;
#endif
    return OK;
}
PROCESSSTATUS ProcessGetStatus(PPROCESS process)
{
    if (not process) return PROCESSSTATUS_FAILED;

#ifdef _WIN32
    DWORD exit_code;
    if (not GetExitCodeProcess(process->ProcessHandle, &exit_code))
        return PROCESSSTATUS_FAILED;

    if (exit_code == STILL_ACTIVE)
        return PROCESSSTATUS_RUNNING;

    process->ExitCode = (I32)exit_code;
    return PROCESSSTATUS_EXITED;
#else
    int status;
    pid_t result = waitpid(process->ProcessId, &status, WNOHANG);

    if (result == 0)
        return PROCESSSTATUS_RUNNING;

    if (result == -1)
        return PROCESSSTATUS_FAILED;

    if (WIFEXITED(status))
    {
        process->ExitCode = WEXITSTATUS(status);
        return PROCESSSTATUS_EXITED;
    }

    return PROCESSSTATUS_FAILED;
#endif
}
RESULT ProcessWait(PPROCESS process, I32 *exit_code)
{
    if (!process) return INVALID_PARAMETER;

#ifdef _WIN32
    if (WaitForSingleObject(process->ProcessHandle, INFINITE) != WAIT_OBJECT_0)
        return UNDEFINED_ERROR;

    DWORD code;
    if (!GetExitCodeProcess(process->ProcessHandle, &code))
        return UNDEFINED_ERROR;
    process->ExitCode = (I32)code;
    if (exit_code) *exit_code = process->ExitCode;
#else
    int status;
    if (waitpid(process->ProcessId, &status, 0) == -1)
        return UNDEFINED_ERROR;

    if (WIFEXITED(status))
    {
        process->ExitCode = WEXITSTATUS(status);
        if (exit_code) *exit_code = process->ExitCode;
    }
    else
    {
        return UNDEFINED_ERROR;
    }
#endif
    return OK;
}
RESULT ProcessKill(PPROCESS process)
{
    if (!process) return INVALID_PARAMETER;

#ifdef _WIN32
    if (!TerminateProcess(process->ProcessHandle, 1))
        return UNDEFINED_ERROR;
#else
    if (kill(process->ProcessId, SIGKILL) == -1)
        return UNDEFINED_ERROR;
#endif
    return OK;
}
VOID FreeProcess(PPROCESS process)
{
    if (!process) return;
    PROCESSSTATUS status = ProcessGetStatus(process);
    if (status == PROCESSSTATUS_RUNNING)
        ProcessWait(process, null);
#ifdef _WIN32
    if (process->ReadOut) CloseHandle(process->ReadOut);
    if (process->WriteIn) CloseHandle(process->WriteIn);
    if (process->ProcessHandle) CloseHandle(process->ProcessHandle);
    if (process->ThreadHandle) CloseHandle(process->ThreadHandle);
#else
    if (process->ReadOut) close((int)(intptr_t)process->ReadOut);
    if (process->WriteIn) close((int)(intptr_t)process->WriteIn);
#endif
    Free(&process->Allocator, process);
}


