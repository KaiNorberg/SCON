#ifndef SCON_STDLIB_H
#define SCON_STDLIB_H 1

#include "defs.h"
#include "item.h"

struct scon;

/**
 * @brief SCON built-in library registration and operations.
 * @defgroup stdlib Stdlib
 * @file stdlib.h
 *
 * Built-in libraries provide a set of pre-defined native functions for use in SCON expressions.
 *
 * @see native
 *
 * @{
 */

/**
 * @brief Built-in library sets.
 */
typedef enum
{
    SCON_STDLIB_ERROR = (1 << 0),
    SCON_STDLIB_HIGHER_ORDER = (1 << 1),
    SCON_STDLIB_SYSTEM = (1 << 2),
    SCON_STDLIB_ALL = 0xFFFF,
} scon_stdlib_sets_t;

/**
 * @brief Register a set from the standard library to the SCON instance.
 *
 * @param scon The SCON structure.
 * @param sets The sets to register.
 */
SCON_API void scon_stdlib_register(struct scon* scon, scon_stdlib_sets_t sets);

/** @} */

#endif
