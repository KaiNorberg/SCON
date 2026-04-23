#ifndef SCON_COMPILE_H
#define SCON_COMPILE_H 1

#include "core.h"
#include "function.h"
#include "gc.h"
#include "inst.h"
#include "item.h"
#include "list.h"

/**
 * @brief SCON bytecode compilation.
 * @defgroup compile Compilation
 * @file compile.h
 *
 * The compilation process converts S-expressions into register-based bytecode that can be executed by the SCON virtual
 * machine.
 *
 * @{
 */

/**
 * @brief SCON expression descriptor structure.
 * @struct scon_expr_t
 */
typedef struct scon_expr
{
    scon_mode_t mode; ///< Expression mode.
    union {
        scon_uint16_t value;   ///< Raw union value
        scon_reg_t reg;        ///< Register index
        scon_const_t constant; ///< Constant index
    };
} scon_expr_t;

/**
 * @brief SCON local structure.
 * @struct scon_local_t
 */
typedef struct scon_local
{
    scon_atom_t* name; ///< The name of the local variable.
    scon_expr_t expr;  ///< The expression representing the local's value.
} scon_local_t;

/**
 * @brief SCON compiler structure.
 * @struct scon_compiler_t
 */
typedef struct scon_compiler
{
    struct scon_compiler* enclosing;                ///< The enclosing compiler context, or `SCON_NULL`.
    scon_t* scon;                                   ///< The SCON structure.
    scon_function_t* function;                      ///< The function being compiled.
    scon_uint16_t localCount;                       ///< The amount of local variables.
    scon_uint64_t regAlloc[SCON_REGISTER_MAX / 64]; ///< Bitmask of allocated registers.
    scon_uint64_t regLocal[SCON_REGISTER_MAX / 64]; ///< Bitmask of registers used by locals.
    scon_local_t locals[SCON_REGISTER_MAX];         ///< The local variables.
    scon_item_t* lastItem; ///< The last item processed by the compiler, used for error reporting.
} scon_compiler_t;

/**
 * @brief Compiles a SCON AST into a callable bytecode function.
 *
 * @warning The jump buffer must have been set using `SCON_CATCH` before calling this function.
 *
 * @param scon The SCON structure.
 * @param ast The root AST item to compile (usually a list of expressions).
 * @return A pointer to the compiled function.
 */
SCON_API scon_function_t* scon_compile(scon_t* scon, scon_handle_t* ast);

/**
 * @brief Initialize a compiler context.
 *
 * @param compiler The compiler context to initialize.
 * @param scon The SCON structure.
 * @param function The function to compile into.
 * @param enclosing The enclosing compiler context, or `SCON_NULL`.
 */
SCON_API void scon_compiler_init(scon_compiler_t* compiler, scon_t* scon, scon_function_t* function,
    scon_compiler_t* enclosing);

/**
 * @brief Deinitialize a compiler context.
 *
 * @param compiler The compiler context to deinitialize.
 */
SCON_API void scon_compiler_deinit(scon_compiler_t* compiler);

/**
 * @brief Set a register as allocated.
 *
 * @param _compiler The compiler instance.
 * @param _reg The register to set as allocated.
 */
#define SCON_REG_SET_ALLOCATED(_compiler, _reg) \
    do \
    { \
        (_compiler)->regAlloc[(_reg) / 64] |= (1ULL << ((_reg) % 64)); \
        if ((_reg) + 1 > (_compiler)->function->registerCount) \
        { \
            (_compiler)->function->registerCount = (_reg) + 1; \
        } \
    } while (0)

/**
 * @brief Clear a register's allocation status.
 *
 * @param _compiler The compiler instance.
 * @param _reg The register to clear.
 */
#define SCON_REG_CLEAR_ALLOCATED(_compiler, _reg) ((_compiler)->regAlloc[(_reg) / 64] &= ~(1ULL << ((_reg) % 64)))

/**
 * @brief Check if a register is allocated.
 *
 * @param _compiler The compiler instance.
 * @param _reg The register to check.
 */
#define SCON_REG_IS_ALLOCATED(_compiler, _reg) (((_compiler)->regAlloc[(_reg) / 64] & (1ULL << ((_reg) % 64))) != 0)

/**
 * @brief Set a register as a local.
 *
 * @param _compiler The compiler instance.
 * @param _reg The register to set as a local.
 */
#define SCON_REG_SET_LOCAL(_compiler, _reg) ((_compiler)->regLocal[(_reg) / 64] |= (1ULL << ((_reg) % 64)))

