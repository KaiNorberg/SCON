#ifndef SCON_HANDLE_H
#define SCON_HANDLE_H 1

struct scon;

#include "defs.h"
#include "item.h"

/**
 * @brief SCON handle management.
 * @defgroup handle Handle
 * @file handle.h
 *
 * A handle is a lightweight reference to a SCON item, with the ability to cache the integer or float value of an atom
 * without needing to look up the underlying item using Tagged Pointers (NaN Boxing).
 *
 * @see [Wikipedia Tagged pointer](https://en.wikipedia.org/wiki/Tagged_pointer)
 *
 * @{
 */

/**
 * @brief SCON handle type.
 */
typedef scon_uint64_t scon_handle_t;

/**
 * @brief SCON invalid handle constant.
 */
#define SCON_HANDLE_NONE 0xFFFE000000000000ULL

#define SCON_HANDLE_TAG_INT 0xFFFC000000000000ULL  ///< Tag for integer handles.
#define SCON_HANDLE_TAG_ITEM 0xFFFE000000000000ULL ///< Tag for item handles.

#define SCON_HANDLE_MASK_TAG 0xFFFF000000000000ULL ///< Mask for handle tag bits.
#define SCON_HANDLE_MASK_VAL 0x0000FFFFFFFFFFFFULL ///< Mask for handle value bits.

/**
 * @brief Create a handle from an integer.
 *
 * @param _val The integer value.
 * @return The handle.
 */
#define SCON_HANDLE_FROM_INT(_val) (SCON_HANDLE_TAG_INT | ((scon_handle_t)(scon_uint32_t)(_val)))

/**
 * @brief Create a handle from a float.
 *
 * @param _val The float value.
 * @return The handle.
 */
#define SCON_HANDLE_FROM_FLOAT(_val) \
    (((union { \
        double d; \
        scon_handle_t u; \
    }){.d = (_val)}) \
            .u)

/**
 * @brief Create a handle from an item pointer.
 *
 * @param _ptr The pointer to the scon_item_t.
 * @return The handle.
 */
#define SCON_HANDLE_FROM_ITEM(_ptr) (SCON_HANDLE_TAG_ITEM | ((scon_handle_t)(void*)(_ptr) & SCON_HANDLE_MASK_VAL))

/**
 * @brief Create a handle from an atom pointer.
 *
 * @param _atom The pointer to the scon_atom_t.
 * @return The handle.
 */
#define SCON_HANDLE_FROM_ATOM(_atom) SCON_HANDLE_FROM_ITEM(SCON_CONTAINER_OF(_atom, scon_item_t, atom))

/**
 * @brief Create a handle from a list pointer.
 *
 * @param _list The pointer to the scon_list_t.
 * @return The handle.
 */
#define SCON_HANDLE_FROM_LIST(_list) SCON_HANDLE_FROM_ITEM(SCON_CONTAINER_OF(_list, scon_item_t, list))

/**
 * @brief Check if a handle is an integer.
 *
 * @param _handle Pointer to the handle.
 * @return Non-zero if the handle is an integer, zero otherwise.
 */
#define SCON_HANDLE_IS_INT(_handle) (((*(_handle)) & SCON_HANDLE_MASK_TAG) == SCON_HANDLE_TAG_INT)

/**
 * @brief Check if a handle is a float.
 *
 * @param _handle Pointer to the handle.
 * @return Non-zero if the handle is a float, zero otherwise.
 */
#define SCON_HANDLE_IS_FLOAT(_handle) ((*(_handle)) < SCON_HANDLE_TAG_INT)

/**
 * @brief Check if a handle is an item.
 *
 * @param _handle Pointer to the handle.
 * @return Non-zero if the handle is an item, zero otherwise.
 */
#define SCON_HANDLE_IS_ITEM(_handle) (((*(_handle)) & SCON_HANDLE_MASK_TAG) == SCON_HANDLE_TAG_ITEM)

/**
 * @brief Get the integer value of a handle.
 *
 * @param _handle Pointer to the handle.
 * @return The integer value.
 */
#define SCON_HANDLE_TO_INT(_handle) ((scon_int64_t)((*(_handle)) & SCON_HANDLE_MASK_VAL))

/**
 * @brief Get the float value of a handle.
 *
 * @param _handle Pointer to the handle.
 * @return The float value.
 */
#define SCON_HANDLE_TO_FLOAT(_handle) \
    (((union { \
        scon_handle_t u; \
        double d; \
    }){.u = (*(_handle))}) \
            .d)

/**
 * @brief Get the item pointer of a handle.
 *
 * @param _handle Pointer to the handle.
 * @return The item pointer.
 */
#define SCON_HANDLE_TO_ITEM(_handle) ((scon_item_t*)(void*)((*(_handle)) & SCON_HANDLE_MASK_VAL))

/**
 * @brief Get the number of items in a list or the number of characters in an atom.
 *
 * @param scon The SCON structure.
 * @param handle The handle to the check, will be upgraded to an item.
 * @return The length of the handle.
 */
SCON_API scon_size_t scon_handle_len(struct scon* scon, scon_handle_t* handle);

/**
 * @brief Get the type of a SCON handle.
 *
 * A handle is considered an atom even if it is an integer or a float.
 *
 * @param scon The SCON structure.
 * @param handle The handle to check, might be upgraded to an item.
 * @return The type of the handle.
 */
