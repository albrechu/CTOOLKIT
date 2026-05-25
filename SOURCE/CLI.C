#include <TOOLKIT/CLI.H>
#include <TOOLKIT/FILESYSTEM.H>

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

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
    if (progress >= 100)
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
        if (end == string || *end != '\0' || errno == ERANGE)
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
        if (end == string || *end != '\0' || errno == ERANGE)
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
        if (end == string || *end != '\0' || errno == ERANGE)
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
