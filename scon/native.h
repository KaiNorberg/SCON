#ifndef SCON_NATIVE_H
#define SCON_NATIVE_H 1

#include "core.h"
#include "defs.h"
#include "handle.h"

/**
 * @brief SCON native function registration.
 * @defgroup native Native Functions
 * @file native.h
 *
 * A "native" is a C function that can be called at runtime.
 *
 * @{
 */

/**
 * @brief Native function pointer type.
 *
 * @param scon The SCON structure.
 * @param argc The number of arguments.
 * @param argv The array of arguments.
 * @return The result of the function.
 */
typedef scon_handle_t (*scon_native_fn)(scon_t* scon, scon_size_t argc, scon_handle_t* argv);

/**
 * @brief Native function definition structure.
 */
typedef struct
{
    const char* name;
    scon_native_fn fn;
} scon_native_t;

/**
 * @brief Register native functions.
 *
 * @param scon The SCON structure.
 * @param array An array of native function definitions.
 * @param count The number of functions in the array.
 */
SCON_API void scon_native_register(scon_t* scon, scon_native_t* array, scon_size_t count);

/** @} */

#endif
