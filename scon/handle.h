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
 * A handle is a lightweight reference to a SCON item, with the ability to cache various flags from its referenced item or the integer/float value of an atom using Tagged Pointers (NaN Boxing).
 *
 * ## 64-bit Handle Bit Layout
 *
 * The top 16 bits are used as a type tag. The remaining 48 bits represent the payload.
 * For item pointers, the lowest 6 bits are guaranteed to be zero due to 64-byte alignment,
 * allowing us to store flags in these bits.
 *
 * | Tag (16 bits)   | Payload (42 bits)                 | Flags (6 bits) |
 * |-----------------|-----------------------------------|----------------|
 * | `0x0000`        | Item Pointer (`scon_item_t*`)     | Item Flags     |
 * | `0x0006`        | Integer Value (48-bit signed)     | *N/A*          |
 * | `0x0007...FFFF` | Float (Shifted IEEE 754 double)   | *N/A*          |
 *
 * @see [Wikipedia Tagged pointer](https://en.wikipedia.org/wiki/Tagged_pointer)
 *
 * @{
 */

/**
 * @brief SCON invalid handle constant.
 */
#define SCON_HANDLE_NONE 0x0000000000000000ULL

#define SCON_HANDLE_OFFSET_FLOAT 0x0007000000000000ULL ///< Offset used for encoding doubles.

#define SCON_HANDLE_TAG_INT 0x0006000000000000ULL  ///< Tag for integer handles.
#define SCON_HANDLE_TAG_ITEM 0x0000000000000000ULL ///< Tag for item handles.

#define SCON_HANDLE_MASK_TAG 0xFFFF000000000000ULL   ///< Mask for handle tag bits.
#define SCON_HANDLE_MASK_VAL 0x0000FFFFFFFFFFFFULL   ///< Mask for handle value bits.
#define SCON_HANDLE_MASK_PTR 0x0000FFFFFFFFFFC0ULL   ///< Mask for item pointer bits, leaving 6 bits for flags.
#define SCON_HANDLE_MASK_FLAGS 0x000000000000003FULL ///< Mask for 6 bits of flags in item handles.

/**
 * @brief Create a handle from an integer.
 *
 * @param _val The integer value.
 * @return The handle.
 */
#define SCON_HANDLE_FROM_INT(_val) (SCON_HANDLE_TAG_INT | ((scon_handle_t)(_val) & SCON_HANDLE_MASK_VAL))

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
            .u + SCON_HANDLE_OFFSET_FLOAT)

/**
 * @brief Create a handle from an item pointer.
 *
 * @param _ptr The pointer to the scon_item_t.
 * @return The handle.
 */
#define SCON_HANDLE_FROM_ITEM(_ptr) (SCON_HANDLE_TAG_ITEM | ((scon_handle_t)(void*)(_ptr) & SCON_HANDLE_MASK_PTR) | (((_ptr) != SCON_NULL) ? ((_ptr)->flags & SCON_HANDLE_MASK_FLAGS) : 0))

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
 * @brief Create a handle from a function pointer.
 *
 * @param _func The pointer to the scon_function_t.
 * @return The handle.
 */
#define SCON_HANDLE_FROM_FUNCTION(_func) SCON_HANDLE_FROM_ITEM(SCON_CONTAINER_OF(_func, scon_item_t, function))

/**
 * @brief Create a handle from a closure pointer.
 *
 * @param _closure The pointer to the scon_closure_t.
 * @return The handle.
 */
#define SCON_HANDLE_FROM_CLOSURE(_closure) SCON_HANDLE_FROM_ITEM(SCON_CONTAINER_OF(_closure, scon_item_t, closure))

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
#define SCON_HANDLE_IS_FLOAT(_handle) ((*(_handle)) >= SCON_HANDLE_OFFSET_FLOAT)

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
#define SCON_HANDLE_TO_INT(_handle) (((scon_int64_t)((*(_handle)) & SCON_HANDLE_MASK_VAL)))

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
    }){.u = (*(_handle)) - SCON_HANDLE_OFFSET_FLOAT}) \
            .d)

/**
 * @brief Get the item pointer of a handle.
 *
 * @param _handle Pointer to the handle.
 * @return The item pointer.
 */
#define SCON_HANDLE_TO_ITEM(_handle) ((scon_item_t*)(void*)((*(_handle)) & SCON_HANDLE_MASK_PTR))

/**
 * @brief Set flags on an item handle, uses the `scon_item_type_t`.
 * 
 * @param _handle Pointer to the handle.
 * @param _flags The flags to set.
 */
#define SCON_HANDLE_SET_FLAGS(_handle, _flags) \
    do { \
        if (SCON_HANDLE_IS_ITEM(_handle)) { \
            *(_handle) = ((*(_handle) & ~SCON_HANDLE_MASK_FLAGS) | ((_flags) & SCON_HANDLE_MASK_FLAGS)); \
        } \
    } while (0)

/**
 * @brief Get flags from an item handle.
 *
 * @param _handle Pointer to the handle.
 * @return The flags stored in the handle.
 */
#define SCON_HANDLE_GET_FLAGS(_handle) \
    (SCON_HANDLE_IS_ITEM(_handle) ? (scon_uint8_t)((*(_handle)) & SCON_HANDLE_MASK_FLAGS) : 0)

#define SCON_HANDLE_FALSE() SCON_HANDLE_FROM_INT(0) ///< Constant false handle.

