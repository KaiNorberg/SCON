#include "inst.h"
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
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(ast != SCON_NULL);

    scon_function_t* func = scon_function_new(scon);
    scon_item_t* funcItem = SCON_CONTAINER_OF(func, scon_item_t, function);
    SCON_GC_RETAIN_ITEM(scon, funcItem);
    scon_compiler_t compiler;

    scon_compiler_init(&compiler, scon, func, SCON_NULL);
    compiler.lastItem = SCON_HANDLE_TO_ITEM(ast);

    scon_item_t* astItem = SCON_HANDLE_TO_ITEM(ast);
    funcItem->input = astItem->input;
    if (astItem->length == 0)
    {
        SCON_ERROR_COMPILE(&compiler, astItem, "empty function");
    }

    scon_expr_t lastExpr = SCON_EXPR_NONE();

    if (astItem->length > 1)
    {
        scon_reg_t target = scon_reg_alloc(&compiler);
        scon_compile_list(&compiler, target);

        for (scon_size_t i = 0; i < astItem->length; ++i)
        {
            scon_item_t* item = SCON_LIST_GET_ITEM(astItem, i);
            scon_expr_t argExpr = SCON_EXPR_NONE();
            scon_expr_build(&compiler, item, &argExpr);
            scon_compile_append(&compiler, target, &argExpr);
            scon_expr_done(&compiler, &argExpr);
        }

        lastExpr = SCON_EXPR_REG(target);
    }
    else
    {
        scon_expr_build(&compiler, SCON_LIST_GET_ITEM(astItem, 0), &lastExpr);
    }

    scon_compile_return(&compiler, &lastExpr);

    scon_compiler_deinit(&compiler);

    return func;
}

SCON_API void scon_compiler_init(scon_compiler_t* compiler, scon_t* scon, scon_function_t* function,
    scon_compiler_t* enclosing)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(function != SCON_NULL);

    compiler->enclosing = enclosing;
    compiler->scon = scon;
    compiler->function = function;
    compiler->localCount = 0;
    compiler->lastItem = SCON_NULL;

    SCON_MEMSET(compiler->regAlloc, 0, sizeof(compiler->regAlloc));
    SCON_MEMSET(compiler->regLocal, 0, sizeof(compiler->regLocal));
    SCON_MEMSET(compiler->locals, 0, sizeof(compiler->locals));
}

SCON_API void scon_compiler_deinit(scon_compiler_t* compiler)
{
    SCON_ASSERT(compiler != SCON_NULL);
    (void)compiler;
}

SCON_API scon_reg_t scon_reg_alloc(scon_compiler_t* compiler)
{
    SCON_ASSERT(compiler != SCON_NULL);

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

    SCON_ERROR_COMPILE(compiler, compiler->lastItem, "too many registers in function");
    return (scon_reg_t)-1;
}

SCON_API scon_reg_t scon_reg_alloc_range(scon_compiler_t* compiler, scon_uint32_t count)
{
    SCON_ASSERT(compiler != SCON_NULL);

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

    SCON_ERROR_COMPILE(compiler, compiler->lastItem, "too many registers in function");
    return (scon_reg_t)-1;
}

SCON_API scon_reg_t scon_reg_alloc_range_hint(scon_compiler_t* compiler, scon_uint32_t count, scon_reg_t hint)
{
    SCON_ASSERT(compiler != SCON_NULL);

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
    SCON_ASSERT(compiler != SCON_NULL);

    if (SCON_REG_IS_LOCAL(compiler, reg))
    {
        return;
    }

    SCON_REG_CLEAR_ALLOCATED(compiler, reg);
}

SCON_API void scon_reg_free_range(scon_compiler_t* compiler, scon_reg_t start, scon_uint32_t count)
{
    SCON_ASSERT(compiler != SCON_NULL);

    for (scon_uint32_t i = 0; i < count; i++)
    {
        scon_reg_free(compiler, start + i);
    }
}