SCON_API scon_item_type_t scon_handle_get_type(struct scon* scon, scon_handle_t* handle);

/**
 * @brief Get the string representation of an atom.
 *
 * @param scon The SCON structure.
 * @param handle The handle to get the string representation of, must be an atom, might be upgraded.
 * @param out The output buffer.
 * @param len The length of the output buffer.
 */
SCON_API void scon_handle_get_string(struct scon* scon, scon_handle_t* handle, char** out, scon_size_t* len);

/**
 * @brief Get the integer value of an atom.
 *
 * @param scon The SCON structure.
 * @param handle The handle to get the integer value of, must be integer-shaped, might be upgraded to an item.
 * @return The integer value of the handle.
 */
SCON_API scon_int64_t scon_handle_get_int(struct scon* scon, scon_handle_t* handle);

/**
 * @brief Get the float value of an atom.
 *
 * @param scon The SCON structure.
 * @param handle The handle to get the float value of, must be float-shaped, might be upgraded to an item.
 * @return The float value of the handle.
 */
SCON_API scon_float_t scon_handle_get_float(struct scon* scon, scon_handle_t* handle);

/**
 * @brief Check if an handle is number-shaped (integer or float).
 *
 * @param scon The SCON structure.
 * @param handle The handle to check, might be upgraded to an item.
 * @return `SCON_TRUE` if the handle is a number, `SCON_FALSE` otherwise.
 */
SCON_API scon_bool_t scon_handle_is_number(struct scon* scon, scon_handle_t* handle);

/**
 * @brief Check if an handle is integer-shaped.
 *
 * @param scon The SCON structure.
 * @param handle The handle to check, might be upgraded to an item.
 * @return `SCON_TRUE` if the handle is an integer, `SCON_FALSE` otherwise.
 */
SCON_API scon_bool_t scon_handle_is_int(struct scon* scon, scon_handle_t* handle);

/**
 * @brief Check if an handle is float-shaped.
 *
 * @param scon The SCON structure.
 * @param handle The handle to check, might be upgraded to an item.
 * @return `SCON_TRUE` if the handle is a float, `SCON_FALSE` otherwise.
 */
SCON_API scon_bool_t scon_handle_is_float(struct scon* scon, scon_handle_t* handle);

/**
 * @brief Check if an handle is considered truthy.
 *
 * @param scon The SCON structure.
 * @param handle The handle to check.
 * @return `SCON_TRUE` if the handle is truthy, `SCON_FALSE` otherwise.
 */
SCON_API scon_bool_t scon_handle_is_truthy(struct scon* scon, scon_handle_t* handle);

/**
 * @brief Check if two items are exactly equal string-wise or structurally.
 *
 * @param scon The SCON structure.
 * @param a The first handle, will be upgraded.
 * @param b The second handle, will be upgraded.
 * @return `SCON_TRUE` if the items are strictly equal, `SCON_FALSE` otherwise.
 */
SCON_API scon_bool_t scon_handle_is_equal(struct scon* scon, scon_handle_t* a, scon_handle_t* b);

/**
 * @brief Compare two items for ordering (less than, equal, or greater than).
 *
 * Useful for sorting or range checks.
 *
 * @param scon The SCON structure.
 * @param a The first handle.
 * @param b The second handle.
 * @return A negative value if a < b, zero if a == b, and a positive value if a > b.
 */
SCON_API scon_int64_t scon_handle_compare(struct scon* scon, scon_handle_t* a, scon_handle_t* b);

/**
 * @brief Find a sub-list by its head atom name.
 *
 * Given a list, this searches for a child handle that is itself a list whose first element
 * is an atom matching `name`.
 *
 * @param scon The SCON structure.
 * @param list The handle to the parent list.
 * @param name The name of the head atom to search for.
 * @return A handle to the found list, or a handle with index `SCON_HANDLE_NONE` if not found.
 */
SCON_API scon_handle_t scon_handle_get(struct scon* scon, scon_handle_t* list, const char* name);

/**
 * @brief Retrieve the n-th handle in a SCON list.
 *
 * @param scon The SCON structure.
 * @param list The handle to the list handle.
 * @param n The index of the handle to retrieve.
 * @return A handle to the n-th handle, or a handle with index `SCON_HANDLE_NONE` if not found.
 */
SCON_API scon_handle_t scon_handle_nth(struct scon* scon, scon_handle_t* list, scon_size_t n);

/**
 * @brief Get the constant true handle.
 *
 * @param scon Pointer to the SCON structure.
 * @return The true handle.
 */
SCON_API scon_handle_t scon_handle_true(struct scon* scon);

/**
 * @brief Get the constant false handle.
 *
 * @param scon Pointer to the SCON structure.
 * @return The false handle.
 */
SCON_API scon_handle_t scon_handle_false(struct scon* scon);

/**
 * @brief Get the constant nil handle.
 *
 * @param scon Pointer to the SCON structure.
 * @return The nil handle.
 */
SCON_API scon_handle_t scon_handle_nil(struct scon* scon);

/**
 * @brief Get the constant PI handle.
 *
 * @param scon Pointer to the SCON structure.
 * @return The PI handle.
 */
SCON_API scon_handle_t scon_handle_pi(struct scon* scon);

/**
 * @brief Get the constant E handle.
 *
 * @param scon Pointer to the SCON structure.
 * @return The E handle.
 */
SCON_API scon_handle_t scon_handle_e(struct scon* scon);

/** @} */

#endif
