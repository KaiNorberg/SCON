#ifndef SCON_CORE_API_H
#define SCON_CORE_API_H 1

#ifdef SCON_INLINE
#define SCON_API static inline
#else
#define SCON_API
#endif

#ifndef SCON_FREESTANDING
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#define SCON_NULL NULL

#define SCON_MALLOC(_size) malloc(_size)
#define SCON_CALLOC(_nmemb, _size) calloc(_nmemb, _size)
#define SCON_REALLOC(_ptr, _size) realloc(_ptr, _size)
#define SCON_FREE(_ptr) free(_ptr)

#define SCON_MEMSET(_ptr, _val, _size) memset(_ptr, _val, _size)
#define SCON_MEMCPY(_dest, _src, _size) memcpy(_dest, _src, _size)

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

typedef int64_t scon_int64_t;
typedef uint64_t scon_uint64_t;
typedef int32_t scon_int32_t;
typedef uint32_t scon_uint32_t;
typedef int16_t scon_int16_t;
typedef uint16_t scon_uint16_t;
typedef size_t scon_size_t;
typedef double scon_float_t;
#endif

#if !defined(SCON_USE_NAN_BOXING) && !defined(SCON_USE_TAGGED_UNION)
#if UINTPTR_MAX == 0xffffffffffffffff
#define SCON_USE_NAN_BOXING 1
#else
#define SCON_USE_TAGGED_UNION 1
#endif
#endif

#if defined(__GNUC__) || defined(__clang__)
#define SCON_NORETURN __attribute__((noreturn))
#elif defined(_MSC_VER)
#define SCON_NORETURN __declspec(noreturn)
#else
#define SCON_NORETURN
#endif

/**
 * @brief SCON state structure.
 * @struct scon_t
 */
typedef struct scon scon_t;

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
#define SCON_CONTAINER_OF(_ptr, _type, _member) ((_type*)((char*)(_ptr) - (scon_size_t)&((_type*)0)->_member))

/**
 * @brief Create a new SCON structure.
 *
 * @param cb Pointer to a callbacks structure.
 * @return A pointer to the newly allocated SCON structure or `SCON_NULL` if the allocation failed.
 */
SCON_API scon_t* scon_new(void);

/**
 * @brief Free a the SCON structure.
 *
 * @note Will not free the input buffer.
 *
 * @param scon Pointer to the SCON structure to free.
 */
SCON_API void scon_free(scon_t* scon);

/**
 * @brief Set the error message for a SCON structure.
 *
 * @param scon Pointer to the SCON structure.
 * @param message The error message to set.
 * @param input The input buffer where the error occurred.
 * @param length The length of the input buffer.
 * @param position The position in the input buffer where the error occurred.
 */
SCON_API void scon_error_set(scon_t* scon, const char* message, const char* input, scon_size_t length, scon_size_t position);

/**
 * @brief Get the last error message from a SCON structure.
 *
 * @param scon Pointer to the SCON structure.
 * @return A null-terminated string containing the error message.
 */
SCON_API const char* scon_error_get(scon_t* scon);

/**
 * @brief Get the jump buffer for a SCON structure.
 *
 * @param scon Pointer to the SCON structure.
 * @return A pointer to the jump buffer.
 */
SCON_API scon_jmp_buf_t* scon_jmp_buf_get(scon_t* scon);

/**
 * @brief Get the current input buffer.
 *
 * @param scon Pointer to the SCON structure.
 * @return A pointer to the current input buffer.
 */
SCON_API const char* scon_input_get(scon_t* scon);

/**
 * @brief Get the length of the current input buffer.
 *
 * @param scon Pointer to the SCON structure.
 * @return The length of the current input buffer.
 */
SCON_API scon_size_t scon_input_get_length(scon_t* scon);

/**
 * @brief Get the path of the current input buffer.
 *
 * @param scon Pointer to the SCON structure.
 * @return The path of the current input buffer.
 */
SCON_API const char* scon_input_get_path(scon_t* scon);

/**
 * @brief Catch an error in a SCON structure.
 *
 * @param scon Pointer to the SCON structure.
 * @return `SCON_TRUE` if an error was caught, `SCON_FALSE` otherwise.
 */
#define SCON_CATCH(_scon) SCON_SETJMP(*scon_jmp_buf_get(_scon))

/**
 * @brief Throw an error in a SCON structure.
 *
 * @param _scon Pointer to the SCON structure.
 * @param _msg The error message to set.
 */
#define SCON_THROW(_scon, _msg) \
    scon_error_set(_scon, _msg, scon_input_get(_scon), scon_input_get_length(_scon), (scon_size_t)-1);\
    SCON_LONGJMP(*scon_jmp_buf_get(_scon), SCON_TRUE)

/**
 * @brief Throw an error in a SCON structure with a specific position.
 *
 * @param _scon Pointer to the SCON structure.
 * @param _msg The error message to set.
 * @param _pos The position in the input buffer where the error occurred.
 */
#define SCON_THROW_POS(_scon, _msg, _pos) \
    scon_error_set(_scon, _msg, scon_input_get(_scon), scon_input_get_length(_scon), _pos);\
    SCON_LONGJMP(*scon_jmp_buf_get(_scon), SCON_TRUE)

#endif
