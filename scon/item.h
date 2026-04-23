#ifndef SCON_ITEM_H
#define SCON_ITEM_H 1

#include "atom.h"
#include "closure.h"
#include "defs.h"
#include "function.h"
#include "list.h"

/**
 * @brief SCON item management.
 * @defgroup item Item
 * @file item.h
 *
 * An item is a generic container for all data types and heap alloacted structures within SCON.
 *
 * To optimize memory cacheing and reduce fragmentation, all items are 64 bytes and aligned to cache lines.
 *
 * @{
 */

/**
 * @brief SCON item type enumeration.
 */
typedef scon_uint8_t scon_item_type_t;
#define SCON_ITEM_TYPE_NONE 0     ///< No type.
#define SCON_ITEM_TYPE_ATOM 1     ///< An atom.
#define SCON_ITEM_TYPE_LIST 2     ///< A list.
#define SCON_ITEM_TYPE_FUNCTION 3 ///< A function.
#define SCON_ITEM_TYPE_CLOSURE 4  ///< A closure.

/**
 * @brief SCON item flags enumeration.
 */
typedef scon_uint8_t scon_item_flags_t;
#define SCON_ITEM_FLAG_NONE 0                ///< No flags.
#define SCON_ITEM_FLAG_FALSY (1 << 0)        ///< Item is falsy.
#define SCON_ITEM_FLAG_INT_SHAPED (1 << 1)   ///< Item is an integer shaped atom.
#define SCON_ITEM_FLAG_FLOAT_SHAPED (1 << 2) ///< Item is a float shaped atom.
#define SCON_ITEM_FLAG_INTRINSIC (1 << 3)    ///< Item is an atom and a intrinsic.
#define SCON_ITEM_FLAG_NATIVE (1 << 4)       ///< Item is an atom and a native function.
#define SCON_ITEM_FLAG_QUOTED (1 << 5)       ///< Item is a quoted atom.
#define SCON_ITEM_FLAG_GC_MARK (1 << 6)      ///< Item is marked by GC.

/**
 * @brief SCON item structure.
 * @struct scon_item_t
 *
 * Should be exactly 64 bytes for caching.
 *
 * @see handle
 */
typedef struct scon_item
{
    struct scon_input* input;  ///< The parsed input that created this item.
    scon_uint32_t position;    ///< The position in the input buffer where the item was parsed.
    scon_item_flags_t flags;   ///< Flags for the item.
    scon_item_type_t type;     ///< The type of the item.
    scon_uint16_t retainCount; ///< The reference count for GC retention.
    union {
        scon_uint32_t length; ///< Common length for the item. (Stored in the union to save space due to padding rules.)
        scon_atom_t atom;     ///< An atom.
        scon_list_t list;     ///< A list.
        scon_function_t function; ///< A function.
        scon_closure_t closure;   ///< A closure.
        struct scon_item* free;   ///< The next free item in the free list.
    };
} scon_item_t;

#ifdef _Static_assert
_Static_assert(sizeof(scon_item_t) == 64, "scon_item_t must be 64 bytes");
#endif

#define SCON_ITEM_BLOCK_MAX 1023 ///< The maximum number of items in a block.

/**
 * @brief SCON item block structure.
 * @struct scon_item_block_t
 *
 * Should be a power of two size as that should help most memory allocators.
 */
typedef struct scon_item_block
{
    struct scon_item_block* next;
    scon_uint8_t _padding[sizeof(scon_item_t) - sizeof(struct scon_item_block*)];
    scon_item_t items[SCON_ITEM_BLOCK_MAX];
} scon_item_block_t;

#ifdef _Static_assert
_Static_assert((sizeof(scon_item_block_t) & (sizeof(scon_item_block_t) - 1)) == 0,
    "scon_item_block_t must be a power of two");
#endif

/**
 * @brief Allocate a new SCON item.
 *
 * @param scon Pointer to the SCON structure.
 * @return A pointer to the newly created item.
 */
SCON_API scon_item_t* scon_item_new(struct scon* scon);

/**
 * @brief Free an SCON item.
 *
 * @param scon Pointer to the SCON structure.
 * @param item Pointer to the item to free.
 */
SCON_API void scon_item_free(struct scon* scon, scon_item_t* item);

/**
 * @brief Get the string representation of an SCON item type.
 *
 * @param type The item type.
 * @return The string representation of the item type.
 */
SCON_API const char* scon_item_type_str(scon_item_type_t type);

/** @} */

#endif
