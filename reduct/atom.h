#ifndef REDUCT_ATOM_H
#define REDUCT_ATOM_H 1

#include "defs.h"
#include "intrinsic.h"
#include "native.h"

struct reduct;

/**
 * @file atom.h
 * @brief Atom representation and operations.
 * @defgroup atom Atoms
 *
 * Atoms represent all strings within a Reduct expression, as such it also represents anything that a string can be,
 * including integers, floats and intrinsics.
 *
 * @{
 */

#define REDUCT_ATOM_MAP_INITIAL 256 ///< The initial size of the atom map.
#define REDUCT_ATOM_MAP_GROWTH 2    ///< The growth factor of the atom map.
#define REDUCT_ATOM_SMALL_MAX 16 ///< The maximum length of a small atom.

#define REDUCT_ATOM_INDEX_NONE ((reduct_uint32_t)-1) ///< The value of an unindexed atom.

/**
 * @brief Atom lookup flags.
 */
typedef enum
{
    REDUCT_ATOM_LOOKUP_NONE = 0,       ///< No flags.
    REDUCT_ATOM_LOOKUP_QUOTED = 1 << 0 ///< Atom should be explicitly quoted.
} reduct_atom_lookup_flags_t;

typedef reduct_uint8_t reduct_atom_flags_t;
#define REDUCT_ATOM_FLAG_NONE 0                ///< No flags.
#define REDUCT_ATOM_FLAG_INTEGER (1 << 0)   ///< Atom is known to be integer shaped.
#define REDUCT_ATOM_FLAG_FLOAT (1 << 1) ///< Atom is known to be float shaped.
#define REDUCT_ATOM_FLAG_INTRINSIC (1 << 2)    ///< Atom is known to represent an intrinsic.
#define REDUCT_ATOM_FLAG_NATIVE (1 << 3)       ///< Atom is known to represent a native function.
#define REDUCT_ATOM_FLAG_NUMBER_CHECKED (1 << 4) ///< Atom has been checked for integer/float shaping.
#define REDUCT_ATOM_FLAG_SUBSTR (1 << 5) ///< Atom is a substring of another atom.
#define REDUCT_ATOM_FLAG_LARGE (1 << 6) ///< Atom has an allocated buffer.

#define REDUCT_ATOM_TOMBSTONE ((reduct_atom_t*)(uintptr_t)1) ///< Tombstone value for the atom map.

/**
 * @brief Atom structure.
 * @struct reduct_atom_t
 */
typedef struct reduct_atom
{
    reduct_uint32_t length; ///< The length of the string (must be first, check the `reduct_item_t` structure).
    reduct_uint32_t hash;   ///< The hash of the string.
    reduct_uint32_t index; ///< The index within the atom map.
    reduct_intrinsic_t intrinsic;      ///< Cached intrinsic, item must have `REDUCT_ATOM_FLAG_INTRINSIC`.
    reduct_atom_flags_t flags; ///< Atom flags.
    struct
    {
        union
        {
            char small[REDUCT_ATOM_SMALL_MAX]; ///< Small string data, atom must not have `REDUCT_ATOM_FLAG_LARGE | REDUCT_ATOM_FLAG_SUBSTR`.
            struct reduct_atom* parent; ///< The atom that this atom is a substring of, atom must have `REDUCT_ATOM_FLAG_SUBSTR`.
        };
        char* string; ///< Pointer to the data, atom must have not have `REDUCT_ATOM_FLAG_NODES`.
    };
    union {
        reduct_int64_t integerValue; ///< Pre-computed integer value, atom must have `REDUCT_ATOM_FLAG_INTEGER`.
        reduct_float_t floatValue;   ///< Pre-computed float value, atom must have `REDUCT_ATOM_FLAG_FLOAT`.
        reduct_native_fn native;     ///< Cached native function, atom must have `REDUCT_ATOM_FLAG_NATIVE`.
    };
} reduct_atom_t;

#define REDUCT_FNV_PRIME 16777619U    ///< FNV-1a 32-bit prime.
#define REDUCT_FNV_OFFSET 2166136261U ///< FNV-1a 32-bit offset basis.

/**
 * @brief Hash a string.
 *
 * @param str The string to hash.
 * @param len The length of the string.
 * @return The hash of the string.
 */
