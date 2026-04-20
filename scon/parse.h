#ifndef SCON_PARSE_H
#define SCON_PARSE_H 1

#include "core.h"
#include "handle.h"

/**
 * @brief SCON parser.
 * @defgroup parse Parse
 * @file parse.h
 *
 * @{
 */

/**
 * @brief Parse a SCON string.
 *
 * @warning The jump buffer must have been set using `SCON_CATCH` before calling this function.
 *
 * @param scon The SCON structure.
 * @param str The SCON string to parse, SCON will take ownership of this string.
 * @param len The length of the input string.
 * @param path The path to the file being parsed (for error reporting).
 * @return The root item of the parsed expression, will be retained by the GC.
 */
SCON_API scon_handle_t scon_parse(scon_t* scon, const char* str, scon_size_t len, const char* path);

/**
 * @brief Parse a SCON file.
 *
 * @warning The jump buffer must have been set using `SCON_CATCH` before calling this function.
 *
 * @param scon The SCON structure.
 * @param path The path to the file to parse.
 * @return The root item of the parsed expression, will be retained by the GC.
 */
SCON_API scon_handle_t scon_parse_file(scon_t* scon, const char* path);

/** @} */

#endif
