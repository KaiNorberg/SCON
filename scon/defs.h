#ifndef SCON_DEFS_H
#define SCON_DEFS_H 1

#ifdef SCON_INLINE
#define SCON_API static inline
#else
#define SCON_API
#endif

#ifndef SCON_FREESTANDING
#include <assert.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCON_NULL NULL

#define SCON_ASSERT(_cond) assert(_cond)

#define SCON_MALLOC(_size) malloc(_size)
#define SCON_CALLOC(_nmemb, _size) calloc(_nmemb, _size)
#define SCON_REALLOC(_ptr, _size) realloc(_ptr, _size)
#define SCON_FREE(_ptr) free(_ptr)

#define SCON_MEMSET(_ptr, _val, _size) memset(_ptr, _val, _size)
#define SCON_MEMCPY(_dest, _src, _size) memcpy(_dest, _src, _size)
#define SCON_MEMCMP(_s1, _s2, _size) memcmp(_s1, _s2, _size)

#define SCON_STRNCPY(dest, src, n) strncpy(dest, src, n)
#define SCON_STRCMP(s1, s2) strcmp(s1, s2)
#define SCON_STRLEN(s) strlen(s)

typedef jmp_buf scon_jmp_buf_t;
#define SCON_SETJMP(_env) setjmp(_env)
#define SCON_LONGJMP(_env, _val) longjmp(_env, _val)

typedef FILE* scon_file_t;
#define SCON_FOPEN(_path, _mode) fopen(_path, _mode)
#define SCON_FCLOSE(_file) fclose(_file)
#define SCON_FREAD(_ptr, _size, _nmemb, _file) fread(_ptr, _size, _nmemb, _file)
#define SCON_FWRITE(_ptr, _size, _nmemb, _file) fwrite(_ptr, _size, _nmemb, _file)
#define SCON_FPRINTF fprintf
#define SCON_SNPRINTF snprintf
#define SCON_VSNPRINTF vsnprintf
#define SCON_STDIN stdin
#define SCON_STDOUT stdout
#define SCON_STDERR stderr

#define SCON_VA_START(_ap, _last) va_start(_ap, _last)
#define SCON_VA_END(_ap) va_end(_ap)
#define SCON_VA_COPY(_dest, _src) va_copy(_dest, _src)
typedef va_list scon_va_list;

typedef int64_t scon_int64_t;
typedef uint64_t scon_uint64_t;
typedef int32_t scon_int32_t;
typedef uint32_t scon_uint32_t;
typedef int16_t scon_int16_t;
typedef uint16_t scon_uint16_t;
typedef int8_t scon_int8_t;
typedef uint8_t scon_uint8_t;
typedef size_t scon_size_t;
typedef double scon_float_t;
#endif

#if defined(__GNUC__) || defined(__clang__)
#define SCON_LIKELY(_x) __builtin_expect(!!(_x), 1)
#define SCON_UNLIKELY(_x) __builtin_expect(!!(_x), 0)
#define SCON_NORETURN __attribute__((noreturn))
#define SCON_ALWAYS_INLINE __attribute__((always_inline))
#define SCON_HAS_COMPUTED_GOTO
#elif defined(_MSC_VER)
#define SCON_LIKELY(_x) (_x)
#define SCON_UNLIKELY(_x) (_x)
#define SCON_NORETURN __declspec(noreturn)
#define SCON_ALWAYS_INLINE __forceinline
#else
#define SCON_LIKELY(_x) (_x)
#define SCON_UNLIKELY(_x) (_x)
#define SCON_NORETURN
#define SCON_ALWAYS_INLINE
#endif

#define SCON_MIN(_a, _b) ((_a) < (_b) ? (_a) : (_b))
#define SCON_MAX(_a, _b) ((_a) > (_b) ? (_a) : (_b))

/**
 * @brief SCON boolean type.
 * @enum scon_bool_t
 *
 * Equivalent to bool.
 */
typedef enum
{
    SCON_TRUE = 1,
    SCON_FALSE = 0
} scon_bool_t;

/**
 * @brief SCON PI constant.
 */
#define SCON_PI 3.14159265358979323846

/**
 * @brief SCON E constant.
 */
#define SCON_E 2.7182818284590452354

/**
 * @brief SCON INFINITY constant.
 */
#define SCON_INF \
    (((union { \
        scon_uint64_t u; \
        scon_float_t f; \
    }){0x7FF0000000000000ULL}) \
            .f)

/**
 * @brief SCON NAN constant.
 */
#define SCON_NAN \
    (((union { \
        scon_uint64_t u; \
        scon_float_t f; \
    }){0x7FF8000000000000ULL}) \
            .f)

/**
 * @brief Maximum path length for SCON.
 */
#define SCON_PATH_MAX 512

/**
 * @brief SCON container of macro.
 *
 * Used to get the pointer to a structure from a pointer to one of its members.
 *
 * @param _ptr The pointer to the member.
 * @param _type The type of the structure.
 * @param _member The name of the member.
 */
#define SCON_CONTAINER_OF(_ptr, _type, _member) ((_type*)((char*)(_ptr) - (scon_size_t) & ((_type*)0)->_member))

/**
 * @brief SCON handle type.
 */
typedef scon_uint64_t scon_handle_t;

#endif
