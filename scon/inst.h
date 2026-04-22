#ifndef SCON_INST_H
#define SCON_INST_H 1

#include "defs.h"

/**
 * @brief SCON bytecode instruction format.
 * @defgroup inst Instruction Format
 * @file inst.h
 *
 * Instructions are 32-bit words
 * with the following formats:
 *
 * - iABC:  [ Opcode:6 | A:8 | B:8 | C:10 ]
 * - iAsBx: [ Opcode:6 | A:8 | sBx:18 ]
 *
 * Fields:
 * - A: Usually the target/destination register.
 * - B: Usually the first operand (register).
 * - C: Usually the second operand (register or constant).
 * - sBx: Signed offsets for jumps.
 *
 * To determine if the C field is a register or a constant the `scon_mode_t` flags are used to modify the opcode.
 *
 * @note The reason we avoid formats such as iABx, used within Lua, is that even if it increases the maximum constant
 * capacity it means that operations such as `SCON_OPCODE_EQUAL` always need to act on registers, which introduces
 * unnecessary `MOV` instructions to load constants into registers before they can be compared.
 *
 * @{
 */

/**
 * @brief SCON opcode mode enumeration.
 * @enum scon_mode_t
 */
typedef enum
{
    SCON_MODE_NONE = -1,      ///< Invalid mode.
    SCON_MODE_TARGET = -2,    ///< Compilation target hint mode.
    SCON_MODE_REG = 0,        ///< Register operand mode.
    SCON_MODE_CONST = 1 << 5, ///< Constant operand mode.
} scon_mode_t;

/**
 * @brief SCON opcode enumeration.
 * @enum scon_opcode_t
 */
typedef enum
{
    SCON_OPCODE_NONE,
    SCON_OPCODE_LIST,    ///< (A) Create a new list and store it in R(A).
    SCON_OPCODE_JMP,     ///< (sBx) Unconditional jump by relative offset sBx.
    SCON_OPCODE_JMPF,    ///< (A, sBx) Jump by sBx if R(A) is falsy.
    SCON_OPCODE_JMPT,    ///< (A, sBx) Jump by sBx if R(A) is truthy.
    SCON_OPCODE_JEQ,     ///< (A, C) Skip the next instruction if R(A) == R/K(C), else continue.
    SCON_OPCODE_CALL,    ///< (A, B, C) Call callable in R/K(C) with B args starting from R(A). Result in R(A).
    SCON_OPCODE_MOV,     ///< (A, C) Move value in R/K(C) to R(A).
    SCON_OPCODE_RET,     ///< (C) Return value in R/K(C).
    SCON_OPCODE_APPEND,  ///< (A, C) Append value in R/K(C) to the back of the list in R(A).
    SCON_OPCODE_EQ,      ///< (A, B, C) If R(B) == R/K(C) store true in R(A), else false.
    SCON_OPCODE_NEQ,     ///< (A, B, C) If R(B) != R/K(C) store true in R(A), else false.
    SCON_OPCODE_SEQ,     ///< (A, B, C) If R(B) === R/K(C) store true in R(A), else false.
    SCON_OPCODE_SNEQ,    ///< (A, B, C) If R(B) !== R/K(C) store true in R(A), else false.
    SCON_OPCODE_LT,      ///< (A, B, C) If R(B) < R/K(C) store true in R(A), else false.
    SCON_OPCODE_LE,      ///< (A, B, C) If R(B) <= R/K(C) store true in R(A), else false.
    SCON_OPCODE_GT,      ///< (A, B, C) If R(B) > R/K(C) store true in R(A), else false.
    SCON_OPCODE_GE,      ///< (A, B, C) If R(B) >= R/K(C) store true in R(A), else false.
    SCON_OPCODE_ADD,     ///< (A, B, C) R(A) = R(B) + R/K(C)
    SCON_OPCODE_SUB,     ///< (A, B, C) R(A) = R(B) - R/K(C)
    SCON_OPCODE_MUL,     ///< (A, B, C) R(A) = R(B) * R/K(C)
    SCON_OPCODE_DIV,     ///< (A, B, C) R(A) = R(B) / R/K(C)
    SCON_OPCODE_MOD,     ///< (A, B, C) R(A) = R(B) % R/K(C)
    SCON_OPCODE_BAND,    ///< (A, B, C) R(A) = R(B) & R/K(C)
    SCON_OPCODE_BOR,     ///< (A, B, C) R(A) = R(B) | R/K(C)
    SCON_OPCODE_BXOR,    ///< (A, B, C) R(A) = R(B) ^ R/K(C)
    SCON_OPCODE_BNOT,    ///< (A, C) R(A) = ~R/K(C)
    SCON_OPCODE_SHL,     ///< (A, B, C) R(A) = R(B) << R/K(C)
    SCON_OPCODE_SHR,     ///< (A, B, C) R(A) = R(B) >> R/K(C)
    SCON_OPCODE_CLOSURE, ///< (A, C) Wrap the function prototype in K(C) in a closure and store in R(A).
    SCON_OPCODE_CAPTURE, ///< (A, B, C) Capture R/K(C) into constant slot B in closure R(A).
    SCON_OPCODE_TAILCALL,///< (A, B, C) Tail call callable in R/K(C) with B args starting from R(A).
} scon_opcode_t;

/**
 * @brief SCON register type.
 */
typedef scon_uint16_t scon_reg_t;

/**
 * @brief SCON instruction type.
 */
typedef scon_uint32_t scon_inst_t;

