#ifndef SCON_ATOM_H
#define SCON_ATOM_H 1

#include "defs.h"
#include "keyword.h"
#include "native.h"

struct scon;

/**
 * @brief SCON atom representation and operations.
 * @defgroup atom Atoms
 * @file atom.h
 *
 * Atoms represent all strings within a SCON expression, as such it also represents anything that a string can be,
 * including integers, floats and keywords.
 *
 * ## Interning
 *
 * To improve performance, we "intern" all atoms. Meaning that instead of needing to use `strcmp()` or similar to
 * compare atoms, we store all atoms in a hash map.
 *
 * When a new string is encountered, we look it up in the map. As such, any instance of the same string will always be
 * represented by the same `scon_atom_t` structure. Turning a string comparison into a pointer comparison and avoiding
 * redundant parsing of numeric or keyword values.
 *
 * @see [Wikipedia String Interning](https://en.wikipedia.org/wiki/String_interning)
 *
 * @{
 */

#define SCON_ATOM_SMALL_MAX 16 ///< The maximum length of a small atom.

/**
 * @brief SCON atom structure.
 * @struct scon_atom_t
 */
typedef struct scon_atom
{
    scon_uint32_t length;            ///< The length of the string (must be first, check the `scon_item_t` structure).
    scon_uint32_t hash;              ///< The hash of the string.
    char small[SCON_ATOM_SMALL_MAX]; ///< The small string buffer.
    char* string;                    ///< Pointer to the string.
    union {
        scon_int64_t integerValue; ///< Pre-computed integer value, item must have `SCON_ITEM_FLAG_INT_SHAPED`.
        scon_float_t floatValue;   ///< Pre-computed float value, item must have `SCON_ITEM_FLAG_FLOAT_SHAPED`.
        scon_keyword_t keyword;    ///< Cached keyword, item must have `SCON_ITEM_FLAG_KEYWORD`.
        scon_native_fn native;     ///< Native function, item must have `SCON_ITEM_FLAG_NATIVE`.
    };
    struct scon_atom* next; ///< Pointer to the next atom in the hash map.
} scon_atom_t;

#define SCON_FNV_OFFSET 16777619U  ///< FNV offset.
#define SCON_FNV_PRIME 2166136261U ///< FNV prime.

/**
 * @brief Hash a string.
 *
 * @param str The string to hash.
 * @param len The length of the string.
 * @return The hash of the string.
 */
static inline scon_uint32_t scon_hash(const char* str, scon_size_t len)
{
    scon_uint32_t hash = SCON_FNV_OFFSET;
    for (scon_size_t i = 0; i < len; i++)
    {
        hash ^= (unsigned char)str[i];
        hash *= SCON_FNV_PRIME;
    }
    return hash;
}

/**
 * @brief Initialize an atom.
 *
 * @param scon Pointer to the SCON structure.
 * @param atom Pointer to the atom to initialize.
 */
static inline void scon_atom_init(struct scon* scon, scon_atom_t* atom)
{
    atom->length = 0;
    atom->next = SCON_NULL;
    atom->hash = 0;
}

/**
 * @brief Deinitialize an atom.
 *
 * @param scon Pointer to the SCON structure.
 * @param atom Pointer to the atom to deinitialize.
 */
SCON_API void scon_atom_deinit(struct scon* scon, scon_atom_t* atom);

/**
 * @brief Check if an atom is equal to a string.
 *
 * @param atom Pointer to the atom.
 * @param str The string to compare.
 * @param len The length of the string.
 * @return `SCON_TRUE` if the atom is equal to the string, `SCON_FALSE` otherwise.
 */
SCON_API scon_bool_t scon_atom_is_equal(scon_atom_t* atom, const char* str, scon_size_t len);

/**
 * @brief Lookup an atom by integer value.
 *
 * Will create a new atom if it does not exist.
 *
 * @param scon Pointer to the SCON structure.
 * @param value The integer value.
 * @return A pointer to the atom.
 */
SCON_API scon_atom_t* scon_atom_lookup_int(struct scon* scon, scon_int64_t value);

/**
 * @brief Lookup an atom by float value.
 *
 * Will create a new atom if it does not exist.
 *
 * @param scon Pointer to the SCON structure.
 * @param value The float value.
 * @return A pointer to the atom.
 */
SCON_API scon_atom_t* scon_atom_lookup_float(struct scon* scon, scon_float_t value);

/**
 * @brief Lookup an atom in the SCON structure.
 *
 * Will create a new atom if it does not exist.
 *
 * @param scon Pointer to the SCON structure.
 * @param str The string to lookup.
 * @param len The length of the string.
 * @return A pointer to the atom.
 */
SCON_API scon_atom_t* scon_atom_lookup(struct scon* scon, const char* str, scon_size_t len);

/**
 * @brief Normalize an atom, determining its shape and parsing escape sequences.
 *
 * @warning Should only be called on atoms stored in a `scon_item_t`.
 *
 * @param scon Pointer to the SCON structure.
 * @param atom Pointer to the atom to normalize.
 */
SCON_API void scon_atom_normalize(struct scon* scon, scon_atom_t* atom);

/** @} */

#endif