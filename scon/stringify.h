#ifndef SCON_STRINGIFY_H
#define SCON_STRINGIFY_H 1

#include "core.h"
#include "handle.h"

/**
 * @brief SCON stringification.
 * @defgroup stringify Stringification
 * @file stringify.h
 *
 * @{
 */

/**
 * @brief Converts a SCON handle to its string representation.
 *
 * @param scon The SCON structure.
 * @param handle The handle to stringify.
 * @param buffer The destination buffer.
 * @param size The size of the destination buffer.
 * @return The number of characters that would have been written if the buffer was large enough, excluding the null
 * terminator.
 */
SCON_API scon_size_t scon_stringify(scon_t* scon, scon_handle_t* handle, char* buffer, scon_size_t size);

/** @} */

#endif
