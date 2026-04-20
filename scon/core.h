#ifndef SCON_CORE_H
#define SCON_CORE_H 1

struct scon_item;

#include "atom.h"
#include "item.h"
#include "list.h"

/**
 * @brief Core SCON definitions and structures.
 * @defgroup core
 * @file core.h
 *
 * @{
 */

#define SCON_ERROR_MAX_LEN 512 ///< Maximum length of an error string.
#define SCON_BUCKETS_MAX 512   ///< Amount of buckets used for intering atoms.
#define SCON_CONSTANTS_MAX 64  ///< Maximum amount of predefined constants.

#define SCON_GC_THRESHOLD_INITIAL 1024 ///< Initial blocks allocated threshold for garbage collection.

/**
 * @brief SCON input structure.
 * @struct scon_input
 */
typedef struct scon_input
{
    struct scon_input* prev;
    const char* buffer;
    scon_size_t length;
    char path[SCON_PATH_MAX];
} scon_input_t;

/**
 * @brief SCON constant structure.
 * @struct scon_constant_t
 */
typedef struct scon_constant
{
    struct scon_atom* name;
    struct scon_item* item;
} scon_constant_t;

/**
 * @brief SCON state structure.
 * @struct scon_t
 */
typedef struct scon
{
    scon_size_t blocksAllocated;
    scon_size_t gcThreshold;
    scon_list_t retained;
    scon_item_block_t* block;
    struct scon_item* freeList;
    scon_input_t* input;
    scon_jmp_buf_t jmp;
    scon_item_block_t firstBlock;
    scon_input_t firstInput;
    scon_item_t* trueItem;
    scon_item_t* falseItem;
    scon_item_t* nilItem;
    scon_item_t* piItem;
    scon_item_t* eItem;
    struct scon_atom* atomBuckets[SCON_BUCKETS_MAX];
    scon_constant_t constants[SCON_CONSTANTS_MAX];
    scon_uint32_t constantCount;
    char error[SCON_ERROR_MAX_LEN];
} scon_t;

/**
 * @brief Create a new SCON structure.
 *
 * @param cb Pointer to a callbacks structure.
 * @return A pointer to the newly allocated SCON structure or `SCON_NULL` if the allocation failed.
 */
SCON_API scon_t* scon_new(void);

/**
 * @brief Free the SCON structure.
 *
 * @param scon Pointer to the SCON structure to free.
 */
SCON_API void scon_free(scon_t* scon);

/**
 * @brief Register a constant in a SCON structure.
 *
 * @param scon Pointer to the SCON structure.
 * @param name The name of the constant.
 * @param item The item associated with the constant.
 */
SCON_API void scon_constant_register(scon_t* scon, const char* name, struct scon_item* item);

/**
 * @brief Set the error message for a SCON structure.
 *
 * @param scon Pointer to the SCON structure.
 * @param message The error message to set.
 * @param input The input buffer where the error occurred.
 * @param length The length of the input buffer.
 * @param position The position in the input buffer where the error occurred.
 */
SCON_API void scon_error_set(scon_t* scon, const char* message, const char* input, scon_size_t length,
    scon_size_t position);

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
#define SCON_CATCH(scon) SCON_SETJMP(*scon_jmp_buf_get(scon))

/**
 * @brief Throw an error in a SCON structure.
 *
 * @param scon Pointer to the SCON structure.
 * @param _msg The error message to set.
 */
#define SCON_THROW(scon, _msg) \
    do \
    { \
        scon_error_set((scon), (_msg), scon_input_get(scon), scon_input_get_length(scon), (scon_size_t) - 1); \
        SCON_LONGJMP(*scon_jmp_buf_get(scon), SCON_TRUE); \
    } while (0)

/**
 * @brief Throw an error in a SCON structure with a specific position.
 *
 * @param scon Pointer to the SCON structure.
 * @param _msg The error message to set.
 * @param _pos The position in the input buffer where the error occurred.
 */
#define SCON_THROW_POS(scon, _msg, _pos) \
    do \
    { \
        scon_error_set((scon), (_msg), scon_input_get(scon), scon_input_get_length(scon), (_pos)); \
        SCON_LONGJMP(*scon_jmp_buf_get(scon), SCON_TRUE); \
    } while (0)

/**
 * @brief Throw an error in a SCON structure with a specific item.
 *
 * @param scon Pointer to the SCON structure.
 * @param _msg The error message to set.
 * @param _item The item where the error occurred.
 */
#define SCON_THROW_ITEM(scon, _msg, _item) \
    do \
    { \
        scon_item_t* __item = (_item); \
        scon_error_set((scon), (_msg), __item->input ? __item->input->buffer : SCON_NULL, \
            __item->input ? __item->input->length : 0, __item->position); \
        SCON_LONGJMP(*scon_jmp_buf_get(scon), SCON_TRUE); \
    } while (0)

/**
 * @brief Throw an error in a SCON structure with a specific handle.
 *
 * @param scon Pointer to the SCON structure.
 * @param _msg The error message to set.
 * @param _handle The handle where the error occurred.
 */
#define SCON_THROW_HANDLE(scon, _msg, _handle) \
    do \
    { \
        scon_item_t* __item = SCON_HANDLE_TO_ITEM(_handle); \
        scon_error_set((scon), (_msg), __item->input ? __item->input->buffer : SCON_NULL, \
            __item->input ? __item->input->length : 0, __item->position); \
        SCON_LONGJMP(*scon_jmp_buf_get(scon), SCON_TRUE); \
    } while (0)

/**
 * @brief Create a new input structure and push it onto the input stack.
 *
 * @param scon The SCON structure.
 * @param buffer The input buffer.
 * @param length The length of the input buffer.
 * @param path The path to the input file.
 * @return A pointer to the newly created input structure.
 */
SCON_API scon_input_t* scon_input_new(scon_t* scon, const char* buffer, scon_size_t length, const char* path);

/** @} */

#endif
