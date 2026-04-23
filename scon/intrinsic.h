#ifndef SCON_INTRINSIC_H
#define SCON_INTRINSIC_H 1

#include "defs.h"

struct scon;
struct scon_compiler;
struct scon_item;
struct scon_expr;

/**
 * @brief SCON intrinsic management.
 * @defgroup intrinsic Intrinsics
 * @file intrinsic.h
 *
 * A intrinsic is an atom recognized by the bytecode compiler as a special form or built-in operator, resulting in it
 * emitting specific bytecode instructions for each intrinsic.
 *
 * This is in contrast to a "native" which will be invoked as a standard function call at runtime.
 *
 * @{
 */

/**
 * @brief SCON intrinsic types.
 */
typedef enum
{
    SCON_INTRINSIC_NONE = 0, ///< None
    SCON_INTRINSIC_QUOTE,    ///< Quote
    SCON_INTRINSIC_LIST,     ///< List
    SCON_INTRINSIC_DO,       ///< Do
    SCON_INTRINSIC_LAMBDA,   ///< Lambda
    SCON_INTRINSIC_DEF,      ///< Def
    SCON_INTRINSIC_LET,      ///< Let
    SCON_INTRINSIC_IF,       ///< If
    SCON_INTRINSIC_WHEN,     ///< When
    SCON_INTRINSIC_UNLESS,   ///< Unless
    SCON_INTRINSIC_COND,     ///< Cond
    SCON_INTRINSIC_AND,      ///< And
    SCON_INTRINSIC_OR,       ///< Or
    SCON_INTRINSIC_NOT,      ///< Not
    SCON_INTRINSIC_ADD,  ///< Add
    SCON_INTRINSIC_SUB,  ///< Sub
    SCON_INTRINSIC_MUL,  ///< Mul
    SCON_INTRINSIC_DIV,  ///< Div
    SCON_INTRINSIC_MOD,  ///< Mod
    SCON_INTRINSIC_INC,  ///< Inc
    SCON_INTRINSIC_DEC,  ///< Dec
    SCON_INTRINSIC_BAND, ///< Bitwise And
    SCON_INTRINSIC_BOR,  ///< Bitwise Or
    SCON_INTRINSIC_BXOR, ///< Bitwise Xor
    SCON_INTRINSIC_BNOT, ///< Bitwise Not
    SCON_INTRINSIC_SHL,  ///< Bitwise Shift Left
    SCON_INTRINSIC_SHR,  ///< Bitwise Shift Right
    SCON_INTRINSIC_EQ,   ///< Equal
    SCON_INTRINSIC_NEQ,  ///< Not Equal
    SCON_INTRINSIC_SEQ,  ///< Strict Equal
    SCON_INTRINSIC_SNEQ, ///< Strict Not Equal
    SCON_INTRINSIC_LT,   ///< Less
    SCON_INTRINSIC_LE,   ///< Less Equal
    SCON_INTRINSIC_GT,   ///< Greater
    SCON_INTRINSIC_GE,   ///< Greater Equal
    SCON_INTRINSIC_MAX   ///< The amount of intrinsics
} scon_intrinsic_t;

/**
 * @brief SCON intrinsic handler function type.
 */
typedef void (*scon_intrinsic_handler_t)(struct scon_compiler* compiler, struct scon_item* expr, struct scon_expr* out);

/**
 * @brief SCON intrinsic handler functions array.
 */
extern scon_intrinsic_handler_t sconIntrinsicHandlers[SCON_INTRINSIC_MAX];

/**
 * @brief SCON intrinsic names array.
 */
extern const char* sconIntrinsics[SCON_INTRINSIC_MAX];

/**
 * @brief Registers all SCON intrinsics with the given SCON instance.
 *
 * @param scon The SCON instance to register intrinsics with.
 */
SCON_API void scon_intrinsic_register_all(struct scon* scon);

#endif
