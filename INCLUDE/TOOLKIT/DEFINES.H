#pragma once

////////////////////////////////////////
// VALUES                             //
////////////////////////////////////////
#define CACHELINESIZE      64

////////////////////////////////////////
// ATTRIBUTES                         //
////////////////////////////////////////
#define ALIGNCACHE //alignas(CACHELINESIZE) 
#define NOALIGN 
#define _T(type) // Indicates the result the following operation yields.
#define __ENTRY__

////////////////////////////////////////
// HELPER FUNCTIONS                   //
////////////////////////////////////////
// / / / / / / / / / / / / / / / / / / / 
// ALIGNOF                             /
// / / / / / / / / / / / / / / / / / / /
#ifdef __cplusplus
    //#if __cplusplus >= 201103L // C++11 or later
    //#else // < C++11
    //    #if defined(__GNUC__) || defined(__clang__)
    //        #define alignof(type) __alignof__(type)
    //    #elif defined(_MSC_VER)
    //        #define alignof(type) __alignof(type)
    //    #elif defined(__INTEL_COMPILER)
    //        #define alignof(type) __alignof__(type)
    //    #else // Fallback
    //        #include <stddef.h>
    //        #define alignof(type) offsetof(struct { char c; type member; }, member)
    //    #endif
    //#endif
#else // C
    #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L // C11 or later
        #ifndef alignof
            #define alignof(type) _Alignof(type)
        #endif
    #else // Pre-C11
        #if defined(__GNUC__) || defined(__clang__)
            #define alignof(type) __alignof__(type)
        #elif defined(_MSC_VER)
            #define alignof(type) __alignof(type)
        #elif defined(__INTEL_COMPILER)
            #define alignof(type) __alignof__(type)
        #else // Fallback
            #include <stddef.h>
            #define alignof(type) offsetof(struct { char c; type member; }, member)
        #endif
    #endif
#endif

// / / / / / / / / / / / / / / / / / / / 
// ALIGNAS                             /
// / / / / / / / / / / / / / / / / / / /
#ifdef __cplusplus
#else // C
    #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L // C11 or later
        #ifndef alignas
            #define alignas(alignment) _Alignas(alignment)
        #endif
    #else // Pre-C11
        #if defined(__GNUC__) || defined(__clang__)
            #define alignas(alignment) __attribute__((aligned(alignment)))
        #elif defined(_MSC_VER)
            #define alignas(alignment) __declspec(align(alignment))
        #elif defined(__INTEL_COMPILER)
            #define alignas(alignment) __attribute__((aligned(alignment)))
        #else // Fallback
            #error "Your C compiler does not support alignas extensions."
        #endif
    #endif
#endif

// / / / / / / / / / / / / / / / / / / / 
// ALIGNSTRING                         /
// / / / / / / / / / / / / / / / / / / /
#define ALIGNSTRING(S, A)  (struct { alignas(A) char s[sizeof S]; }){ S }.s // https://stackoverflow.com/a/34797534/25506417

// / / / / / / / / / / / / / / / / / / / 
// TYPEOF                              /
// / / / / / / / / / / / / / / / / / / /
#if defined(__cplusplus)
    #if __cplusplus >= 201103L
        #define typeof(expr) decltype(expr)
    #else // GCC / Clang for C++03
        #if defined(__GNUC__) || defined(__clang__)
            #define typeof(expr) __typeof__(expr)
        #elif defined(_MSC_VER)
            #define typeof(expr) __typeof(expr)
        #endif
    #endif
#else // C
    #ifndef typeof
        #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
            // C23
        #elif defined(__GNUC__) || defined(__clang__) // GCC / Clang 
            #define typeof(expr) __typeof__(expr)
        #elif defined(_MSC_VER) // MSVC
            #define typeof(expr) __typeof(expr)
        #else
            #error "Your C compiler does not support typeof extensions."
        #endif
    #endif
#endif
// / / / / / / / / / / / / / / / / / / / 
// DISCARD                             /
//                                     /
// _ = notusedparameter;               /
// / / / / / / / / / / / / / / / / / / /
#define CONCAT_INNER(a, b) a ## b
#define CONCAT(a, b) CONCAT_INNER(a, b)
#if defined(__cplusplus)
    #define _ [[maybe_unused]] auto CONCAT(unused_var_, __COUNTER__)
