#ifndef SCON_INTRINSIC_H
#define SCON_INTRINSIC_H 1

#include "defs.h"
#include "native.h"

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
typedef scon_uint8_t scon_intrinsic_t;

#define SCON_INTRINSIC_NONE 0   ///< None
#define SCON_INTRINSIC_QUOTE 1  ///< Quote
#define SCON_INTRINSIC_LIST 2   ///< List
#define SCON_INTRINSIC_DO 3     ///< Do
#define SCON_INTRINSIC_LAMBDA 4 ///< Lambda
#define SCON_INTRINSIC_THREAD 5 ///< Thread
#define SCON_INTRINSIC_DEF 6    ///< Def
#define SCON_INTRINSIC_IF 7     ///< If
#define SCON_INTRINSIC_WHEN 8   ///< When
#define SCON_INTRINSIC_UNLESS 9 ///< Unless
#define SCON_INTRINSIC_COND 10  ///< Cond
#define SCON_INTRINSIC_MATCH 11 ///< Match
#define SCON_INTRINSIC_AND 12   ///< And
#define SCON_INTRINSIC_OR 13    ///< Or
#define SCON_INTRINSIC_NOT 14   ///< Not
#define SCON_INTRINSIC_ADD 15   ///< Add
#define SCON_INTRINSIC_SUB 16   ///< Sub
#define SCON_INTRINSIC_MUL 17   ///< Mul
#define SCON_INTRINSIC_DIV 18   ///< Div
#define SCON_INTRINSIC_MOD 19   ///< Mod
#define SCON_INTRINSIC_INC 20   ///< Inc
#define SCON_INTRINSIC_DEC 21   ///< Dec
#define SCON_INTRINSIC_BAND 22  ///< Bitwise And
#define SCON_INTRINSIC_BOR 23   ///< Bitwise Or
#define SCON_INTRINSIC_BXOR 24  ///< Bitwise Xor
#define SCON_INTRINSIC_BNOT 25  ///< Bitwise Not
#define SCON_INTRINSIC_SHL 26   ///< Bitwise Shift Left
#define SCON_INTRINSIC_SHR 27   ///< Bitwise Shift Right
#define SCON_INTRINSIC_EQ 28    ///< Equal
#define SCON_INTRINSIC_NEQ 29   ///< Not Equal
#define SCON_INTRINSIC_SEQ 30   ///< Strict Equal
#define SCON_INTRINSIC_SNEQ 31  ///< Strict Not Equal
#define SCON_INTRINSIC_LT 32    ///< Less
#define SCON_INTRINSIC_LE 33    ///< Less Equal
#define SCON_INTRINSIC_GT 34    ///< Greater
#define SCON_INTRINSIC_GE 35    ///< Greater Equal
#define SCON_INTRINSIC_MAX 36   ///< The amount of intrinsics

/**
 * @brief SCON intrinsic handler function type.
 */
typedef void (*scon_intrinsic_handler_t)(struct scon_compiler* compiler, struct scon_item* expr, struct scon_expr* out);

/**
 * @brief SCON intrinsic handler functions array.
 */
extern scon_intrinsic_handler_t sconIntrinsicHandlers[SCON_INTRINSIC_MAX];

/**
 * @brief SCON intrinsic native functions array.
 */
extern scon_native_fn sconIntrinsicNatives[SCON_INTRINSIC_MAX];

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
