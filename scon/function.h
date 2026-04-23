#include "defs.h"
#ifndef SCON_FUNCTION_H
#define SCON_FUNCTION_H 1

struct scon;
struct scon_item;
struct scon_atom;

#include "inst.h"

/**
 * @brief SCON compiled function
 * @defgroup function SCON function
 * @file function.h
 *
 * A scon function is a sequence of instructions and an associated constant pool that can be executed by the SCON
 * virtual machine.
 *
 * ## Constants Template
 *
 * A function's constant pool is actually a template of constant slots. These slots can either
 * contain ann item or a variable name that needs to be captured from the enclosing scope when a closure is created.
 *
 * @{
 */

/**
 * @brief SCON constant slot type.
 * @typedef scon_const_slot_type_t
 */
typedef enum
{
    SCON_CONST_SLOT_NONE,    ///< No constant slot.
    SCON_CONST_SLOT_ITEM,    ///< A constant slot containing an item.
    SCON_CONST_SLOT_CAPTURE, ///< A constant slot containing a variable name to be captured.
} scon_const_slot_type_t;

/**
 * @brief SCON constant slot.
 * @struct scon_const_slot_t
 */
typedef struct scon_const_slot
{
    scon_const_slot_type_t type; ///< The type of the constant slot.
    union {
        scon_uint64_t raw;
        struct scon_item* item;    ///< The item contained in the constant slot.
        struct scon_atom* capture; ///< The name of the variable to be captured.
    };
} scon_const_slot_t;

/**
 * @brief Create a constant slot containing an item.
 *
 * @param _item The item to wrap in a slot.
 */
#define SCON_CONST_SLOT_ITEM(_item) ((scon_const_slot_t){.type = SCON_CONST_SLOT_ITEM, .item = (_item)})

/**
 * @brief Create a constant slot containing a variable name to be captured.
 *
 * @param _capture The name of the variable to be captured.
 */
#define SCON_CONST_SLOT_CAPTURE(_capture) ((scon_const_slot_t){.type = SCON_CONST_SLOT_CAPTURE, .capture = (_capture)})

/**
 * @brief SCON constant index type.
 * @typedef scon_const_t
 */
typedef scon_uint16_t scon_const_t;

/**
 * @brief SCON compiled function structure.
 * @struct scon_function_t
 */
typedef struct scon_function
{
    scon_uint32_t instCount;        ///< Number of instructions.
    scon_uint32_t instCapacity;     ///< Capacity of the instruction array.
    scon_inst_t* insts;             ///< An array of instructions.
    scon_uint32_t* positions;       ///< An array of source positions parallel to the instructions.
    scon_const_slot_t* constants;   ///< The array of constant slots forming the constant template.
    scon_uint16_t constantCount;    ///< Number of constants.
    scon_uint16_t constantCapacity; ///< Capacity of the constant array.
    scon_uint16_t registerCount;    ///< The number of registers the function uses.
    scon_uint8_t arity;             ///< The number of arguments the function expects.
} scon_function_t;

/**
 * @brief Initialize a function structure.
 *
 * @param func The function to initialize.
 */
SCON_API void scon_function_init(scon_function_t* func);

/**
 * @brief Deinitialize a function structure.
 *
 * @param func The function to deinitialize.
 */
SCON_API void scon_function_deinit(scon_function_t* func);

/**
 * @brief Create a new function.
 *
 * @param scon The SCON structure.
 * @return A pointer to the newly allocated function.
 */
SCON_API scon_function_t* scon_function_new(struct scon* scon);

/**
 * @brief Grow the instruction buffer.
 *
 * @param scon The SCON structure.
 * @param func The function to grow.
 */
SCON_API void scon_function_grow(struct scon* scon, scon_function_t* func);

/**
 * @brief Emit an instruction to the function.
 *
 * @param scon The SCON structure.
 * @param func The function to emit to.
 * @param inst The instruction to emit.
 * @param position The position in the source code.
 */
static inline void scon_function_emit(struct scon* scon, scon_function_t* func, scon_inst_t inst,
    scon_uint32_t position)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(func != SCON_NULL);
    if (func->instCount >= func->instCapacity)
    {
        scon_function_grow(scon, func);
    }
    func->positions[func->instCount] = position;
    func->insts[func->instCount++] = inst;
}

/**
 * @brief Get the index of a constant in a function's constant template, adding it if it doesn't exist.
 *
 * @param scon The SCON structure.
 * @param func The function.
 * @param slot The constant slot to add or lookup.
 * @return The index of the constant in the constant template.
 */
SCON_API scon_const_t scon_function_lookup_constant(struct scon* scon, scon_function_t* func, scon_const_slot_t* slot);

/** @} */

#endif
