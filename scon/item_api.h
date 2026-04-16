#ifndef SCON_ITEM_API_H
#define SCON_ITEM_API_H 1

#include "core_api.h"

/**
 * @brief The internal representation of a SCON item.
 * @struct _scon_node_t
 */
typedef struct _scon_node _scon_node_t;

/**
 * @brief SCON item type.
 */
typedef scon_uint16_t scon_item_type_t;
#define SCON_ITEM_NONE    0
#define SCON_ITEM_ATOM    1
#define SCON_ITEM_LIST    2

#ifdef SCON_USE_NAN_BOXING

typedef uint64_t scon_item_t;

#define SCON_NONE 0xFFFE000000000000ULL

#else

#define _SCON_INTERNAL_TYPE_INT   10
#define _SCON_INTERNAL_TYPE_FLOAT 11
#define _SCON_INTERNAL_TYPE_NODE  12

typedef struct 
{
    scon_uint16_t internalType;
    union {
        scon_int64_t i;
        scon_float_t f;
        void* node;
    } as;
} scon_item_t;

#define SCON_NONE ((scon_item_t){ .internalType = SCON_ITEM_NONE })

#endif

/**
 * @brief Get the number of items in a list or the number of characters in an atom.
 *
 * @param scon The SCON structure.
 * @param item The item to the check.
 * @return The length of the item.
 */
SCON_API scon_size_t scon_item_len(scon_t* scon, scon_item_t* item);

/**
 * @brief Get the type of a SCON item.
 *
 * @param scon The SCON structure.
 * @param item The item to check.
 * @return The type of the item.
 */
SCON_API scon_item_type_t scon_item_get_type(scon_t* scon, scon_item_t* item);

/**
 * @brief Get the string representation of an atom.
 *
 * @param scon The SCON structure.
 * @param item The item to get the string representation of, must be an atom.
 * @param out The output buffer.
 * @param len The length of the output buffer.
 */
SCON_API void scon_item_get_string(scon_t* scon, scon_item_t* item, char** out, scon_size_t* len);

/**
 * @brief Get the integer value of an atom.
 *
 * @param scon The SCON structure.
 * @param item The item to get the integer value of, must be integer-shaped.
 * @return The integer value of the item.
 */
SCON_API scon_int64_t scon_item_get_int(scon_t* scon, scon_item_t* item);

/**
 * @brief Get the float value of an atom.
 *
 * @param scon The SCON structure.
 * @param item The item to get the float value of, must be float-shaped.
 * @return The float value of the item.
 */
SCON_API scon_float_t scon_item_get_float(scon_t* scon, scon_item_t* item);

/**
 * @brief Check if an item is number-shaped (integer or float).
 *
 * @param scon The SCON structure.
 * @param item The item to check.
 * @return `SCON_TRUE` if the item is a number, `SCON_FALSE` otherwise.
 */
SCON_API scon_bool_t scon_item_is_number(scon_t* scon, scon_item_t* item);

/**
 * @brief Check if an item is integer-shaped.
 *
 * @param scon The SCON structure.
 * @param item The item to check.
 * @return `SCON_TRUE` if the item is an integer, `SCON_FALSE` otherwise.
 */
SCON_API scon_bool_t scon_item_is_int(scon_t* scon, scon_item_t* item);

/**
 * @brief Check if an item is float-shaped.
 *
 * @param scon The SCON structure.
 * @param item The item to check.
 * @return `SCON_TRUE` if the item is a float, `SCON_FALSE` otherwise.
 */
SCON_API scon_bool_t scon_item_is_float(scon_t* scon, scon_item_t* item);

/**
 * @brief Check if an item is considered truthy.
 *
 * @param scon The SCON structure.
 * @param item The item to check.
 * @return `SCON_TRUE` if the item is truthy, `SCON_FALSE` otherwise.
 */
SCON_API scon_bool_t scon_item_is_truthy(scon_t* scon, scon_item_t* item);

/**
 * @brief Check if two items are exactly equal string-wise or structurally.
 *
 * @param scon The SCON structure.
 * @param a The first item.
 * @param b The second item.
 * @return `SCON_TRUE` if the items are strictly equal, `SCON_FALSE` otherwise.
 */
SCON_API scon_bool_t scon_item_is_equal(scon_t* scon, scon_item_t* a, scon_item_t* b);

/**
 * @brief Compare two items for ordering (less than, equal, or greater than).
 *
 * Useful for sorting or range checks.
 *
 * @param scon The SCON structure.
 * @param a The first item.
 * @param b The second item.
 * @return A negative value if a < b, zero if a == b, and a positive value if a > b.
 */
SCON_API scon_int64_t scon_item_compare(scon_t* scon, scon_item_t* a, scon_item_t* b);

/**
 * @brief Find a sub-list by its head atom name.
 *
 * Given a list, this searches for a child item that is itself a list whose first element
 * is an atom matching `name`.
 *
 * @param scon The SCON structure.
 * @param list The item to the parent list.
 * @param name The name of the head atom to search for.
 * @return A item to the found list, or a item with index `SCON_NONE` if not found.
 */
SCON_API scon_item_t scon_item_get(scon_t* scon, scon_item_t* list, const char* name);

/**
 * @brief Retrieve the n-th item in a SCON list.
 *
 * @param scon The SCON structure.
 * @param list The item to the list item.
 * @param n The index of the item to retrieve.
 * @return A item to the n-th item, or a item with index `SCON_NONE` if not found.
 */
SCON_API scon_item_t scon_item_nth(scon_t* scon, scon_item_t* list, scon_size_t n);

#endif