/**
 * @brief Clear a register's local status.
 *
 * @param _compiler The compiler instance.
 * @param _reg The register to clear.
 */
#define SCON_REG_CLEAR_LOCAL(_compiler, _reg) ((_compiler)->regLocal[(_reg) / 64] &= ~(1ULL << ((_reg) % 64)))

/**
 * @brief Check if a register is a local.
 *
 * @param _compiler The compiler instance.
 * @param _reg The register to check.
 */
#define SCON_REG_IS_LOCAL(_compiler, _reg) (((_compiler)->regLocal[(_reg) / 64] & (1ULL << ((_reg) % 64))) != 0)

/**
 * @brief Allocate a new register.
 *
 * @param compiler The compiler context.
 * @return The allocated register index.
 */
SCON_API scon_reg_t scon_reg_alloc(scon_compiler_t* compiler);

/**
 * @brief Allocate a range of registers.
 *
 * @param compiler The compiler context.
 * @param count The number of registers to allocate.
 * @return The first register index in the allocated range.
 */
SCON_API scon_reg_t scon_reg_alloc_range(scon_compiler_t* compiler, scon_uint32_t count);

/**
 * @brief Free a register.
 *
 * @param compiler The compiler context.
 * @param reg The register index to free.
 */
SCON_API void scon_reg_free(scon_compiler_t* compiler, scon_reg_t reg);

/**
 * @brief Free a range of registers.
 *
 * @param compiler The compiler context.
 * @param start The first register index in the range to free.
 * @param count The number of registers to free.
 */
SCON_API void scon_reg_free_range(scon_compiler_t* compiler, scon_reg_t start, scon_uint32_t count);

/**
 * @brief Create a `SCON_MODE_NONE` mode expression.
 */
#define SCON_EXPR_NONE() ((scon_expr_t){.mode = SCON_MODE_NONE})

/**
 * @brief Create a `SCON_MODE_REG` mode expression.
 *
 * @param _reg The register index.
 */
#define SCON_EXPR_REG(_reg) ((scon_expr_t){.mode = SCON_MODE_REG, .reg = (_reg)})

/**
 * @brief Create a `SCON_MODE_CONST` mode expression.
 *
 * @param _const The constant index.
 */
#define SCON_EXPR_CONST(_const) ((scon_expr_t){.mode = SCON_MODE_CONST, .constant = (_const)})

/**
 * @brief Create a `SCON_MODE_CONST` mode expression for a specific item.
 *
 * @param _compiler The compiler context.
 * @param _item The item to look up.
 */
#define SCON_EXPR_CONST_ITEM(_compiler, _item) \
    SCON_EXPR_CONST(scon_function_lookup_constant((_compiler)->scon, (_compiler)->function, \
        &(scon_const_slot_t){.type = SCON_CONST_SLOT_ITEM, .item = (_item)}))

/**
 * @brief Create a `SCON_MODE_CONST` mode expression for a specific atom.
 *
 * @param _compiler The compiler context.
 * @param _atom The atom to look up.
 */
#define SCON_EXPR_CONST_ATOM(_compiler, _atom) \
    SCON_EXPR_CONST(scon_function_lookup_constant((_compiler)->scon, (_compiler)->function, \
        &(scon_const_slot_t){.type = SCON_CONST_SLOT_ITEM, .item = SCON_CONTAINER_OF(_atom, scon_item_t, atom)}))

/**
 * @brief Create a `SCON_MODE_TARGET` mode expression.
 *
 * @param _reg The target register hint.
 */
#define SCON_EXPR_TARGET(_reg) ((scon_expr_t){.mode = SCON_MODE_TARGET, .reg = (_reg)})

/**
 * @brief Create a `SCON_MODE_CONST` mode expression for the true constant.
 *
 * @param _compiler The compiler context.
 */
#define SCON_EXPR_TRUE(_compiler) \
    SCON_EXPR_CONST(scon_function_lookup_constant((_compiler)->scon, (_compiler)->function, \
        &(scon_const_slot_t){.type = SCON_CONST_SLOT_ITEM, .item = (_compiler)->scon->trueItem}))

/**
 * @brief Create a `SCON_MODE_CONST` mode expression for the false constant.
 *
 * @param _compiler The compiler context.
 */
#define SCON_EXPR_FALSE(_compiler) \
    SCON_EXPR_CONST(scon_function_lookup_constant((_compiler)->scon, (_compiler)->function, \
        &(scon_const_slot_t){.type = SCON_CONST_SLOT_ITEM, .item = (_compiler)->scon->falseItem}))

