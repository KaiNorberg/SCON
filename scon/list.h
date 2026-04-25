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
 * A list is a persistent data structure implemented using a bit-mapped vector trie.
 *
 * The list is made up of a "tree" of nodes along with a "tail" node, with each node storing a fixed size array of
 * either children or elements within the list.
 *
 * When an element is added to a list, it will be appended to the array within the tail node, once the tail node is
 * full, it is "pushed" to the front of the tree, which may require increasing the depth of tree.
 *
 * @see [Persistent vectors, Part 2 -- Immutability and persistence]
 * (https://dmiller.github.io/clojure-clr-next/general/2023/02/12/PersistentVector-part-2.html)
 *
 */

#define SCON_LIST_BITS 2 ///< Number of bits per level in the trie.
#define SCON_LIST_WIDTH (1 << SCON_LIST_BITS) ///< The number of children per node.
#define SCON_LIST_MASK (SCON_LIST_WIDTH - 1) ///< Mask for the index at each level.

/**
 * @brief SCON list node structure.
 * @struct scon_list_node_t
 */
typedef struct scon_list_node
{
    union
    {
        struct scon_list_node* children[SCON_LIST_WIDTH];
        scon_handle_t handles[SCON_LIST_WIDTH];
    };
} scon_list_node_t;

/**
 * @brief SCON list structure.
 * @struct scon_list_t
 */
typedef struct scon_list
{
    scon_uint32_t length;      ///< Total number of elements.
    scon_uint32_t shift;       ///< The amount to shift the index to compute access paths.
    scon_list_node_t* root;       ///< Pointer to the trie root node.
    scon_list_node_t* tail;       ///< Pointer to the tail node.
} scon_list_t;

/**
 * @brief Create a new editable list.
 *
 * @param scon Pointer to the SCON structure.
 * @return A pointer to the newly created list.
 */
SCON_API scon_list_t* scon_list_new(struct scon* scon);

/**
 * @brief Create a new list with an updated value at the specified index.
 *
 * @param scon Pointer to the SCON structure.
 * @param list Pointer to the source list.
 * @param index The index to update.
 * @param val The new value to set.
 * @return A pointer to the newly created list.
 */
SCON_API scon_list_t* scon_list_assoc(struct scon* scon, scon_list_t* list, scon_size_t index, scon_handle_t val);

/**
 * @brief Create a new list with the element at the specified index removed.
 *
 * @param scon Pointer to the SCON structure.
 * @param list Pointer to the source list.
 * @param index The index of the element to remove.
 * @return A pointer to the newly created list.
 */
SCON_API scon_list_t* scon_list_dissoc(struct scon* scon, scon_list_t* list, scon_size_t index);

/**
 * @brief Create a new list by slicing an existing list.
 *
 * @param scon Pointer to the SCON structure.
 * @param list Pointer to the source list.
 * @param start The starting index (inclusive).
 * @param end The ending index (exclusive).
 * @return A pointer to the newly created list slice.
 */
SCON_API scon_list_t* scon_list_slice(struct scon* scon, scon_list_t* list, scon_size_t start, scon_size_t end);

/**
 * @brief Get the nth element of the list.
 *
 * @param scon Pointer to the SCON structure.
 * @param list Pointer to the list.
 * @param index The index of the element to retrieve.
 * @return The handle of the nth element.
 */
SCON_API scon_handle_t scon_list_nth(struct scon* scon, scon_list_t* list, scon_size_t index);

/**
 * @brief Get the nth element of the list as an item.
 *
 * @param scon Pointer to the SCON structure.
 * @param list Pointer to the list.
 * @param index The index of the element to retrieve.
 * @return A pointer to the item of the nth element.
 */
SCON_API struct scon_item* scon_list_nth_item(struct scon* scon, scon_list_t* list, scon_size_t index);

/**
 * @brief Append an element to the list.
 * 
 * @param scon Pointer to the SCON structure.
 * @param list The target list (must be editable).
 * @param val Handle to the value to append.
 */
SCON_API void scon_list_append(struct scon* scon, scon_list_t* list, scon_handle_t val);

/**
 * @brief Append all elements from one list to another.
 * 
 * @param scon Pointer to the SCON structure.
 * @param list The target list (must be editable).
 * @param other The source list to copy from.
 */
SCON_API void scon_list_append_list(struct scon* scon, scon_list_t* list, scon_list_t* other);

/**
 * @brief SCON list iterator structure.
 */
typedef struct scon_list_iter
{
    scon_list_t* list;
    scon_size_t index;
    scon_list_node_t* leaf;
    scon_size_t tailOffset;
} scon_list_iter_t;

/**
 * @brief Calculate the offset of the tail node.
 */
#define SCON_LIST_TAIL_OFFSET(_list) (((_list)->length > 0) ? (((_list)->length - 1) & ~SCON_LIST_MASK) : 0)

/**
 * @brief Create a initializer for a list iterator.
 * 
 * @param _list The list to iterate over.
 */
#define SCON_LIST_ITER(_list) {(_list), 0, SCON_NULL, SCON_LIST_TAIL_OFFSET(_list) }

/**
 * @brief Create a initializer for a list iterator start at a specific index.
 *
 * @param _list The list to iterate over.
 * @param _start The starting index.
 */
#define SCON_LIST_ITER_AT(_list, _start) {(_list), ((_start) > (_list)->length) ? (_list)->length : (_start), SCON_NULL, SCON_LIST_TAIL_OFFSET(_list) }
         
/**
 * @brief Get the next element from the iterator.
 * 
 * @param iter Pointer to the iterator.
 * @param out Pointer to store the retrieved handle.
 * @return SCON_TRUE if an element was retrieved, SCON_FALSE if the end was reached.
 */
SCON_API scon_bool_t scon_list_iter_next(scon_list_iter_t* iter, scon_handle_t* out);

/**
 * @brief Macro for iterating over all elements in a list.
 * 
 * @param _handle The scon_handle_t variable to store each element.
 * @param _list Pointer to the scon_list_t to iterate.
 */
#define SCON_LIST_FOR_EACH(_handle, _list) for (scon_list_iter_t _iter = SCON_LIST_ITER(_list); scon_list_iter_next(&_iter, (_handle)); )

#endif
