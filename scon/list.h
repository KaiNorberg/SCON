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
 * A list is a dynamic array of handles with small-array optimization.
 *
 */

#define SCON_LIST_SMALL_MAX 4     ///< The maximum number of handles in a small list.
#define SCON_LIST_GROWTH_FACTOR 2 ///< The factor to grow a list by when it is full.

/**
 * @brief SCON list structure.
 * @struct scon_list_t
 */
typedef struct scon_list
{
    scon_uint32_t length;   ///< The number of handles in the list (must be first, check the `scon_item_t` structure).
    scon_uint32_t capacity; ///< The capacity of the list.
    scon_handle_t small[SCON_LIST_SMALL_MAX]; ///< The small handle buffer.
    scon_handle_t* handles;                   ///< The handles in the list.
} scon_list_t;

/**
 * @brief Helper to get the handle at an index in a list item.
 */
#define SCON_LIST_GET_HANDLE(_item, _index) ((_item)->list.handles[_index])

/**
 * @brief Helper to get the item pointer at an index in a list item.
 */
#define SCON_LIST_GET_ITEM(_item, _index) SCON_HANDLE_TO_ITEM(&(_item)->list.handles[_index])

/**
 * @brief Initialize a list structure.
 *
 * @param scon Pointer to the SCON structure.
 * @param list Pointer to the list to initialize.
 */
static inline void scon_list_init(struct scon* scon, scon_list_t* list)
{
    SCON_ASSERT(list != SCON_NULL);

    list->length = 0;
    list->handles = list->small;
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
 * @brief Append a handle to a list.
 *
 * @param scon Pointer to the SCON structure.
 * @param list Pointer to the list.
 * @param handle The handle to append.
 */
SCON_API void scon_list_append(struct scon* scon, scon_list_t* list, scon_handle_t handle);

/**
 * @brief Remove a handle from a list without maintaining order.
 *
 * @param scon Pointer to the SCON structure.
 * @param list Pointer to the list.
 * @param handle The handle to remove.
 */
SCON_API void scon_list_remove_unstable(struct scon* scon, scon_list_t* list, scon_handle_t handle);

/**
 * @brief Copy the contents of one list to another.
 *
 * @param scon Pointer to the SCON structure.
 * @param dest Pointer to the destination list.
 * @param src Pointer to the source list.
 */
SCON_API void scon_list_copy(struct scon* scon, scon_list_t* dest, scon_list_t* src);

#endif