/**
 * @brief Create a `SCON_MODE_CONST` mode expression for the nil constant.
 *
 * @param _compiler The compiler context.
 */
#define SCON_EXPR_NIL(_compiler) \
    SCON_EXPR_CONST(scon_function_lookup_constant((_compiler)->scon, (_compiler)->function, \
        &(scon_const_slot_t){.type = SCON_CONST_SLOT_ITEM, .item = (_compiler)->scon->nilItem}))

/**
 * @brief Compiles a single SCON item into an expression descriptor.
 *
 * @param compiler The compiler context.
 * @param item The item to compile.
 * @param Output pointer for the compiled expression.
 */
SCON_API void scon_expr_build(scon_compiler_t* compiler, scon_item_t* item, scon_expr_t* out);

/**
 * @brief Free resources associated with an expression descriptor.
 *
 * @param compiler The compiler context.
 * @param expr The expression to free.
 */
static inline void scon_expr_done(scon_compiler_t* compiler, scon_expr_t* expr)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(expr != SCON_NULL);
    if (expr->mode == SCON_MODE_REG)
    {
        scon_reg_free(compiler, expr->reg);
    }
}

/**
 * @brief Allocate a new register, favoring the output expression's target if provided.
 *
 * @param compiler The compiler context.
 * @param out The output expression which may contain a target hint.
 * @return The allocated register index.
 */
static inline scon_reg_t scon_expr_get_reg(scon_compiler_t* compiler, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    if (out != SCON_NULL && out->mode == SCON_MODE_TARGET)
    {
        return out->reg;
    }
    return scon_reg_alloc(compiler);
}

/**
 * @brief Check if a local variable has finished being defined.
 *
 * If the local variable is not defined, then we are currently within the expression that defines it.
 *
 * @param _local The local variable descriptor.
 */
#define SCON_LOCAL_IS_DEFINED(_local) ((_local)->expr.mode != SCON_MODE_NONE)

/**
 * @brief Define a new local variable.
 *
 * @note The `scon_local_def_done()` function must be called after this one.
 *
 * @param compiler The compiler context.
 * @param name The name of the local.
 * @return A pointer to the local variable descriptor.
 */
SCON_API scon_local_t* scon_local_def(scon_compiler_t* compiler, scon_atom_t* name);

/**
 * @brief Finalize a local variable definition with its value expression.
 *
 * @param compiler The compiler context.
 * @param local The local variable descriptor.
 * @param expr The expression representing the local's value.
 */
SCON_API void scon_local_def_done(scon_compiler_t* compiler, scon_local_t* local, scon_expr_t* expr);

/**
 * @brief Add a function argument local to the compiler context.
 *
 * @param compiler The compiler context.
 * @param name The name of the argument.
 * @return A pointer to the local variable descriptor.
 */
SCON_API scon_local_t* scon_local_add_arg(scon_compiler_t* compiler, scon_atom_t* name);

/**
 * @brief Look up a local by name and return its expression.
 *
 * @param compiler The compiler context.
 * @param name The name of the local.
 * @return A pointer to the local variable descriptor, or `SCON_NULL` if not found.
 */
SCON_API scon_local_t* scon_local_lookup(scon_compiler_t* compiler, scon_atom_t* name);

/**
 * @brief Emits an instruction to the current function.
 *
 * @param compiler The compiler context.
 * @param inst The instruction to emit.
 */
static inline void scon_compile_inst(scon_compiler_t* compiler, scon_inst_t inst)
{
    SCON_ASSERT(compiler != SCON_NULL);
    scon_uint32_t pos = compiler->lastItem != SCON_NULL ? compiler->lastItem->position : 0;
    scon_function_emit(compiler->scon, compiler->function, inst, pos);
}

/**
 * @brief Emits a `SCON_OPCODE_LIST` instruction, that creates a list in the target register.
 *
 * @param compiler The compiler context.
 * @param target The target register.
 */
static inline void scon_compile_list(scon_compiler_t* compiler, scon_reg_t target)
{
    SCON_ASSERT(compiler != SCON_NULL);
    scon_compile_inst(compiler, SCON_INST_MAKE_ABC(SCON_OPCODE_LIST, target, 0, 0));
}

/**
 * @brief Emits a `SCON_OPCODE_CALL` instruction, that returns its result in the target register.
 *
 * @param compiler The compiler context.
 * @param target The target register.
 * @param callable The callable expression.
 * @param arity The number of arguments.
 */
