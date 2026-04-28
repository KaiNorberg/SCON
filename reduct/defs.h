#ifndef REDUCT_DEFS_H
#define REDUCT_DEFS_H 1

#ifdef REDUCT_INLINE
#define REDUCT_API static inline
#else
#define REDUCT_API
#endif

#ifndef REDUCT_FREESTANDING
#define _CRT_SECURE_NO_WARNINGS // Make windows stop being annoying
#include <assert.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define REDUCT_NULL NULL

#define REDUCT_ASSERT(_cond) assert(_cond)

#define REDUCT_MALLOC(_size) malloc(_size)
#define REDUCT_CALLOC(_nmemb, _size) calloc(_nmemb, _size)
#define REDUCT_REALLOC(_ptr, _size) realloc(_ptr, _size)
#define REDUCT_FREE(_ptr) free(_ptr)

#define REDUCT_MEMSET(_ptr, _val, _size) memset(_ptr, _val, _size)
#define REDUCT_MEMCPY(_dest, _src, _size) memcpy(_dest, _src, _size)
#define REDUCT_MEMCMP(_s1, _s2, _size) memcmp(_s1, _s2, _size)

#define REDUCT_STRNCPY(dest, src, n) strncpy(dest, src, n)
#define REDUCT_STRCMP(s1, s2) strcmp(s1, s2)
#define REDUCT_STRLEN(s) strlen(s)

typedef jmp_buf reduct_jmp_buf_t;
#define REDUCT_SETJMP(_env) setjmp(_env)
#define REDUCT_LONGJMP(_env, _val) longjmp(_env, _val)

typedef FILE* reduct_file_t;
#define REDUCT_FOPEN(_path, _mode) fopen(_path, _mode)
#define REDUCT_FCLOSE(_file) fclose(_file)
#define REDUCT_FREAD(_ptr, _size, _nmemb, _file) fread(_ptr, _size, _nmemb, _file)
#define REDUCT_FWRITE(_ptr, _size, _nmemb, _file) fwrite(_ptr, _size, _nmemb, _file)
#define REDUCT_FGETC(_file) fgetc(_file)
#define REDUCT_FPRINTF fprintf
#define REDUCT_SNPRINTF snprintf
#define REDUCT_VSNPRINTF vsnprintf
#define REDUCT_STDIN stdin
#define REDUCT_STDOUT stdout
#define REDUCT_STDERR stderr

#define REDUCT_TIME() time(REDUCT_NULL)
#define REDUCT_CLOCK() clock()
#define REDUCT_GETENV(_name) getenv(_name)

#define REDUCT_FLOOR(_x) floor(_x)
#define REDUCT_CEIL(_x) ceil(_x)
#define REDUCT_ROUND(_x) round(_x)
#define REDUCT_POW(_x, _y) pow(_x, _y)
#define REDUCT_EXP(_x) exp(_x)
#define REDUCT_LOG(_x) log(_x)
#define REDUCT_SQRT(_x) sqrt(_x)
#define REDUCT_SIN(_x) sin(_x)
#define REDUCT_COS(_x) cos(_x)
#define REDUCT_TAN(_x) tan(_x)
#define REDUCT_ASIN(_x) asin(_x)
#define REDUCT_ACOS(_x) acos(_x)
#define REDUCT_ATAN(_x) atan(_x)
#define REDUCT_ATAN2(_y, _x) atan2(_y, _x)
#define REDUCT_SINH(_x) sinh(_x)
#define REDUCT_COSH(_x) cosh(_x)
#define REDUCT_TANH(_x) tanh(_x)
#define REDUCT_ASINH(_x) asinh(_x)
#define REDUCT_ACOSH(_x) acosh(_x)
#define REDUCT_ATANH(_x) atanh(_x)
#define REDUCT_FABS(_x) fabs(_x)

#define REDUCT_RAND() rand()
#define REDUCT_SRAND(_seed) srand(_seed)
#define REDUCT_RAND_MAX RAND_MAX

#define REDUCT_VA_START(_ap, _last) va_start(_ap, _last)
#define REDUCT_VA_END(_ap) va_end(_ap)
#define REDUCT_VA_COPY(_dest, _src) va_copy(_dest, _src)
typedef va_list reduct_va_list;

