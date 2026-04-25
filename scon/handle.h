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
 * A handle is a lightweight reference to a SCON item, with the ability to cache various flags from its referenced item
 * or the integer/float value of an atom using Tagged Pointers (NaN Boxing).
 *
 * ## 64-bit Handle Bit Layout
 *
 * The top 16 bits are used as a type tag. The remaining 48 bits represent the payload
 * (either a 48-bit integer or a 48-bit pointer).
 *
 * | Tag (16 bits)   | Payload (48 bits)                 |
 * |-----------------|-----------------------------------|
 * | `0x0000`        | Item Pointer (`scon_item_t*`)     |
 * | `0x0006`        | Integer Value (48-bit signed)     |
 * | `0x0007...FFFF` | Float (Shifted IEEE 754 double)   |
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
#define SCON_HANDLE_MASK_PTR SCON_HANDLE_MASK_VAL    ///< Mask for item pointer bits.
#define SCON_HANDLE_MASK_FLAGS 0x00000000000000FFULL ///< Mask for flags (not stored in handle anymore).

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
            .u + \
        SCON_HANDLE_OFFSET_FLOAT)

/**
 * @brief Create a handle from an item pointer.
 *
 * @param _ptr The pointer to the scon_item_t.
 * @return The handle.
 */
#define SCON_HANDLE_FROM_ITEM(_ptr) (SCON_HANDLE_TAG_ITEM | ((scon_handle_t)(void*)(_ptr) & SCON_HANDLE_MASK_PTR))

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
 * @brief Check if a handle is integer shaped.
 *
 * @param _handle Pointer to the handle.
 * @return Non-zero if the handle is integer shaped, zero otherwise.
 */
#define SCON_HANDLE_IS_INT_SHAPED(_handle) \
    (SCON_HANDLE_IS_INT(_handle) || (SCON_HANDLE_GET_FLAGS(_handle) & SCON_ITEM_FLAG_INT_SHAPED))

/**
 * @brief Check if a handle is float shaped.
 *
 * @param _handle Pointer to the handle.
 * @return Non-zero if the handle is float shaped, zero otherwise.
 */
#define SCON_HANDLE_IS_FLOAT_SHAPED(_handle) \
    (SCON_HANDLE_IS_FLOAT(_handle) || (SCON_HANDLE_GET_FLAGS(_handle) & SCON_ITEM_FLAG_FLOAT_SHAPED))

/**
 * @brief Get the type of the item referenced by the handle, or `SCON_ITEM_TYPE_ATOM` if not an item.
 * 
 * @param _handle Pointer to the handle.
 */
#define SCON_HANDLE_GET_TYPE(_handle) \
    (SCON_HANDLE_IS_ITEM(_handle) ? SCON_HANDLE_TO_ITEM(_handle)->type : SCON_ITEM_TYPE_ATOM)

/**
 * @brief Check if a handle is a number.
 *
 * @param _handle Pointer to the handle.
 * @return Non-zero if the handle is a number, zero otherwise.
 */
#define SCON_HANDLE_IS_NUMBER(_handle) (SCON_HANDLE_IS_INT_SHAPED(_handle) || SCON_HANDLE_IS_FLOAT_SHAPED(_handle))

/**
 * @brief Check if a handle is an item.
 *
 * @param _handle Pointer to the handle.
 * @return Non-zero if the handle is an item, zero otherwise.
 */
#define SCON_HANDLE_IS_ITEM(_handle) (((*(_handle)) & SCON_HANDLE_MASK_TAG) == SCON_HANDLE_TAG_ITEM)

/**
 * @brief Check if a handle is an atom.
 *
 * @param _handle Pointer to the handle.
 * @return Non-zero if the handle is an atom, zero otherwise.
 */
#define SCON_HANDLE_IS_ATOM(_handle) (SCON_HANDLE_GET_TYPE(_handle) == SCON_ITEM_TYPE_ATOM)

/**
 * @brief Check if a handle is a list.
 *
 * @param _handle Pointer to the handle.
 * @return Non-zero if the handle is a list, zero otherwise.
 */
#define SCON_HANDLE_IS_LIST(_handle) (SCON_HANDLE_GET_TYPE(_handle) == SCON_ITEM_TYPE_LIST)

/**
 * @brief Check if a handle is a function.
 *
 * @param _handle Pointer to the handle.
 * @return Non-zero if the handle is a function, zero otherwise.
 */
#define SCON_HANDLE_IS_FUNCTION(_handle) (SCON_HANDLE_GET_TYPE(_handle) == SCON_ITEM_TYPE_FUNCTION)

/**
 * @brief Check if a handle is a closure.
 *
 * @param _handle Pointer to the handle.
 * @return Non-zero if the handle is a closure, zero otherwise.
 */
#define SCON_HANDLE_IS_CLOSURE(_handle) (SCON_HANDLE_GET_TYPE(_handle) == SCON_ITEM_TYPE_CLOSURE)