static inline void scon_compile_call(scon_compiler_t* compiler, scon_reg_t target, scon_expr_t* callable,
    scon_uint32_t arity)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(callable != SCON_NULL);
    SCON_ASSERT(callable->mode == SCON_MODE_REG || callable->mode == SCON_MODE_CONST);
    scon_compile_inst(compiler,
        SCON_INST_MAKE_ABC((scon_opcode_t)(SCON_OPCODE_CALL | callable->mode), target, arity, callable->value));
}

/**
 * @brief Emits a `SCON_OPCODE_MOVE` instruction, that moves the value of the source expression to the target register.
 *
 * @param compiler The compiler context.
 * @param target The target register.
 * @param expr The source expression.
 */
static inline void scon_compile_move(scon_compiler_t* compiler, scon_reg_t target, scon_expr_t* expr)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(expr != SCON_NULL);
    SCON_ASSERT(expr->mode == SCON_MODE_REG || expr->mode == SCON_MODE_CONST);
    scon_compile_inst(compiler,
        SCON_INST_MAKE_ABC((scon_opcode_t)(SCON_OPCODE_MOV | expr->mode), target, 0, expr->value));
}

/**
 * @brief Emits a jump instruction without a target offset.
 *
 * @param compiler The compiler context.
 * @param op The jump opcode (e.g., `SCON_OPCODE_JMP`, `SCON_OPCODE_JMPT`, `SCON_OPCODE_JMPF`).
 * @param a The register to test (if not `SCON_OPCODE_JMP`).
 * @return The index of the emitted instruction to be patched later.
 */
static inline scon_size_t scon_compile_jump(scon_compiler_t* compiler, scon_opcode_t op, scon_reg_t a)
{
    SCON_ASSERT(compiler != SCON_NULL);
    scon_size_t pos = compiler->function->instCount;
    scon_compile_inst(compiler, SCON_INST_MAKE_ASBX(op, a, 0));
    return pos;
}

/**
 * @brief Patch a previously emitted jump instruction to point to the current instruction.
 *
 * @param compiler The compiler context.
 * @param pos The index of the jump instruction to patch.
 */
static inline void scon_compile_jump_patch(scon_compiler_t* compiler, scon_size_t pos)
{
    SCON_ASSERT(compiler != SCON_NULL);
    scon_int32_t offset = (scon_int32_t)(compiler->function->instCount - pos - 1);
    compiler->function->insts[pos] = SCON_INST_SET_SBX(compiler->function->insts[pos], offset);
}

/**
 * @brief Emits a move instruction or allocates a new register if the expression is not already in a register.
 *
 * @param compiler The compiler context.
 * @param expr The expression to move or allocate.
 * @return The register where the value is stored.
 */
static inline scon_reg_t scon_compile_move_or_alloc(scon_compiler_t* compiler, scon_expr_t* expr)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(expr != SCON_NULL);
    SCON_ASSERT(expr->mode == SCON_MODE_REG || expr->mode == SCON_MODE_CONST);
    if (expr->mode == SCON_MODE_REG)
    {
        return expr->reg;
    }

    scon_reg_t target = scon_reg_alloc(compiler);
    scon_compile_move(compiler, target, expr);
    *expr = SCON_EXPR_REG(target);
    return target;
}

/**
 * @brief Emits a `SCON_OPCODE_RET` instruction.
 *
 * @param compiler The compiler context.
 * @param expr The expression to return.
 */
