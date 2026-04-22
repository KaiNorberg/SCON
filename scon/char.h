#ifndef SCON_CHAR_H
#define SCON_CHAR_H 1

/**
 * @brief SCON character handling.
 * @defgroup char Characters
 * @file char.h
 *
 * Lookup tables are used to reduce branching and improve performance when parsing and processing characters.
 *
 * @{
 */

/**
 * @brief Character classification flags.
 * @enum scon_char_flags_t
 *
 * Follows the grammar defined in the README.
 */
typedef enum scon_char_flags
{
    SCON_CHAR_LETTER = (1 << 0),     ///< Is a letter.
    SCON_CHAR_DIGIT = (1 << 1),      ///< Is a decimal digit.
    SCON_CHAR_SYMBOL = (1 << 2),     ///< Is a symbol.
    SCON_CHAR_WHITESPACE = (1 << 3), ///< Is whitespace.
    SCON_CHAR_HEX_DIGIT = (1 << 4),  ///< Is a hexidecimal digit.
} scon_char_flags_t;

/**
 * @brief Character information.
 * @struct scon_char_info
 */
typedef struct scon_char_info
{
    scon_char_flags_t flags; ///< Character classification flags.
    char upper;              ///< Uppercase equivalent.
    char lower;              ///< Lowercase equivalent.
    char decodeEscape;       ///< The char to decode to when escaped.
    char encodeEscape;       ///< The char to use when encoding an escape.
    unsigned char integer;   ///< Integer value.
} scon_char_info_t;

/**
 * @brief Global character lookup table.
 */
extern scon_char_info_t sconCharTable[256];

/**
 * @brief Get the character flags for a given character.
 *
 * @param _c The character to get flags for.
 * @return The character flags.
 */
#define SCON_CHAR_FLAGS(_c) (sconCharTable[(unsigned char)(_c)].flags)

/**
 * @brief Get the lowercase equivalent of a character.
 *
 * @param _c The character to get the lowercase equivalent of.
 * @return The lowercase equivalent of the character.
 */
#define SCON_CHAR_TO_LOWER(_c) (sconCharTable[(unsigned char)(_c)].lower)

/**
 * @brief Get the uppercase equivalent of a character.
 *
 * @param _c The character to get the uppercase equivalent of.
 * @return The uppercase equivalent of the character.
 */
#define SCON_CHAR_TO_UPPER(_c) (sconCharTable[(unsigned char)(_c)].upper)

/**
 * @brief Check if a character is whitespace.
 *
 * @param _c The character to check.
 * @return SCON_TRUE if the character is whitespace, SCON_FALSE otherwise.
 */
#define SCON_CHAR_IS_WHITESPACE(_c) (SCON_CHAR_FLAGS(_c) & SCON_CHAR_WHITESPACE)

/**
 * @brief Check if a character is a letter.
 *
 * @param _c The character to check.
 * @return SCON_TRUE if the character is a letter, SCON_FALSE otherwise.
 */
#define SCON_CHAR_IS_LETTER(_c) (SCON_CHAR_FLAGS(_c) & SCON_CHAR_LETTER)

/**
 * @brief Check if a character is a decimal digit.
 *
 * @param _c The character to check.
 * @return SCON_TRUE if the character is a decimal digit, SCON_FALSE otherwise.
 */
#define SCON_CHAR_IS_DIGIT(_c) (SCON_CHAR_FLAGS(_c) & SCON_CHAR_DIGIT)

/**
 * @brief Check if a character is a symbol.
 *
 * @param _c The character to check.
 * @return SCON_TRUE if the character is a symbol, SCON_FALSE otherwise.
 */
#define SCON_CHAR_IS_SYMBOL(_c) (SCON_CHAR_FLAGS(_c) & SCON_CHAR_SYMBOL)

/**
 * @brief Check if a character is a hexidecimal digit.
 *
 * @param _c The character to check.
 * @return SCON_TRUE if the character is a hexidecimal digit, SCON_FALSE otherwise.
 */
#define SCON_CHAR_IS_HEX_DIGIT(_c) (SCON_CHAR_FLAGS(_c) & SCON_CHAR_HEX_DIGIT)

/** @} */

#endif
