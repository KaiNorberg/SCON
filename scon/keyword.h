#ifndef SCON_KEYWORD_H
#define SCON_KEYWORD_H 1

#include "defs.h"

struct scon;
struct scon_compiler;
struct scon_item;
struct scon_expr;

/**
 * @brief SCON keyword management.
 * @defgroup keyword Keywords
 * @file keyword.h
 *
 * A keyword is an atom recognized by the bytecode compiler as a special form or built-in operator, resulting in it
 * emitting specific bytecode instructions for each keyword.
 *
 * This is in contrast to a "native" which will be invoked as a standard function call at runtime.
 *
 * @{
 */

/**
 * @brief SCON keyword types.
 */
typedef enum
{
    SCON_KEYWORD_NONE = 0,      ///< None
    SCON_KEYWORD_QUOTE,         ///< Quote
    SCON_KEYWORD_LIST,          ///< List
    SCON_KEYWORD_DO,            ///< Do
    SCON_KEYWORD_LAMBDA,        ///< Lambda
    SCON_KEYWORD_DEF,           ///< Def
    SCON_KEYWORD_LET,           ///< Let
    SCON_KEYWORD_IF,            ///< If
    SCON_KEYWORD_WHEN,          ///< When
    SCON_KEYWORD_UNLESS,        ///< Unless
    SCON_KEYWORD_COND,          ///< Cond
    SCON_KEYWORD_AND,           ///< And
    SCON_KEYWORD_OR,            ///< Or
    SCON_KEYWORD_NOT,           ///< Not
    SCON_KEYWORD_ADD,           ///< Add
    SCON_KEYWORD_SUB,           ///< Sub
    SCON_KEYWORD_MUL,           ///< Mul
    SCON_KEYWORD_DIV,           ///< Div
    SCON_KEYWORD_MOD,           ///< Mod
    SCON_KEYWORD_EQUAL,         ///< Equal
    SCON_KEYWORD_STRICT_EQUAL,  ///< Strict Equal
    SCON_KEYWORD_NOT_EQUAL,     ///< Not Equal
    SCON_KEYWORD_LESS,          ///< Less
    SCON_KEYWORD_LESS_EQUAL,    ///< Less Equal
    SCON_KEYWORD_GREATER,       ///< Greater
    SCON_KEYWORD_GREATER_EQUAL, ///< Greater Equal
    SCON_KEYWORD_BIT_AND,       ///< Bitwise And
    SCON_KEYWORD_BIT_OR,        ///< Bitwise Or
    SCON_KEYWORD_BIT_XOR,       ///< Bitwise Xor
    SCON_KEYWORD_BIT_NOT,       ///< Bitwise Not
    SCON_KEYWORD_BIT_SHL,       ///< Bitwise Shift Left
    SCON_KEYWORD_BIT_SHR,       ///< Bitwise Shift Right
    SCON_KEYWORD_MAX            ///< The amount of keywords
} scon_keyword_t;

/**
 * @brief SCON keyword handler function type.
 */
typedef void (*scon_keyword_handler_t)(struct scon_compiler* compiler, struct scon_item* expr, struct scon_expr* out);

/**
 * @brief SCON keyword handler functions array.
 */
extern scon_keyword_handler_t sconKeywordHandlers[SCON_KEYWORD_MAX];

/**
 * @brief SCON keyword names array.
 */
extern const char* sconKeywords[SCON_KEYWORD_MAX];

/**
 * @brief Registers all SCON keywords with the given SCON instance.
 *
 * @param scon The SCON instance to register keywords with.
 */
SCON_API void scon_keyword_register_all(struct scon* scon);

#endif
