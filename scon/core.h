#ifndef SCON_CORE_H
#define SCON_CORE_H 1

struct scon_item;

#include "atom.h"
#include "error.h"
#include "item.h"
#include "list.h"

/**
 * @brief Core SCON definitions and structures.
 * @defgroup core
 * @file core.h
 *
 * @{
 */

#define SCON_BUCKETS_MAX 128  ///< Amount of buckets used for intering atoms.
#define SCON_CONSTANTS_MAX 8 ///< Maximum amount of predefined constants.

#define SCON_GC_THRESHOLD_INITIAL 128 ///< Initial blocks allocated threshold for garbage collection.

/**
 * @brief SCON input structure.
 * @struct scon_input_t
 */
typedef struct scon_input
{
    struct scon_input* prev;
    const char* buffer;
    const char* end;
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
    scon_error_t* error;
    struct scon_eval_state* evalState;
    int argc;
    char** argv;
} scon_t;

/**
 * @brief Create a new SCON structure.
 *
 * @param error Pointer to the error structure to be used for error reporting.
 * @return A pointer to the newly allocated SCON structure.
 */
SCON_API scon_t* scon_new(scon_error_t* error);

/**
 * @brief Free the SCON structure.
 *
 * @param scon Pointer to the SCON structure to free.
 */
SCON_API void scon_free(scon_t* scon);

/**
 * @brief Set the command line arguments for the SCON structure.
 * 
 * Will be utilized by the `(args!)` native.
 * 
 * @param scon Pointer to the SCON structure.
 * @param argc The number of arguments.
 * @param argv The argument strings.
 */
SCON_API void scon_args_set(scon_t* scon, int argc, char** argv);

/**
 * @brief Register a constant in a SCON structure.
 *
 * @param scon Pointer to the SCON structure.
 * @param name The name of the constant.
 * @param item The item associated with the constant.
 */
SCON_API void scon_constant_register(scon_t* scon, const char* name, struct scon_item* item);

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