#define SCON_HANDLE_TRUE() SCON_HANDLE_FROM_INT(1) ///< Constant true handle.

/**
 * @brief Compare two handles using a given operator with a fast path for integers and floats.
 *
 * @param _scon The SCON structure.
 * @param _a The first handle.
 * @param _b The second handle.
 * @param _op The comparison operator (e.g., <, >, <=, >=, etc.).
 * @return The result of the comparison.
 */
#define SCON_HANDLE_COMPARE_FAST(_scon, _a, _b, _op) \
    (((((*(_a)) ^ SCON_HANDLE_TAG_INT) | ((*(_b)) ^ SCON_HANDLE_TAG_INT)) & SCON_HANDLE_MASK_TAG) == 0 \
        ? (((scon_int64_t)((*(_a)) & SCON_HANDLE_MASK_VAL)) _op ((scon_int64_t)((*(_b)) & SCON_HANDLE_MASK_VAL))) \
        : (((*(_a)) >= SCON_HANDLE_OFFSET_FLOAT && (*(_b)) >= SCON_HANDLE_OFFSET_FLOAT) \
            ? (SCON_HANDLE_TO_FLOAT(_a) _op SCON_HANDLE_TO_FLOAT(_b)) \
            : (scon_handle_compare(_scon, _a, _b) _op 0)))

/**
 * @brief Perform a arithmetic operation on two handles with a fast path for integers and floats.
 *
 * @param _scon The SCON structure.
 * @param _a The target handle.
 * @param _b The first handle.
 * @param _c The second handle
 * @param _op The arithmetic operator, (e.g., +, -, *, etc.)
 */
#define SCON_HANDLE_ARITHMETIC_FAST(_scon, _a, _b, _c, _op) \
    do \
    { \
        scon_handle_t _bVal = *(_b); \
        scon_handle_t _cVal = *(_c); \
        if (SCON_LIKELY((((_bVal ^ SCON_HANDLE_TAG_INT) | (_cVal ^ SCON_HANDLE_TAG_INT)) & SCON_HANDLE_MASK_TAG) == 0)) \
        { \
            *(_a) = SCON_HANDLE_FROM_INT(((scon_int64_t)(_bVal & SCON_HANDLE_MASK_VAL)) _op ((scon_int64_t)(_cVal & SCON_HANDLE_MASK_VAL))); \
        } \
        else if (_bVal >= SCON_HANDLE_OFFSET_FLOAT && _cVal >= SCON_HANDLE_OFFSET_FLOAT) \
        { \
            *(_a) = SCON_HANDLE_FROM_FLOAT(SCON_HANDLE_TO_FLOAT(_b) _op SCON_HANDLE_TO_FLOAT(_c)); \
        } \
        else \
        { \
            scon_promotion_t prom; \
            scon_handle_promote(_scon, _b, _c, &prom); \
            if (prom.type == SCON_PROMOTION_TYPE_INT) \
            { \
                *(_a) = SCON_HANDLE_FROM_INT(prom.a.intVal _op prom.b.intVal); \
            } \
            else \
            { \
                *(_a) = SCON_HANDLE_FROM_FLOAT(prom.a.floatVal _op prom.b.floatVal); \
            } \
        } \
    } while (0)

/**
 * @brief Check if a handle is truthy.
 *
 * @param _handle Pointer to the handle.
 * @return `SCON_TRUE` if the handle is truthy, `SCON_FALSE` otherwise.
 */
#define SCON_HANDLE_IS_TRUTHY(_handle) \
    ((((*(_handle)) & SCON_HANDLE_MASK_TAG) == SCON_HANDLE_TAG_INT) \
        ? (((*(_handle)) & SCON_HANDLE_MASK_VAL) != 0) \
        : (((*(_handle)) >= SCON_HANDLE_OFFSET_FLOAT) \
            ? (SCON_HANDLE_TO_FLOAT(_handle) != 0.0) \
            : (((*(_handle)) & SCON_HANDLE_MASK_TAG) == SCON_HANDLE_TAG_ITEM \
                ? ((*(_handle)) != SCON_HANDLE_NONE && !((*(_handle)) & SCON_ITEM_FLAG_FALSY)) \
                : SCON_FALSE)))

/**
 * @brief Get the number of handles in a list or the number of characters in an atom.
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
 * @brief Promotion types for numeric operations.
 * @enum scon_promotion_type_t
 */
typedef enum
{
    SCON_PROMOTION_TYPE_NONE = 0,
    SCON_PROMOTION_TYPE_INT = 1,
    SCON_PROMOTION_TYPE_FLOAT = 2
} scon_promotion_type_t;

/**
 * @brief Promotion result for numeric operations.
 * @struct scon_promotion_t
 */
typedef struct
{
    scon_promotion_type_t type;
    union {
        scon_int64_t intVal;
        scon_float_t floatVal;
    } a;
    union {
        scon_int64_t intVal;
        scon_float_t floatVal;
    } b;
} scon_promotion_t;

/**
 * @brief Promote two handles to a common numeric type.
 *
 * If both handles are numeric, they are promoted to the highest precision type (float if either is a float, otherwise
 * integer).
 *
 * @param scon The SCON structure.
 * @param a The first handle.
 * @param b The second handle.
 * @param out The promotion result structure.
 */
SCON_API void scon_handle_promote(struct scon* scon, scon_handle_t* a, scon_handle_t* b, scon_promotion_t* out);

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
