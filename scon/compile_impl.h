#ifndef SCON_COMPILE_IMPL_H
#define SCON_COMPILE_IMPL_H 1

#include "compile.h"
#include "core.h"
#include "gc.h"
#include "item.h"
#include "item_impl.h"
#include "list.h"

SCON_API scon_function_t* scon_compile(scon_t* scon, scon_handle_t* ast)
{
    scon_function_t* func = scon_function_new(scon);
    scon_compiler_t compiler;

    scon_compiler_init(&compiler, scon, func, SCON_NULL);
    compiler.lastNode = SCON_HANDLE_TO_ITEM(ast);

    scon_item_t* astNode = SCON_HANDLE_TO_ITEM(ast);
    if (astNode->length == 0)
    {
        SCON_THROW_ITEM(scon, "empty function", astNode);
    }

    scon_expr_t lastExpr = SCON_EXPR_NONE();

    for (scon_size_t i = 0; i < astNode->length; ++i)
    {
        scon_item_t* item = astNode->list.items[i];
        scon_expr_compile(&compiler, item, &lastExpr);
        if (i != astNode->length - 1)
        {
            scon_expr_done(&compiler, &lastExpr);
        }
    }

    scon_emit_return(&compiler, &lastExpr);

    scon_compiler_deinit(&compiler);

    scon_gc_retain_item(scon, SCON_CONTAINER_OF(func, scon_item_t, function));
    return func;
}

SCON_API void scon_compiler_init(scon_compiler_t* compiler, scon_t* scon, scon_function_t* function,
    scon_compiler_t* enclosing)
{
    compiler->enclosing = enclosing;
    compiler->scon = scon;
    compiler->function = function;
    compiler->localCount = 0;
    compiler->lastNode = SCON_NULL;
    compiler->targetHint = (scon_reg_t)-1;

    SCON_MEMSET(compiler->regAlloc, 0, sizeof(compiler->regAlloc));
    SCON_MEMSET(compiler->regLocal, 0, sizeof(compiler->regLocal));
    SCON_MEMSET(compiler->locals, 0, sizeof(compiler->locals));
}

SCON_API void scon_compiler_deinit(scon_compiler_t* compiler)
{
    (void)compiler;
}

static inline void scon_expr_compile_atom(scon_compiler_t* compiler, scon_item_t* atom, scon_expr_t* out)
{
    if (atom->flags &
        (SCON_ITEM_FLAG_QUOTED | SCON_ITEM_FLAG_NATIVE | SCON_ITEM_FLAG_FLOAT_SHAPED | SCON_ITEM_FLAG_INT_SHAPED))
    {
        scon_const_slot_t slot = SCON_CONST_SLOT_ITEM(atom);
        *out = SCON_EXPR_CONST(scon_function_lookup_constant(compiler->scon, compiler->function, &slot));
        return;
    }

    scon_local_t* local = scon_local_lookup(compiler, &atom->atom);
    if (local != SCON_NULL)
    {
        *out = local->expr;
        return;
    }

    if (atom == compiler->scon->trueItem)
    {
        *out = SCON_EXPR_CONST(scon_const_true(compiler->scon, compiler->function));
        return;
    }

    if (atom == compiler->scon->falseItem)
    {
        *out = SCON_EXPR_CONST(scon_const_false(compiler->scon, compiler->function));
        return;
    }

    if (atom == compiler->scon->nilItem)
    {
        *out = SCON_EXPR_CONST(scon_const_nil(compiler->scon, compiler->function));
        return;
    }

    if (atom == compiler->scon->piItem)
    {
        *out = SCON_EXPR_CONST(scon_const_pi(compiler->scon, compiler->function));
        return;
    }

    if (atom == compiler->scon->eItem)
    {
        *out = SCON_EXPR_CONST(scon_const_e(compiler->scon, compiler->function));
        return;
    }

    SCON_THROW_ITEM(compiler->scon, "undefined variable", SCON_COMPILER_ERR_ITEM(compiler, atom));
}

