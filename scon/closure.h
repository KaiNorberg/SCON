#ifndef SCON_CLOSURE_H
#define SCON_CLOSURE_H 1

struct scon_item;

#include "defs.h"
#include "function.h"

/**
 * @brief SCON closure
 * @defgroup closure SCON closure
 * @file closure.h
 *
 * A closure is a function instance that has captured variables from its enclosing scope.
 *
 * @{
 */

/**
 * @brief SCON closure structure.
 * @struct scon_closure_t
 */
typedef struct scon_closure
{
    scon_function_t* function; ///< Pointer to the prototype function item.            ///< Pointer to upvalues array.
    scon_handle_t* constants;  ///< The array of constant slots forming the constant template.
} scon_closure_t;

/**
 * @brief Deinitialize a closure structure.
 *
 * @param closure The closure to deinitialize.
 */
SCON_API void scon_closure_deinit(scon_closure_t* closure);

/**
 * @brief Allocate a new closure.
 *
 * @param scon The SCON structure.
 * @param function The prototype function item.
 * @return A pointer to the newly allocated closure.
 */
SCON_API scon_closure_t* scon_closure_new(struct scon* scon, scon_function_t* function);

/** @} */

#endif