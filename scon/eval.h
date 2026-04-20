#ifndef SCON_EVAL_H
#define SCON_EVAL_H 1

/**
 * @brief SCON virtual machine evaluation.
 * @defgroup eval Evaluation
 * @file eval.h
 *
 * @{
 */

#include "core.h"
#include "function.h"
#include "handle.h"

#define SCON_EVAL_REGS_INITIAL 1024    ///< The initial amount of registers.
#define SCON_EVAL_REGS_GROWTH_FACTOR 2 ///< The growth factor of the registers array.

#define SCON_EVAL_FRAMES_INITIAL 1024    ///< The initial size of the frames array.
#define SCON_EVAL_FRAMES_GROWTH_FACTOR 2 ///< The growth factor of the frames array.

/**
 * @brief Evaluates a compiled SCON function.
 *
 * @param scon The SCON instance.
 * @param function The function to evaluate.
 * @return The result of the evaluation as a SCON handle.
 */
SCON_API scon_handle_t scon_eval(scon_t* scon, scon_function_t* function);

/** @} */

#endif