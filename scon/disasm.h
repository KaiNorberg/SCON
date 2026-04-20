#ifndef SCON_DISASM_H
#define SCON_DISASM_H 1

#include "compile.h"
#include "core.h"

/**
 * @brief SCON bytecode disassembly.
 * @defgroup disasm Disassembly
 * @file disasm.h
 *
 * @{
 */

/**
 * @brief Disassembles a compiled function.
 *
 * @param scon The SCON structure.
 * @param function The compiled function to disassemble.
 * @param out The output file stream to write the disassembly to.
 */
SCON_API void scon_disasm(scon_t* scon, scon_function_t* function, scon_file_t out);

/** @} */

#endif