#else // C
    #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L // C23
        #define _ [[maybe_unused]] auto CONCAT(unused_var_, __COUNTER__)
    #elif defined(_MSC_VER) && !defined(__clang__) // MSVC 
        #define _ __pragma(warning(suppress: 4101 4189, 4047, 4048)) auto CONCAT(unused_var_, __COUNTER__)
    #else // GCC / CLANG
        #define _ __attribute__((unused)) __typeof__(auto) CONCAT(unused_var_, __COUNTER__)
    #endif
#endif 

#define memzero(p) memset(p, 0, sizeof(*p))
#define alignup(x, type) ((((uintptr_t)x) + (alignof(x) - 1)) & ~(alignof(x) - 1))
#define aligndown(x, type) (((uintptr_t)x) & ~(alignof(x) - 1))
#define makeopaque(TYPE) typedef struct TYPE TYPE, *P##TYPE, **PP##TYPE
#define arrsize(x) (sizeof(x) / sizeof((x)[0]))
#define stringify(x) #x
#define rounddiv(x, divisor) (((x) + ((divisor) >> 1)) / (divisor))

#define TOSTRING(x) stringify(x)
#define LINE_STRING TOSTRING(__LINE__)
#define EXPAND(x) x

#define ALIGNUP(size, alignment) (((size) + (alignment) - 1) & ~((alignment) - 1))

////////////////////////////////////////
// PACKED BUNDLE                      //
//                                    //
// U64 size = 0;                      //
// I32 *a = packadd(size, I32, 10);   //
// F32 *b = packadd(size, F32, 5);    //
// PVOID bundle = malloc(size);       //
// packfix(bundle, a);                //
// packfix(bundle, b);                //
// // Use a and b...                  //
// free(bundle);                      //
////////////////////////////////////////
#define packadd(bundle_size, type, count) \
    (type*)(uintptr_t)( \
        bundle_size = alignup(bundle_size, type), \
        (bundle_size += (count) * sizeof(type)) - ((count) * sizeof(type)))
#define packfix(bundle, slab) slab = (typeof(slab))((uintptr_t)slab + (uintptr_t)bundle)

////////////////////////////////////////
// BREAKPOINT (Source: SDL3)          //
////////////////////////////////////////
#if defined(__MINGW32__) || (defined(_MSC_VER) && _MSC_VER >= 1310)
/* Don't include intrin.h here because it contains C++ code */
extern void __cdecl __debugbreak(void);
#define breakpoint() __debugbreak()
#elif defined(_MSC_VER) && defined(_M_IX86)
#define breakpoint() { _asm { int 0x03 }  }
#elif __has_builtin(__builtin_debugtrap)
#define breakpoint() __builtin_debugtrap()
#elif __has_builtin(__builtin_trap)
#define breakpoint() __builtin_trap()
#elif (defined(__GNUC__) || defined(__clang__) || defined(__TINYC__) || defined(__slimcc__)) && (defined(__i386__) || defined(__x86_64__))
#define breakpoint() __asm__ __volatile__ ( "int $3\n\t" )
#elif (defined(__GNUC__) || defined(__clang__)) && defined(__riscv)
#define breakpoint() __asm__ __volatile__ ( "ebreak\n\t" )
#elif ( defined(__APPLE__) && (defined(__arm64__) || defined(__aarch64__)) )  /* this might work on other ARM targets, but this is a known quantity... */
#define breakpoint() __asm__ __volatile__ ( "brk #22\n\t" )
#elif defined(__APPLE__) && defined(__arm__)
#define breakpoint() __asm__ __volatile__ ( "bkpt #22\n\t" )
#elif defined(_WIN32) && ((defined(__GNUC__) || defined(__clang__)) && (defined(__arm64__) || defined(__aarch64__)) )
#define breakpoint() __asm__ __volatile__ ( "brk #0xF000\n\t" )
#elif defined(__GNUC__) || defined(__clang__)
#define breakpoint() __builtin_trap()  
#elif defined(__386__) && defined(__WATCOMC__)
#define breakpoint() { _asm { int 0x03 } }
#elif defined(HAVE_SIGNAL_H) && !defined(__WATCOMC__)
#include <signal.h>
#define breakpoint() raise(SIGTRAP)
#else
/* breakpoint is intentionally left undefined on unknown platforms. */
#endif