/**
 * @brief Check if a handle is a lambda.
 *
 * @param _handle Pointer to the handle.
 * @return Non-zero if the handle is a lambda, zero otherwise.
 */
#define SCON_HANDLE_IS_LAMBDA(_handle) (SCON_HANDLE_IS_FUNCTION(_handle) || SCON_HANDLE_IS_CLOSURE(_handle))

/**
 * @brief Check if a handle is a native function.
 *
 * @param _handle Pointer to the handle.
 * @return Non-zero if the handle is a native function, zero otherwise.
 */
#define SCON_HANDLE_IS_NATIVE(_handle) \
    (SCON_HANDLE_IS_ITEM(_handle) && (SCON_HANDLE_GET_FLAGS(_handle) & SCON_ITEM_FLAG_NATIVE))

/**
 * @brief Check if a handle is callable.
 *
 * @param _handle Pointer to the handle.
 * @return Non-zero if the handle is callable, zero otherwise.
 */
#define SCON_HANDLE_IS_CALLABLE(_handle) (SCON_HANDLE_IS_LAMBDA(_handle) || SCON_HANDLE_IS_NATIVE(_handle))

/**
 * @brief Get the integer value of a handle.
 *
 * @param _handle Pointer to the handle.
 * @return The integer value.
 */
#define SCON_HANDLE_TO_INT(_handle) (((scon_int64_t)((*(_handle)) << 16)) >> 16)

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
 * @brief Get flags from an item handle.
 *
 * @param _handle Pointer to the handle.
 * @return The flags stored in the handle.
 */
#define SCON_HANDLE_GET_FLAGS(_handle) (SCON_HANDLE_IS_ITEM(_handle) ? SCON_HANDLE_TO_ITEM(_handle)->flags : 0)

#define SCON_HANDLE_FALSE() SCON_HANDLE_FROM_INT(0) ///< Constant false handle.

#define SCON_HANDLE_TRUE() SCON_HANDLE_FROM_INT(1) ///< Constant true handle.

/**
 * @brief Create a boolean handle from a C condition.
 * 
 * @param _cond The condition to evaluate.
 */
#define SCON_HANDLE_FROM_BOOL(_cond) ((_cond) ? SCON_HANDLE_TRUE() : SCON_HANDLE_FALSE())

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
            ? (SCON_HANDLE_TO_INT(_a) _op SCON_HANDLE_TO_INT(_b)) \
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
        if (SCON_LIKELY( \
                (((_bVal ^ SCON_HANDLE_TAG_INT) | (_cVal ^ SCON_HANDLE_TAG_INT)) & SCON_HANDLE_MASK_TAG) == 0)) \
        { \
            *(_a) = SCON_HANDLE_FROM_INT(SCON_HANDLE_TO_INT(&_bVal) _op SCON_HANDLE_TO_INT(&_cVal)); \
        } \
        else if (SCON_HANDLE_IS_FLOAT(&_bVal) && SCON_HANDLE_IS_FLOAT(&_cVal)) \
        { \
            *(_a) = SCON_HANDLE_FROM_FLOAT(SCON_HANDLE_TO_FLOAT(&_bVal) _op SCON_HANDLE_TO_FLOAT(&_cVal)); \
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
    (SCON_HANDLE_IS_INT(_handle) \
            ? (SCON_HANDLE_TO_INT(_handle) != 0) \
            : (SCON_HANDLE_IS_FLOAT(_handle) \
                      ? (SCON_HANDLE_TO_FLOAT(_handle) != 0.0) \
                      : (SCON_HANDLE_IS_ITEM(_handle) \
                                ? ((*(_handle)) != SCON_HANDLE_NONE && \
                                      !(SCON_HANDLE_TO_ITEM(_handle)->flags & SCON_ITEM_FLAG_FALSY)) \
                                : SCON_FALSE)))

/**
 * @brief Ensure that a handle is an item handle.
 *
 * If the handle is an integer or float, it will be upgraded to an item handle by looking up a corresponding atom.
 *
 * @param scon The SCON structure.
 * @param handle The handle to ensure.
 */
SCON_API void scon_handle_ensure_item(struct scon* scon, scon_handle_t* handle);

/**
 * @brief Ensure that a handle is an item and return the pointer.
 *
 * @param scon The SCON structure.
 * @param handle The handle.
 * @return The item pointer.
 */
SCON_API struct scon_item* scon_handle_item(struct scon* scon, scon_handle_t* handle);

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

/**
 * @brief Get the string pointer and length from an atom handle.
 *
 * @param scon The SCON structure.
 * @param handle The handle to the atom.
 * @param outStr Pointer to store the string pointer.
 * @param outLen Pointer to store the string length.
 */
SCON_API void scon_handle_get_string_params(struct scon* scon, scon_handle_t* handle, char** outStr,
    scon_size_t* outLen);

/** @} */

#endif