static inline void scon_expr_compile_list(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_reg_t hint = compiler->targetHint;
    compiler->targetHint = (scon_reg_t)-1;

    if (list->length == 0)
    {
        return;
    }

    scon_item_t* head = list->list.items[0];
    if (head->type != SCON_ITEM_TYPE_ATOM || head->flags & SCON_ITEM_FLAG_QUOTED)
    {
        SCON_THROW_ITEM(compiler->scon, "first item in list must be a callable atom", head);
    }

    if (head->flags & SCON_ITEM_FLAG_KEYWORD)
    {
        scon_keyword_handler_t handler = sconKeywordHandlers[head->atom.keyword];
        if (handler != SCON_NULL)
        {
            handler(compiler, list, out);
            return;
        }
    }

    scon_uint32_t arity = list->length - 1;
    scon_uint32_t regCount = arity == 0 ? 1 : arity;

    scon_reg_t base;
    if (hint != (scon_reg_t)-1)
    {
        base = scon_reg_alloc_range_hint(compiler, regCount, hint);
    }
    else
    {
        base = scon_reg_alloc_range(compiler, regCount);
    }

    scon_expr_t callable;
    scon_expr_compile(compiler, head, &callable);

    for (scon_uint32_t i = 1; i < list->length; i++)
    {
        scon_reg_t target = base + i - 1;
        compiler->targetHint = target;

        scon_expr_t argExpr;
        scon_expr_compile(compiler, list->list.items[i], &argExpr);

        compiler->targetHint = (scon_reg_t)-1;

        if (argExpr.mode != SCON_MODE_REG || argExpr.reg != target)
        {
            scon_emit_move(compiler, target, &argExpr);
            scon_expr_done(compiler, &argExpr);
        }
    }

    scon_emit_call(compiler, base, &callable, arity);

    scon_expr_done(compiler, &callable);

    if (regCount > 1)
    {
        scon_reg_free_range(compiler, base + 1, regCount - 1);
    }

    *out = SCON_EXPR_REG(base);
}

SCON_API void scon_expr_compile(scon_compiler_t* compiler, scon_item_t* item, scon_expr_t* out)
{
    scon_item_t* previousNode = compiler->lastNode;
    if (item != SCON_NULL && item->input != SCON_NULL)
    {
        compiler->lastNode = item;
    }

    if (item->type == SCON_ITEM_TYPE_ATOM)
    {
        scon_expr_compile_atom(compiler, item, out);
    }
    else
    {
        scon_expr_compile_list(compiler, item, out);
    }

    compiler->lastNode = previousNode;
}

SCON_API scon_reg_t scon_reg_alloc(scon_compiler_t* compiler)
{
    for (scon_uint32_t w = 0; w < SCON_REGISTER_MAX / 64; w++)
    {
        if (compiler->regAlloc[w] != ~(scon_uint64_t)0)
        {
            for (scon_uint32_t b = 0; b < 64; b++)
            {
                if (!(compiler->regAlloc[w] & (1ULL << b)))
                {
                    scon_reg_t reg = (scon_reg_t)(w * 64 + b);
                    SCON_REG_SET_ALLOCATED(compiler, reg);
                    return reg;
                }
            }
        }
    }

    SCON_THROW(compiler->scon, "too many registers in function");
    return (scon_reg_t)-1;
}

SCON_API scon_reg_t scon_reg_alloc_range(scon_compiler_t* compiler, scon_uint32_t count)
{
    if (count == 0)
    {
        return 0;
    }

    for (scon_uint32_t i = 0; i <= SCON_REGISTER_MAX - count; i++)
    {
        scon_uint32_t length;
        for (length = 0; length < count; length++)
        {
            scon_uint32_t reg = i + length;
            if (SCON_REG_IS_ALLOCATED(compiler, reg))
            {
                break;
            }
        }
        if (length == count)
        {
            for (scon_uint32_t j = 0; j < count; j++)
            {
                scon_uint32_t reg = i + j;
                SCON_REG_SET_ALLOCATED(compiler, reg);
            }
            return i;
        }
        i += length;
    }

    SCON_THROW(compiler->scon, "too many registers in function");
    return (scon_reg_t)-1;
}

