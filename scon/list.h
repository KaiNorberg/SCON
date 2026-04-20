#ifndef SCON_LIST_H
#define SCON_LIST_H 1

struct scon;
struct scon_item;

#include "defs.h"

/**
 * @brief SCON list management.
 * @defgroup list List
 * @file list.h
 *
 * A list is a dynamic array of items with small-array optimization.
 *
 */

#define SCON_LIST_SMALL_MAX 4     ///< The maximum number of items in a small list.
#define SCON_LIST_GROWTH_FACTOR 2 ///< The factor to grow a list by when it is full.

/**
 * @brief SCON list structure.
 * @struct scon_list_t
 */
typedef struct scon_list
{
    scon_uint32_t length;   ///< The number of items in the list (must be first, check the `scon_item_t` structure).
    scon_uint32_t capacity; ///< The capacity of the list.
    struct scon_item* small[SCON_LIST_SMALL_MAX]; ///< The small string buffer.
    struct scon_item** items;                     ///< The items in the list.
} scon_list_t;

/**
 * @brief Initialize a list structure.
 *
 * @param scon Pointer to the SCON structure.
 * @param list Pointer to the list to initialize.
 */
static inline void scon_list_init(struct scon* scon, scon_list_t* list)
{
    list->length = 0;
    list->items = list->small;
    list->capacity = SCON_LIST_SMALL_MAX;
}

/**
 * @brief Deinitialize a list structure.
 *
 * @param scon Pointer to the SCON structure.
 * @param list Pointer to the list to deinitialize.
 */
SCON_API void scon_list_deinit(struct scon* scon, scon_list_t* list);

/**
 * @brief Create a new list.
 *
 * @param scon Pointer to the SCON structure.
 * @param capacity The initial capacity of the list.
 * @return A pointer to the newly created list.
 */
SCON_API scon_list_t* scon_list_new(struct scon* scon, scon_size_t capacity);

/**
 * @brief Append an item to a list.
 *
 * @param scon Pointer to the SCON structure.
 * @param list Pointer to the list.
 * @param item The item to append.
 */
SCON_API void scon_list_append(struct scon* scon, scon_list_t* list, struct scon_item* item);

/**
 * @brief Remove an item from a list without maintaining order.
 *
 * @param scon Pointer to the SCON structure.
 * @param list Pointer to the list.
 * @param item The item to remove.
 */
SCON_API void scon_list_remove_unstable(struct scon* scon, scon_list_t* list, struct scon_item* item);

#endif