typedef int64_t reduct_int64_t;
typedef uint64_t reduct_uint64_t;
typedef int32_t reduct_int32_t;
typedef uint32_t reduct_uint32_t;
typedef int16_t reduct_int16_t;
typedef uint16_t reduct_uint16_t;
typedef int8_t reduct_int8_t;
typedef uint8_t reduct_uint8_t;
typedef size_t reduct_size_t;
typedef int reduct_int_t;
typedef double reduct_float_t;
#endif

#if defined(__GNUC__) || defined(__clang__)
#define REDUCT_LIKELY(_x) __builtin_expect(!!(_x), 1)
#define REDUCT_UNLIKELY(_x) __builtin_expect(!!(_x), 0)
#define REDUCT_NORETURN __attribute__((noreturn))
#define REDUCT_ALWAYS_INLINE __attribute__((always_inline))
#define REDUCT_HAS_COMPUTED_GOTO
#elif defined(_MSC_VER)
#define REDUCT_LIKELY(_x) (_x)
#define REDUCT_UNLIKELY(_x) (_x)
#define REDUCT_NORETURN __declspec(noreturn)
#define REDUCT_ALWAYS_INLINE __forceinline
#else
#define REDUCT_LIKELY(_x) (_x)
#define REDUCT_UNLIKELY(_x) (_x)
#define REDUCT_NORETURN
#define REDUCT_ALWAYS_INLINE
#endif

#define REDUCT_MIN(_a, _b) ((_a) < (_b) ? (_a) : (_b))
#define REDUCT_MAX(_a, _b) ((_a) > (_b) ? (_a) : (_b))

#define REDUCT_UNUSED(_x) ((void)(_x))

/**
 * @brief Boolean type.
 * @enum reduct_bool_t
 *
 * Equivalent to bool.
 */
typedef enum
{
    REDUCT_TRUE = 1,
    REDUCT_FALSE = 0
} reduct_bool_t;

/**
 * @brief PI constant.
 */
#define REDUCT_PI 3.14159265358979323846

/**
 * @brief E constant.
 */
#define REDUCT_E 2.7182818284590452354

/**
 * @brief INFINITY constant.
 */
#define REDUCT_INF \
    (((union { \
        reduct_uint64_t u; \
        reduct_float_t f; \
    }){0x7FF0000000000000ULL}) \
            .f)

/**
 * @brief NAN constant.
 */
#define REDUCT_NAN \
    (((union { \
        reduct_uint64_t u; \
        reduct_float_t f; \
    }){0x7FF8000000000000ULL}) \
            .f)

/**
 * @brief Maximum path length for Reduct.
 */
#define REDUCT_PATH_MAX 512

/**
 * @brief Container of macro.
 *
 * Used to get the pointer to a structure from a pointer to one of its members.
 *
 * @param _ptr The pointer to the member.
 * @param _type The type of the structure.
 * @param _member The name of the member.
 */
#define REDUCT_CONTAINER_OF(_ptr, _type, _member) ((_type*)((char*)(_ptr) - offsetof(_type, _member)))

/**
 * @brief Handle type.
 */
typedef reduct_uint64_t reduct_handle_t;

#define REDUCT_STACK_BUFFER_SIZE 256 ///< The size of temporary stack allocated buffers.

/**
 * @brief Scratch buffer macro.
 *
 * Will allocate a buffer on the stack if its small enough, otherwise it will create a heap allocated buffer.
 *
 * @param _name The name of the buffer pointer.
 * @param _size The size of the buffer to allocate.
 */
#define REDUCT_SCRATCH_BUFFER(_name, _size) \
    char _name##Stack[REDUCT_STACK_BUFFER_SIZE]; \
    char* _name = ((reduct_size_t)(_size) + 1 <= REDUCT_STACK_BUFFER_SIZE) \
        ? _name##Stack \
        : (char*)REDUCT_MALLOC((reduct_size_t)(_size) + 1); \
    if (_name == REDUCT_NULL) \
    { \
        REDUCT_ERROR_INTERNAL(reduct, "out of memory"); \
    }

/**
 * @brief Scratch buffer free macro.
 * 
 * Frees a scratch buffer if it was heap allocated.
 *
 * @param _name The name of the buffer pointer.
 */
#define REDUCT_SCRATCH_BUFFER_FREE(_name) \
    if (_name != _name##Stack) \
    { \
        REDUCT_FREE(_name); \
    }

#endif