static inline void scon_expr_build_atom(scon_compiler_t* compiler, scon_item_t* atom, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(atom != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    if (atom->flags &
        (SCON_ITEM_FLAG_QUOTED | SCON_ITEM_FLAG_NATIVE | SCON_ITEM_FLAG_FLOAT_SHAPED | SCON_ITEM_FLAG_INT_SHAPED))
    {
        *out = SCON_EXPR_CONST_ITEM(compiler, atom);
        return;
    }

    scon_local_t* local = scon_local_lookup(compiler, &atom->atom);
    if (local != SCON_NULL)
    {
        if (!SCON_LOCAL_IS_DEFINED(local))
        {
            SCON_ERROR_COMPILE(compiler, atom, "undefined variable '%.*s'", atom->atom.length, atom->atom.string);
        }

        *out = local->expr;
        return;
    }

    for (scon_uint32_t i = 0; i < compiler->scon->constantCount; i++)
    {
        if (&atom->atom == compiler->scon->constants[i].name)
        {
            *out = SCON_EXPR_CONST_ITEM(compiler, compiler->scon->constants[i].item);
            return;
        }
    }

    SCON_ERROR_COMPILE(compiler, atom, "undefined variable '%.*s'", atom->atom.length, atom->atom.string);
}

static inline void scon_expr_build_list(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    if (list->length == 0)
    {
        *out = SCON_EXPR_NIL(compiler);
        return;
    }

    scon_item_t* head = SCON_LIST_GET_ITEM(list, 0);
    if (head->type != SCON_ITEM_TYPE_ATOM || (head->flags & (SCON_ITEM_FLAG_QUOTED | SCON_ITEM_FLAG_INT_SHAPED | SCON_ITEM_FLAG_FLOAT_SHAPED)))
    {
        scon_reg_t target = scon_expr_get_reg(compiler, out);
        scon_compile_list(compiler, target);

        for (scon_uint32_t i = 0; i < list->length; i++)
        {
            scon_expr_t argExpr = SCON_EXPR_NONE();
            scon_expr_build(compiler, SCON_LIST_GET_ITEM(list, i), &argExpr);

            scon_compile_append(compiler, target, &argExpr);

            scon_expr_done(compiler, &argExpr);
        }

        *out = SCON_EXPR_REG(target);
        return;
    }

    if (head->flags & SCON_ITEM_FLAG_INTRINSIC)
    {
        scon_intrinsic_handler_t handler = sconIntrinsicHandlers[head->atom.intrinsic];
        if (handler != SCON_NULL)
        {
            handler(compiler, list, out);
            return;
        }
    }

    scon_uint32_t arity = list->length - 1;
    scon_uint32_t regCount = arity == 0 ? 1 : arity;

    scon_reg_t base = 0;
    for (scon_int32_t i = SCON_REGISTER_MAX - 1; i >= 0; i--)
    {
        if (SCON_REG_IS_ALLOCATED(compiler, i))
        {
            base = i + 1;
            break;
        }
    }

    if (base + regCount > SCON_REGISTER_MAX)
    {
        SCON_ERROR_COMPILE(compiler, list, "too many registers in function");
    }

    for (scon_uint32_t i = 0; i < regCount; i++)
    {
        SCON_REG_SET_ALLOCATED(compiler, base + i);
    }

    scon_expr_t callable = SCON_EXPR_NONE();
    scon_expr_build(compiler, head, &callable);

    for (scon_uint32_t i = 1; i < list->length; i++)
    {
        scon_reg_t target = base + i - 1;
        scon_expr_t argExpr = SCON_EXPR_TARGET(target);
        scon_expr_build(compiler, SCON_LIST_GET_ITEM(list, i), &argExpr);

        if (argExpr.mode != SCON_MODE_REG || argExpr.reg != target)
        {
            scon_compile_move(compiler, target, &argExpr);
            scon_expr_done(compiler, &argExpr);
        }
    }

    scon_compile_call(compiler, base, &callable, arity);

    scon_expr_done(compiler, &callable);

    if (regCount > 1)
    {
        scon_reg_free_range(compiler, base + 1, regCount - 1);
    }

    *out = SCON_EXPR_REG(base);
}

SCON_API void scon_expr_build(scon_compiler_t* compiler, scon_item_t* item, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(item != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    scon_item_t* previousItem = compiler->lastItem;
    if (item != SCON_NULL && item->input != SCON_NULL)
    {
        compiler->lastItem = item;
    }

    if (item->type == SCON_ITEM_TYPE_ATOM)
    {
        scon_expr_build_atom(compiler, item, out);
    }
    else
    {
        scon_expr_build_list(compiler, item, out);
    }

    compiler->lastItem = previousItem;
}

SCON_API scon_local_t* scon_local_def(scon_compiler_t* compiler, scon_atom_t* name)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(name != SCON_NULL);

    if (compiler->localCount >= SCON_REGISTER_MAX)
    {
        SCON_ERROR_COMPILE(compiler, compiler->lastItem, "too many local variables");
    }

    compiler->locals[compiler->localCount].name = name;
    compiler->locals[compiler->localCount].expr = SCON_EXPR_NONE();
    return &compiler->locals[compiler->localCount++];
}

SCON_API void scon_local_def_done(scon_compiler_t* compiler, scon_local_t* local, scon_expr_t* expr)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(local != SCON_NULL);
    SCON_ASSERT(expr != SCON_NULL);

    if (local->expr.mode != SCON_MODE_NONE)
    {
        return;
    }

    if (expr->mode == SCON_MODE_REG)
    {
        SCON_REG_SET_LOCAL(compiler, expr->reg);
    }

    local->expr = *expr;
}

SCON_API scon_local_t* scon_local_add_arg(scon_compiler_t* compiler, scon_atom_t* name)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(name != SCON_NULL);

    if (compiler->localCount >= SCON_REGISTER_MAX)
    {
        SCON_ERROR_COMPILE(compiler, compiler->lastItem, "too many local variables");
    }

    scon_reg_t reg = scon_reg_alloc(compiler);
    compiler->locals[compiler->localCount].name = name;
    compiler->locals[compiler->localCount].expr = SCON_EXPR_REG(reg);
    SCON_REG_SET_LOCAL(compiler, reg);

    return &compiler->locals[compiler->localCount++];
}

SCON_API scon_local_t* scon_local_lookup(scon_compiler_t* compiler, scon_atom_t* name)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(name != SCON_NULL);

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

                scon_local_t* local = scon_local_def(compiler, name);
                scon_local_def_done(compiler, local, &constExpr);
                return local;
            }

            scon_const_slot_t slot = SCON_CONST_SLOT_CAPTURE(name);
            scon_const_t constant = scon_function_lookup_constant(compiler->scon, compiler->function, &slot);
            scon_expr_t constExpr = SCON_EXPR_CONST(constant);

            scon_local_t* local = scon_local_def(compiler, name);
            scon_local_def_done(compiler, local, &constExpr);
            return local;
        }
        current = current->enclosing;
    }

    return SCON_NULL;
}

#endif