////////////////////////////////////////
// DEFER AND ERROR DEFER              //
////////////////////////////////////////
makeopaque(DEFERCONTEXT);
struct DEFERCONTEXT
{
    void(*fn)(void *, void *, void *);
    void *a0, *a1, *a2;
};

static inline void DfThunk0(void *f, void *_1, void *_2)
{
    ((void(*)(void))f)();
    _ = _1;
    _ = _2;
}
static inline void DfThunk1(void *f, void *a, void *_1)
{
    ((void(*)(void *))f)(a);
    _ = _1;
}
static inline void DfThunk2(void *f, void *a, void *b)
{
    ((void(*)(void *, void *))f)(a, b);
}
static inline void DfP0(PDEFERCONTEXT d, void(*f)(void))
{
    d->fn = DfThunk0; d->a0 = (void *)f;
}
static inline void DfP1(PDEFERCONTEXT d, void(*f)(void *), void *a)
{
    d->fn = DfThunk1; d->a0 = (void *)f; d->a1 = a;
}
static inline void DfP2(PDEFERCONTEXT d, void(*f)(void *, void *), void *a, void *b)
{
    d->fn = DfThunk2; d->a0 = (void *)f; d->a1 = a; d->a2 = b;
}
static inline void DfP3(PDEFERCONTEXT d, void(*f)(void *, void *), void *a, void *b, void *c)
{
    d->fn = (void(*)(void *, void *, void *))f; d->a0 = a; d->a1 = b; d->a2 = c;
}
static inline int DfFlush(PPDEFERCONTEXT ptr, PDEFERCONTEXT end)
{
    while (*ptr < end)
    {
        PDEFERCONTEXT d = (*ptr)++;
        d->fn(d->a0, d->a1, d->a2);
    }
    return 0;
}
// C99 Macro Dispatcher
#define DFP0_CAST(d, f)          DfP0(d, (VOID(*)(VOID))f)
#define DFP1_CAST(d, f, a)       DfP1(d, (VOID(*)(PVOID))f, (PVOID)a)
#define DFP2_CAST(d, f, a, b)    DfP2(d, (VOID(*)(PVOID, PVOID))f, (PVOID)a, (PVOID)b)
#define DFP3_CAST(d, f, a, b, c) DfP3(d, (VOID(*)(PVOID, PVOID, PVOID))f, (PVOID)a, (PVOID)b, (PVOID)c)
#define DF_EXEC(d) (d)->fn((d)->a0, (d)->a1, (d)->a2)
#define DF_GET_MACRO(_0, _1, _2, _3, NAME, ...) NAME
#define DF_CHOOSE(...) EXPAND(DF_GET_MACRO(__VA_ARGS__, DFP3_CAST, DFP2_CAST, DFP1_CAST, DFP0_CAST))

#define dfs(n) struct { PDEFERCONTEXT ptr; DEFERCONTEXT items[n]; } _deferred = { &_deferred.items[n] }
#define df(fn, ...) do { \
    assert(_deferred.ptr > _deferred.items and "defer buffer overflowed!"); \
    --_deferred.ptr; \
    DF_CHOOSE(fn, __VA_ARGS__)(_deferred.ptr, fn, __VA_ARGS__); \
} while(0)
#define dfflush() DfFlush(&_deferred.ptr, &_deferred.items[arrsize(_deferred.items)])

#define errdfs(n) struct { PDEFERCONTEXT ptr; DEFERCONTEXT items[n]; } _errdeferred = { &_errdeferred.items[n] }
#define errdf(fn, ...) do { \
    assert(_errdeferred.ptr > _errdeferred.items and "defer buffer overflowed!"); \
    --_errdeferred.ptr; \
    DF_CHOOSE(fn, __VA_ARGS__)(_errdeferred.ptr, fn, __VA_ARGS__); \
} while(0)
#define errdfflush() DfFlush(&_errdeferred.ptr, &_errdeferred.items[arrsize(_errdeferred.items)])


////////////////////////////////////////
// UNREACHABLE                        //
////////////////////////////////////////
#if defined(__GNUC__) || defined(__clang__) // GCC, Clang, ICC
    #define unreachable() (__builtin_unreachable())
#elif defined(_MSC_VER) // MSVC
    #define unreachable() (__assume(false))
#else
    [[noreturn]] inline void unreachable_impl() {} // Raise undefined behavior by noreturn.
    #define unreachable() (unreachable_impl())
#endif