static inline void scon_compile_return(scon_compiler_t* compiler, scon_expr_t* expr)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(expr != SCON_NULL);

    if (expr->mode == SCON_MODE_REG)
    {
        scon_reg_t retReg = expr->reg;
        scon_uint32_t retInstPos = compiler->function->instCount;

        for (scon_size_t i = 0; i < retInstPos; i++)
        {
            scon_inst_t inst = compiler->function->insts[i];
            scon_opcode_t op = SCON_INST_GET_OP_BASE(inst);

            if (op != SCON_OPCODE_CALL)
            {
                continue;
            }

            scon_bool_t isTail = SCON_FALSE;

            scon_size_t curr = i + 1;
            scon_reg_t currentReg = SCON_INST_GET_A(inst);
            scon_bool_t valid = SCON_TRUE;

            while (curr < retInstPos)
            {
                scon_inst_t nextInst = compiler->function->insts[curr];
                scon_opcode_t nextOp = SCON_INST_GET_OP(nextInst);
                scon_opcode_t nextOpBase = SCON_INST_GET_OP_BASE(nextInst);

                if (nextOp == SCON_OPCODE_MOV && SCON_INST_GET_A(nextInst) == retReg &&
                    SCON_INST_GET_C(nextInst) == currentReg)
                {
                    currentReg = retReg;
                    curr++;
                }
                else if (nextOpBase == SCON_OPCODE_JMP)
                {
                    scon_int32_t offset = SCON_INST_GET_SBX(nextInst);
                    if (offset < 0)
                    {
                        valid = SCON_FALSE;
                        break;
                    }
                    curr = curr + 1 + offset;
                }
                else
                {
                    valid = SCON_FALSE;
                    break;
                }
            }

            if (valid && currentReg == retReg && curr == retInstPos)
            {
                isTail = SCON_TRUE;
            }

            if (isTail)
            {
                scon_mode_t mode = (scon_mode_t)(SCON_INST_GET_OP(inst) & SCON_MODE_CONST);
                compiler->function->insts[i] =
                    (inst & ~SCON_INST_MASK_OPCODE) | ((SCON_OPCODE_TAILCALL | mode) & SCON_INST_MASK_OPCODE);
            }
        }
    }

    if (expr->mode == SCON_MODE_NONE)
    {
        scon_expr_t nilExpr = SCON_EXPR_NIL(compiler);
        scon_uint32_t pos = compiler->lastItem != SCON_NULL ? compiler->lastItem->position : 0;
        scon_function_emit(compiler->scon, compiler->function,
            SCON_INST_MAKE_ABC(SCON_OPCODE_RET | SCON_MODE_CONST, 0, 0, nilExpr.constant), pos);
        return;
    }

    SCON_ASSERT(expr->mode == SCON_MODE_REG || expr->mode == SCON_MODE_CONST);
    scon_compile_inst(compiler, SCON_INST_MAKE_ABC((scon_opcode_t)(SCON_OPCODE_RET | expr->mode), 0, 0, expr->value));
}

/**
 * @brief Emits an `SCON_OPCODE_APPEND` instruction.
 *
 * @param compiler The compiler context.
 * @param target The target list register.
 * @param expr The expression to append.
 */
static inline void scon_compile_append(scon_compiler_t* compiler, scon_reg_t target, scon_expr_t* expr)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(expr != SCON_NULL);
    SCON_ASSERT(expr->mode == SCON_MODE_REG || expr->mode == SCON_MODE_CONST);
    scon_compile_inst(compiler,
        SCON_INST_MAKE_ABC((scon_opcode_t)(SCON_OPCODE_APPEND | expr->mode), target, 0, expr->value));
}

/**
 * @brief Emits a comparison, arithmetic or bitwise instruction.
 *
 * @param compiler The compiler context.
 * @param opBase The base opcode (without a mode) for the operation (e.g, `SCON_OPCODE_ADD`, `SCON_OPCODE_EQ`).
 * @param target The target register.
 * @param left The left operand register.
 * @param right The right operand expression.
 */
static inline void scon_compile_binary(scon_compiler_t* compiler, scon_opcode_t opBase, scon_reg_t target,
    scon_reg_t left, scon_expr_t* right)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(right != SCON_NULL);
    SCON_ASSERT(right->mode == SCON_MODE_REG || right->mode == SCON_MODE_CONST);
    scon_compile_inst(compiler, SCON_INST_MAKE_ABC((scon_opcode_t)(opBase | right->mode), target, left, right->value));
}

/**
 * @brief Emits a `SCON_OPCODE_CLOSURE` instruction.
 *
 * @param compiler The compiler context.
 * @param target The target register.
 * @param funcConst The constant index of the function prototype.
 */
static inline void scon_compile_closure(scon_compiler_t* compiler, scon_reg_t target, scon_const_t funcConst)
{
    SCON_ASSERT(compiler != SCON_NULL);
    scon_compile_inst(compiler, SCON_INST_MAKE_ABC(SCON_OPCODE_CLOSURE, target, 0, funcConst));
}

/**
 * @brief Emits a `SCON_OPCODE_CAPTURE` instruction.
 *
 * @param compiler The compiler context.
 * @param closureReg The register containing the closure.
 * @param slot The constant slot index in the closure to capture into.
 * @param expr The expression to be captured.
 */
static inline void scon_compile_capture(scon_compiler_t* compiler, scon_reg_t closureReg, scon_uint32_t slot,
    scon_expr_t* expr)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(expr != SCON_NULL);
    SCON_ASSERT(expr->mode == SCON_MODE_REG || expr->mode == SCON_MODE_CONST);
    scon_compile_inst(compiler,
        SCON_INST_MAKE_ABC((scon_opcode_t)(SCON_OPCODE_CAPTURE | expr->mode), closureReg, slot, expr->value));
}

#endif
