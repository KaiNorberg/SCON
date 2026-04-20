#ifndef SCON_BUILTIN_H
#define SCON_BUILTIN_H 1

#include "defs.h"
#include "item.h"

struct scon;

/**
 * @brief SCON built-in library registration and operations.
 * @defgroup builtin Builtins
 * @file builtin.h
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
    SCON_BUILTIN_ERROR = (1 << 0),
    SCON_BUILTIN_HIGHER_ORDER = (1 << 1),
    SCON_BUILTIN_SYSTEM = (1 << 2),
    SCON_BUILTIN_ALL = 0xFFFF,
} scon_builtins_t;

/**
 * @brief Register a built-in library to the SCON instance.
 *
 * @param scon The SCON structure.
 */
SCON_API void scon_builtin_register(struct scon* scon, scon_builtins_t builtins);

/** @} */

#endif
