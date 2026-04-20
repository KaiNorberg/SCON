#ifndef SCON_CLOSURE_H
#define SCON_CLOSURE_H 1

struct scon_item;

#include "defs.h"

/**
 * @brief SCON closure structure.
 * @struct scon_closure_t
 */
typedef struct scon_closure
{
    struct scon_item* function;  ///< Pointer to the prototype function item.            ///< Pointer to upvalues array.
    struct scon_item* constants; ///< The array of constant slots forming the constant template.
    scon_uint16_t constantCount; ///< Number of constants.
    scon_uint16_t constantCapacity; ///< Capacity of the constant array.
} scon_closure_t;

#endif