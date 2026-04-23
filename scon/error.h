#ifndef SCON_ERROR_H
#define SCON_ERROR_H 1

#include "defs.h"

struct scon;
struct scon_item;

/**
 * @brief SCON error handling and reporting.
 * @defgroup error Error
 * @file error.h
 *
 * @{
 */

#define SCON_ERROR_MAX_LEN 512 ///< Maximum length of an error string.

/**
 * @brief SCON error type enumeration.
 * @enum scon_error_type_t
 */
typedef enum scon_error_type
{
    SCON_ERROR_TYPE_NONE,
    SCON_ERROR_TYPE_SYNTAX,
    SCON_ERROR_TYPE_COMPILE,
    SCON_ERROR_TYPE_RUNTIME,
    SCON_ERROR_TYPE_INTERNAL,
} scon_error_type_t;

/**
 * @brief SCON error structure.
 * @struct scon_error_t
 */
typedef struct
{
    const char* input;        ///< The input buffer.
    scon_size_t inputLength;  ///< The total length of the input buffer.
    const char* path;         ///< THe path to the file that caused the error.
    scon_size_t regionLength; ///< The length of the region that caused the error.
    scon_size_t index;        ///< The index of the region in the input buffer that caused the error.
    scon_jmp_buf_t jmp;
    char message[SCON_ERROR_MAX_LEN];
} scon_error_t;

/**
 * @brief Create a SCON error structure.
 *
 * @return A new SCON error structure initialized to zero.
 */
#define SCON_ERROR() ((scon_error_t){0})

/**
 * @brief Format and print the error to a file.
 *
 * @param error Pointer to the error structure.
 * @param file The file to print to.
 */
SCON_API void scon_error_print(scon_error_t* error, scon_file_t file);

/**
 * @brief Get the row and column by traversing the input buffer.
 *
 * @param error Pointer to the error structure.
 * @param row Pointer to the row variable.
 * @param column Pointer to the column variable.
 */
SCON_API void scon_error_get_row_column(scon_error_t* error, scon_size_t* row, scon_size_t* column);

/**
 * @brief Set the error information in the error structure.
 *
 * @param error Pointer to the error structure.
 * @param path The path to the file where the error occurred.
 * @param input The input buffer where the error occurred.
 * @param inputLength The total length of the input buffer.
 * @param regionLength The length of the token/region that caused the error.
 * @param position The position in the input buffer where the error occurred.
 * @param type The type of the error.
 * @param message The error message format string.
 * @param ... The arguments for the format string.
 */
SCON_API void scon_error_set(scon_error_t* error, const char* path, const char* input, scon_size_t inputLength,
    scon_size_t regionLength, scon_size_t position, scon_error_type_t type, const char* message, ...);

/**
 * @brief Get the error parameters from a SCON item.
 *
 * @param item Pointer to the item.
 * @param path Pointer to the path variable.
 * @param input Pointer to the input variable.
 * @param inputLength Pointer to the input length variable.
 * @param regionLength Pointer to the region length variable.
 * @param position Pointer to the position variable.
 */
SCON_API void scon_error_get_item_params(struct scon_item* item, const char** path, const char** input,
    scon_size_t* inputLength, scon_size_t* regionLength, scon_size_t* position);

/**
 * @brief Throw a runtime error utilizing the evaluation state to determine the context.
 *
 * @param scon Pointer to the SCON instance.
 * @param message The error message format string.
 * @param ... Additional arguments.
 */
SCON_API SCON_NORETURN void scon_error_throw_runtime(struct scon* scon, const char* message, ...);

/**
 * @brief Catch an error using the jump buffer in the error structure.
 *
 * @param _error Pointer to the error structure.
 */
#define SCON_ERROR_CATCH(_error) (SCON_SETJMP((_error)->jmp))

/**
 * @brief Throw an error using the jump buffer in the error structure.
 *
 * @param _error Pointer to the error structure.
 * @param _item Pointer to the item that caused the error.
 * @param _type The suffix of the error type (e.g., INTERNAL, RUNTIME, etc.).
 * @param ... The error message format string and any optional arguments.
 */
#define SCON_ERROR_THROW(_error, _item, _type, ...) \
    do \
    { \
        const char* __path; \
        const char* __input; \
        scon_size_t __input_length; \
        scon_size_t __region_length; \
        scon_size_t __position; \
        scon_error_get_item_params((_item), &__path, &__input, &__input_length, &__region_length, &__position); \
        scon_error_set((_error), __path, __input, __input_length, __region_length, __position, \
            SCON_ERROR_TYPE_##_type, __VA_ARGS__); \
        SCON_LONGJMP((_error)->jmp, SCON_TRUE); \
    } while (0)

/**
 * @brief Throw a syntax error using the jump buffer in the error structure.
 *
 * @param _error Pointer to the error structure.
 * @param _input Pointer to the input structure being parsed.
 * @param _ptr Pointer to the current position in the input buffer.
 * @param ... The error message format string and any optional arguments.
 */
#define SCON_ERROR_SYNTAX(_error, _input, _ptr, ...) \
    do \
    { \
        scon_error_set((_error), (_input)->path, (_input)->buffer, (_input)->end - (_input)->buffer, 1, \
            (scon_size_t)((_ptr) - (_input)->buffer), SCON_ERROR_TYPE_SYNTAX, __VA_ARGS__); \
        SCON_LONGJMP((_error)->jmp, SCON_TRUE); \
    } while (0)

/**
 * @brief Throw a compile error using the jump buffer in the error structure.
 *
 * @param _compiler The compiler instance.
 * @param _item Pointer to the item that caused the error.
 * @param ... The error message format string and any optional arguments.
 */
#define SCON_ERROR_COMPILE(_compiler, _item, ...) \
    SCON_ERROR_THROW((_compiler)->scon->error, \
        (((_item) != SCON_NULL && (_item)->input != SCON_NULL) \
                ? (_item) \
                : ((_compiler)->lastItem != SCON_NULL ? (_compiler)->lastItem : (_item))), \
        COMPILE, __VA_ARGS__)

/**
 * @brief Throw a runtime error using the jump buffer in the error structure.
 *
 * @param _scon Pointer to the scon instance.
 * @param ... The error message format string and any optional arguments.
 */
#define SCON_ERROR_RUNTIME(_scon, ...) scon_error_throw_runtime((_scon), __VA_ARGS__)

/**
 * @brief Throw an internal error using the jump buffer in the error structure.
 *
 * @param _scon Pointer to the scon instance.
 * @param ... The error message format string and any optional arguments.
 */
#define SCON_ERROR_INTERNAL(_scon, ...) SCON_ERROR_THROW((_scon)->error, SCON_NULL, INTERNAL, __VA_ARGS__)

/** @} */

#endif