SCON_API scon_reg_t scon_reg_alloc_range_hint(scon_compiler_t* compiler, scon_uint32_t count, scon_reg_t hint)
{
    if (hint != (scon_reg_t)-1 && hint + count <= SCON_REGISTER_MAX)
    {
        scon_bool_t canUseHint = SCON_TRUE;
        for (scon_uint32_t i = 1; i < count; i++)
        {
            scon_uint32_t reg = hint + i;
            if (SCON_REG_IS_ALLOCATED(compiler, reg))
            {
                canUseHint = SCON_FALSE;
                break;
            }
        }

        if (canUseHint)
        {
            for (scon_uint32_t i = 0; i < count; i++)
            {
                scon_uint32_t reg = hint + i;
                SCON_REG_SET_ALLOCATED(compiler, reg);
            }
            return hint;
        }
    }

    return scon_reg_alloc_range(compiler, count);
}

SCON_API void scon_reg_free(scon_compiler_t* compiler, scon_reg_t reg)
{
    if (SCON_REG_IS_LOCAL(compiler, reg))
    {
        return;
    }

    SCON_REG_CLEAR_ALLOCATED(compiler, reg);
}

SCON_API void scon_reg_free_range(scon_compiler_t* compiler, scon_reg_t start, scon_uint32_t count)
{
    for (scon_uint32_t i = 0; i < count; i++)
    {
        scon_reg_free(compiler, start + i);
    }
}

SCON_API scon_local_t* scon_local_add(scon_compiler_t* compiler, scon_atom_t* name, scon_expr_t* expr)
{
    if (compiler->localCount >= SCON_REGISTER_MAX)
    {
        SCON_THROW_ITEM(compiler->scon, "too many local variables",
            SCON_COMPILER_ERR_ITEM(compiler, SCON_CONTAINER_OF(name, scon_item_t, atom)));
    }
    compiler->locals[compiler->localCount].name = name;

    if (expr->mode == SCON_MODE_REG)
    {
        compiler->locals[compiler->localCount].expr = *expr;
        SCON_REG_SET_LOCAL(compiler, expr->reg);
    }
    else
    {
        compiler->locals[compiler->localCount].expr = *expr;
    }

    return &compiler->locals[compiler->localCount++];
}

SCON_API scon_local_t* scon_local_add_arg(scon_compiler_t* compiler, scon_atom_t* name)
{
    if (compiler->localCount >= SCON_REGISTER_MAX)
    {
        SCON_THROW_ITEM(compiler->scon, "too many local variables",
            SCON_COMPILER_ERR_ITEM(compiler, SCON_CONTAINER_OF(name, scon_item_t, atom)));
    }

    scon_reg_t reg = scon_reg_alloc(compiler);

    compiler->locals[compiler->localCount].name = name;
    compiler->locals[compiler->localCount].expr = SCON_EXPR_REG(reg);
    SCON_REG_SET_LOCAL(compiler, reg);

    return &compiler->locals[compiler->localCount++];
}

SCON_API scon_local_t* scon_local_lookup(scon_compiler_t* compiler, scon_atom_t* name)
{
    for (scon_int16_t i = compiler->localCount - 1; i >= 0; i--)
    {
        if (compiler->locals[i].name == name)
        {
            return &compiler->locals[i];
        }
    }

    scon_compiler_t* current = compiler->enclosing;
    while (current != SCON_NULL)
    {
        for (scon_int16_t i = current->localCount - 1; i >= 0; i--)
        {
            if (current->locals[i].name != name)
            {
                continue;
            }

            if (current->locals[i].expr.mode == SCON_MODE_CONST)
            {
                scon_const_t constant = scon_function_lookup_constant(compiler->scon, compiler->function,
                    &current->function->constants[current->locals[i].expr.constant]);
                scon_expr_t constExpr = SCON_EXPR_CONST(constant);
                return scon_local_add(compiler, name, &constExpr);
            }

            scon_const_slot_t slot = SCON_CONST_SLOT_CAPTURE(name);
            scon_const_t constant = scon_function_lookup_constant(compiler->scon, compiler->function, &slot);
            scon_expr_t constExpr = SCON_EXPR_CONST(constant);
            return scon_local_add(compiler, name, &constExpr);
        }
        current = current->enclosing;
    }

    return SCON_NULL;
}

#endif
