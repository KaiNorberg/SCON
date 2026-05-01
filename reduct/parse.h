#ifndef REDUCT_PARSE_H
#define REDUCT_PARSE_H 1

#include "core.h"
#include "handle.h"

/**
 * @file parse.h
 * @brief Parser.
 * @defgroup parse Parse
 *
 * @{
 */

/**
 * @brief Parse a Reduct string.
 *
 * @warning The jump buffer must have been set using `REDUCT_CATCH` before calling this function.
 *
 * @param reduct Pointer to the Reduct structure.
 * @param str The Reduct string to parse, Reduct will take ownership of this string.
 * @param len The length of the input string.
 * @param path The path to the file being parsed (for error reporting).
 * @return The root item of the parsed expression, will be retained by the GC.
 */
REDUCT_API reduct_handle_t reduct_parse(reduct_t* reduct, const char* str, reduct_size_t len, const char* path);

/**
 * @brief Parse a Reduct file.
 *
 * @warning The jump buffer must have been set using `REDUCT_CATCH` before calling this function.
 *
 * @param reduct Pointer to the Reduct structure.
 * @param path The path to the file to parse.
 * @return The root item of the parsed expression, will be retained by the GC.
 */
REDUCT_API reduct_handle_t reduct_parse_file(reduct_t* reduct, const char* path);

/** @} */

#endif
