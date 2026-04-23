#ifndef SCON_EVAL_H
#define SCON_EVAL_H 1

struct scon_closure;

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
 * @brief SCON evaluation frame structure.
 * @struct scon_eval_frame_t
 */
typedef struct scon_eval_frame
{
    struct scon_closure* closure; ///< The closure being evaluated.
    scon_inst_t* ip;              ///< The current instruction pointer.
    scon_uint32_t base;           ///< The base register, where the functions registers start.
    scon_uint32_t prevRegCount;   ///< The previous register count to restore upon return.
} scon_eval_frame_t;

/**
 * @brief SCON evaluation state structure.
 * @struct scon_eval_state_t
 */
typedef struct scon_eval_state
{
    scon_eval_frame_t* frames;
    scon_uint32_t frameCount;
    scon_uint32_t frameCapacity;
    scon_handle_t* regs;
    scon_uint32_t regCount;
    scon_uint32_t regCapacity;
} scon_eval_state_t;

/**
 * @brief Deinitialize an evaluation state structure.
 *
 * @param state The evaluation state to deinitialize.
 */
SCON_API void scon_eval_state_deinit(scon_eval_state_t* state);

/**
 * @brief Evaluates a compiled SCON function.
 *
 * @param scon The SCON instance.
 * @param function The function to evaluate.
 * @return The result of the evaluation as a SCON handle.
 */
SCON_API scon_handle_t scon_eval(scon_t* scon, scon_function_t* function);

/**
 * @brief Calls a SCON callable (closure or native) with arguments.
 *
 * @param scon The SCON instance.
 * @param callable The callable item handle.
 * @param argc The number of arguments.
 * @param argv Pointer to the arguments array.
 * @return The result of the call.
 */
SCON_API scon_handle_t scon_eval_call(scon_t* scon, scon_handle_t callable, scon_size_t argc, scon_handle_t* argv);

/** @} */

#endif