static inline REDUCT_ALWAYS_INLINE reduct_uint32_t reduct_hash(const char* str, reduct_size_t len)
{
    reduct_uint32_t hash = REDUCT_FNV_OFFSET;
    for (reduct_size_t i = 0; i < len; i++)
    {
        hash ^= (unsigned char)str[i];
        hash *= REDUCT_FNV_PRIME;
    }
    return hash;
}

/**
 * @brief Initialize an atom.
 *
 * @param atom Pointer to the atom to initialize.
 */
static inline void reduct_atom_init(reduct_atom_t* atom)
{
    REDUCT_ASSERT(atom != REDUCT_NULL);
    atom->length = 0;
    atom->hash = 0;
    atom->index = REDUCT_ATOM_INDEX_NONE;
    atom->intrinsic = REDUCT_INTRINSIC_NONE;
    atom->flags = 0;
    atom->string = REDUCT_NULL;
    atom->integerValue = 0;
}

/**
 * @brief Deinitialize an atom.
 *
 * @param reduct Pointer to the Reduct structure.
 * @param atom Pointer to the atom to deinitialize.
 */
REDUCT_API void reduct_atom_deinit(struct reduct* reduct, reduct_atom_t* atom);

/**
 * @brief Check if an atom is equal to a string.
 *
 * @param atom Pointer to the atom.
 * @param str The string to compare.
 * @param len The length of the string.
 * @return `REDUCT_TRUE` if the atom is equal to the string, `REDUCT_FALSE` otherwise.
 */
REDUCT_API reduct_bool_t reduct_atom_is_equal(reduct_atom_t* atom, const char* str,
    reduct_size_t len);

/**
 * @brief Create an atom with a reserved size.
 *
 * @param reduct Pointer to the Reduct structure.
 * @param data The raw buffer to create the atom from.
 * @param len The length of the buffer.
 * @return A pointer to the atom.
 */
REDUCT_API reduct_atom_t* reduct_atom_new(struct reduct* reduct, reduct_size_t len);

/**
 * @brief Create an atom from a integer value.
 *
 * @param reduct Pointer to the Reduct structure.
 * @param value The integer value.
 * @return A pointer to the atom.
 */
REDUCT_API reduct_atom_t* reduct_atom_new_int(struct reduct* reduct, reduct_int64_t value);

/**
 * @brief Create an atom from a float value.
 *
 * @param reduct Pointer to the Reduct structure.
 * @param value The float value.
 * @return A pointer to the atom.
 */
REDUCT_API reduct_atom_t* reduct_atom_new_float(struct reduct* reduct, reduct_float_t value);

/**
 * @brief Intern an existing atom into the Reduct structure.
 *
 * @param reduct Pointer to the Reduct structure.
 * @param atom Pointer to the atom to intern.
 */
REDUCT_API void reduct_atom_intern(struct reduct* reduct, reduct_atom_t* atom);

/**
 * @brief Lookup an atom in the Reduct structure.
 *
 * Will create a new atom if it does not exist.
 *
 * @param reduct Pointer to the Reduct structure.
 * @param str The string to lookup.
 * @param len The length of the string.
 * @param flags Lookup flags to alter the interning behavior.
 * @return A pointer to the atom.
 */
REDUCT_API reduct_atom_t* reduct_atom_lookup(struct reduct* reduct, const char* str, reduct_size_t len,
    reduct_atom_lookup_flags_t flags);

/**
 * @brief Check if an atom is a number.
 *
 * @param atom Pointer to the atom.
 */
REDUCT_API void reduct_atom_check_number(reduct_atom_t* atom);

/**
 * @brief Check if an atom is an intrinsic.
 *
 * @param atom Pointer to the atom.
 * @return `REDUCT_TRUE` if the atom is an intrinsic, `REDUCT_FALSE` otherwise.
 */
static inline REDUCT_ALWAYS_INLINE reduct_bool_t reduct_atom_is_intrinsic(reduct_atom_t* atom)
{
    return (atom->flags & REDUCT_ATOM_FLAG_INTRINSIC) != 0;
}

/**
 * @brief Check if an atom is a native function.
 *
 * @param atom Pointer to the atom.
 * @return `REDUCT_TRUE` if the atom is a native function, `REDUCT_FALSE` otherwise.
 */