#define SCON_INST_WIDTH_OPCODE 6                                    ///< Opcode width in bits.
#define SCON_INST_WIDTH_A 8                                         ///< A operand width in bits.
#define SCON_INST_WIDTH_B 8                                         ///< B operand width in bits.
#define SCON_INST_WIDTH_C 10                                        ///< C operand width in bits.
#define SCON_INST_WIDTH_SBX (SCON_INST_WIDTH_B + SCON_INST_WIDTH_C) ///< SBx operand width in bits.

/**
 * @brief The max number of registers per function frame.
 */
#define SCON_REGISTER_MAX (1 << SCON_INST_WIDTH_A)
/**
 * @brief The max number of constants per function.
 */
#define SCON_CONSTANT_MAX (1 << SCON_INST_WIDTH_C)

#define SCON_INST_POS_OPCODE 0                                          ///< Opcode position in bits.
#define SCON_INST_POS_A (SCON_INST_POS_OPCODE + SCON_INST_WIDTH_OPCODE) ///< A operand position in bits.
#define SCON_INST_POS_B (SCON_INST_POS_A + SCON_INST_WIDTH_A)           ///< B operand position in bits.
#define SCON_INST_POS_C (SCON_INST_POS_B + SCON_INST_WIDTH_B)           ///< C operand position in bits.

#define SCON_INST_MASK_OPCODE ((1 << SCON_INST_WIDTH_OPCODE) - 1) ///< Opcode mask.
#define SCON_INST_MASK_A ((1 << SCON_INST_WIDTH_A) - 1)           ///< A operand mask.
#define SCON_INST_MASK_B ((1 << SCON_INST_WIDTH_B) - 1)           ///< B operand mask.
#define SCON_INST_MASK_C ((1 << SCON_INST_WIDTH_C) - 1)           ///< C operand mask.
#define SCON_INST_MASK_SBX ((1 << SCON_INST_WIDTH_SBX) - 1)       ///< SBx operand mask.

/**
 * @brief Create an instruction with opcode, A, B, and C operands.
 *
 * @param _op Opcode operand.
 * @param _a A operand.
 * @param _b B operand.
 * @param _c C operand.
 */
#define SCON_INST_MAKE_ABC(_op, _a, _b, _c) \
    ((((scon_inst_t)(_op)) & SCON_INST_MASK_OPCODE) << SCON_INST_POS_OPCODE | \
        (((scon_inst_t)(_a)) & SCON_INST_MASK_A) << SCON_INST_POS_A | \
        (((scon_inst_t)(_b)) & SCON_INST_MASK_B) << SCON_INST_POS_B | \
        (((scon_inst_t)(_c)) & SCON_INST_MASK_C) << SCON_INST_POS_C)

/**
 * @brief Create an instruction with opcode and A operands, and SBx B operand.
 *
 * @param _op Opcode operand.
 * @param _a A operand.
 * @param _sbx SBx operand.
 */
#define SCON_INST_MAKE_ASBX(_op, _a, _sbx) \
    ((((scon_inst_t)(_op)) & SCON_INST_MASK_OPCODE) << SCON_INST_POS_OPCODE | \
        (((scon_inst_t)(_a)) & SCON_INST_MASK_A) << SCON_INST_POS_A | \
        (((scon_inst_t)(_sbx)) & SCON_INST_MASK_SBX) << SCON_INST_POS_B)

/**
 * @brief Get the opcode from an instruction.
 *
 * @param _inst Instruction.
 */
#define SCON_INST_GET_OP(_inst) (((_inst) >> SCON_INST_POS_OPCODE) & SCON_INST_MASK_OPCODE)

/**
 * @brief Get the opcode base (without `scon_mode_t`) from an instruction. Mask clears the `SCON_MODE_CONST` bit.
 *
 * @param _inst Instruction.
 */
#define SCON_INST_GET_OP_BASE(_inst) (((_inst) >> SCON_INST_POS_OPCODE) & (SCON_INST_MASK_OPCODE & ~SCON_MODE_CONST))

/**
 * @brief Get the A operand from an instruction.
 *
 * @param _inst Instruction.
 */
#define SCON_INST_GET_A(_inst) (((_inst) >> SCON_INST_POS_A) & SCON_INST_MASK_A)

/**
 * @brief Get the B operand from an instruction.
 *
 * @param _inst Instruction.
 */
#define SCON_INST_GET_B(_inst) (((_inst) >> SCON_INST_POS_B) & SCON_INST_MASK_B)

/**
 * @brief Get the C operand from an instruction.
 *
 * @param _inst Instruction.
 */
#define SCON_INST_GET_C(_inst) (((_inst) >> SCON_INST_POS_C) & SCON_INST_MASK_C)

/**
 * @brief Get the SBX operand from an instruction.
 *
 * @param _inst Instruction.
 */
#define SCON_INST_GET_SBX(_inst) \
    ((scon_int32_t)(((_inst) >> SCON_INST_POS_B) & SCON_INST_MASK_SBX) << (32 - SCON_INST_WIDTH_SBX) >> \
        (32 - SCON_INST_WIDTH_SBX))

/**
 * @brief Set the A operand in an instruction.
 *
 * @param _inst Instruction.
 * @param _a A operand value.
 */
#define SCON_INST_SET_A(_inst, _a) \
    (((_inst) & ~(SCON_INST_MASK_A << SCON_INST_POS_A)) | (((_a) & SCON_INST_MASK_A) << SCON_INST_POS_A))

/**
 * @brief Set the SBX operand in an instruction.
 *
 * @param _inst Instruction.
 * @param _sbx SBX operand value.
 */
#define SCON_INST_SET_SBX(_inst, _sbx) \
    (((_inst) & ~(SCON_INST_MASK_SBX << SCON_INST_POS_B)) | (((_sbx) & SCON_INST_MASK_SBX) << SCON_INST_POS_B))

#endif