static inline REDUCT_ALWAYS_INLINE reduct_bool_t reduct_atom_is_native(reduct_atom_t* atom)
{
    return (atom->flags & REDUCT_ATOM_FLAG_NATIVE) != 0;
}

/**
 * @brief Check if an atom is integer-shaped.
 *
 * @param atom Pointer to the atom.
 * @return `REDUCT_TRUE` if the atom is an integer, `REDUCT_FALSE` otherwise.
 */
static inline REDUCT_ALWAYS_INLINE reduct_bool_t reduct_atom_is_int(reduct_atom_t* atom)
{
    if (REDUCT_UNLIKELY(!(atom->flags & REDUCT_ATOM_FLAG_NUMBER_CHECKED)))
    {
        reduct_atom_check_number(atom);
    }
    return (atom->flags & REDUCT_ATOM_FLAG_INTEGER) != 0;
}

/**
 * @brief Check if an atom is float-shaped.
 *
 * @param atom Pointer to the atom.
 * @return `REDUCT_TRUE` if the atom is a float, `REDUCT_FALSE` otherwise.
 */
static inline REDUCT_ALWAYS_INLINE reduct_bool_t reduct_atom_is_float(reduct_atom_t* atom)
{
    if (REDUCT_UNLIKELY(!(atom->flags & REDUCT_ATOM_FLAG_NUMBER_CHECKED)))
    {
        reduct_atom_check_number(atom);
    }
    return (atom->flags & REDUCT_ATOM_FLAG_FLOAT) != 0;
}

/**
 * @brief Check if an atom is integer-shaped or float-shaped.
 *
 * @param atom Pointer to the atom.
 * @return `REDUCT_TRUE` if the atom is a number, `REDUCT_FALSE` otherwise.
 */
static inline REDUCT_ALWAYS_INLINE reduct_bool_t reduct_atom_is_number(reduct_atom_t* atom)
{
    if (REDUCT_UNLIKELY(!(atom->flags & REDUCT_ATOM_FLAG_NUMBER_CHECKED)))
    {
        reduct_atom_check_number(atom);
    }
    return (atom->flags & (REDUCT_ATOM_FLAG_INTEGER | REDUCT_ATOM_FLAG_FLOAT)) != 0;
}

/**
 * @brief Get the integer value of an atom.
 *
 * @param atom Pointer to the atom.
 * @return The integer value.
 */
static inline REDUCT_ALWAYS_INLINE reduct_int64_t reduct_atom_get_int(reduct_atom_t* atom)
{
    REDUCT_ASSERT(reduct_atom_is_number(atom));

    if (reduct_atom_is_int(atom))
    {
        return atom->integerValue;
    }
    else
    {
        return (reduct_int64_t)atom->floatValue;
    }
}

/**
 * @brief Get the float value of an atom.
 *
 * @param atom Pointer to the atom.
 * @return The float value.
 */
static inline REDUCT_ALWAYS_INLINE reduct_float_t reduct_atom_get_float(reduct_atom_t* atom)
{
    REDUCT_ASSERT(reduct_atom_is_number(atom));

    if (reduct_atom_is_float(atom))
    {
        return atom->floatValue;
    }
    else
    {
        return (reduct_float_t)atom->integerValue;
    }
}

/**
 * @brief Create a substring of an existing atom.
 *
 * @param reduct Pointer to the Reduct structure.
 * @param atom Pointer to the source atom.
 * @param start The starting index.
 * @param len The length of the substring.
 * @return A pointer to the new atom.
 */
REDUCT_API reduct_atom_t* reduct_atom_substring(struct reduct* reduct, reduct_atom_t* atom, reduct_size_t start,
    reduct_size_t len);

/**
 * @brief Create a new atom by copying data directly into it.
 *
 * The atom is NOT interned and its hash is set to 0, avoiding
 * the overhead of hash computation and map lookup.
 *
 * @param reduct Pointer to the Reduct structure.
 * @param data The data to copy.
 * @param len The length of the data.
 * @return A pointer to the new atom.
 */
static inline REDUCT_ALWAYS_INLINE reduct_atom_t* reduct_atom_new_copy(struct reduct* reduct, const char* data,
    reduct_size_t len)
{
    reduct_atom_t* atom = reduct_atom_new(reduct, len);
    REDUCT_MEMCPY(atom->string, data, len);
    return atom;
}

/** @} */

#endif
