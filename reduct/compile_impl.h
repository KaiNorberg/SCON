#include "inst.h"
#ifndef REDUCT_COMPILE_IMPL_H
#define REDUCT_COMPILE_IMPL_H 1

#include "compile.h"
#include "core.h"
#include "gc.h"
#include "item.h"
#include "item_impl.h"
#include "list.h"

REDUCT_API reduct_function_t* reduct_compile(reduct_t* reduct, reduct_handle_t* ast)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(ast != REDUCT_NULL);

    reduct_function_t* func = reduct_function_new(reduct);
    reduct_item_t* funcItem = REDUCT_CONTAINER_OF(func, reduct_item_t, function);
    REDUCT_GC_RETAIN_ITEM(reduct, funcItem);
    reduct_compiler_t compiler;

    reduct_compiler_init(&compiler, reduct, func, REDUCT_NULL);
    compiler.lastItem = REDUCT_HANDLE_TO_ITEM(ast);

    reduct_item_t* astItem = REDUCT_HANDLE_TO_ITEM(ast);
    funcItem->input = astItem->input;
    if (astItem->length == 0)
    {
        REDUCT_ERROR_COMPILE(&compiler, astItem, "empty function");
    }

    reduct_expr_t lastExpr = REDUCT_EXPR_NONE();

    if (astItem->length > 1)
    {
        reduct_reg_t target = reduct_reg_alloc(&compiler);
        reduct_compile_list(&compiler, target);

        reduct_handle_t h;
        REDUCT_LIST_FOR_EACH(&h, &astItem->list)
        {
            reduct_item_t* item = REDUCT_HANDLE_TO_ITEM(&h);
            reduct_expr_t argExpr = REDUCT_EXPR_NONE();
            reduct_expr_build(&compiler, item, &argExpr);
            reduct_compile_append(&compiler, target, &argExpr);
            reduct_expr_done(&compiler, &argExpr);
        }

        lastExpr = REDUCT_EXPR_REG(target);
    }
    else
    {
        reduct_expr_build(&compiler, reduct_list_nth_item(reduct, &astItem->list, 0), &lastExpr);
    }

    reduct_compile_return(&compiler, &lastExpr);

    reduct_compiler_deinit(&compiler);

    return func;
}

REDUCT_API void reduct_compiler_init(reduct_compiler_t* compiler, reduct_t* reduct, reduct_function_t* function,
    reduct_compiler_t* enclosing)
{
    REDUCT_ASSERT(compiler != REDUCT_NULL);
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(function != REDUCT_NULL);

    compiler->enclosing = enclosing;
    compiler->reduct = reduct;
    compiler->function = function;
    compiler->localCount = 0;
    compiler->lastItem = REDUCT_NULL;

    REDUCT_MEMSET(compiler->regAlloc, 0, sizeof(compiler->regAlloc));
    REDUCT_MEMSET(compiler->regLocal, 0, sizeof(compiler->regLocal));
    REDUCT_MEMSET(compiler->locals, 0, sizeof(compiler->locals));
}

REDUCT_API void reduct_compiler_deinit(reduct_compiler_t* compiler)
{
    REDUCT_ASSERT(compiler != REDUCT_NULL);
    (void)compiler;
}

REDUCT_API reduct_reg_t reduct_reg_alloc(reduct_compiler_t* compiler)
{
    REDUCT_ASSERT(compiler != REDUCT_NULL);

    for (reduct_uint32_t w = 0; w < REDUCT_REGISTER_MAX / 64; w++)
    {
        if (compiler->regAlloc[w] != ~(reduct_uint64_t)0)
        {
            for (reduct_uint32_t b = 0; b < 64; b++)
            {
                if (!(compiler->regAlloc[w] & (1ULL << b)))
                {
                    reduct_reg_t reg = (reduct_reg_t)(w * 64 + b);
                    REDUCT_REG_SET_ALLOCATED(compiler, reg);
                    return reg;
                }
            }
        }
    }

    REDUCT_ERROR_COMPILE(compiler, compiler->lastItem, "too many registers in function");
    return (reduct_reg_t)-1;
}

REDUCT_API reduct_reg_t reduct_reg_alloc_range(reduct_compiler_t* compiler, reduct_uint32_t count)
{
    REDUCT_ASSERT(compiler != REDUCT_NULL);

    if (count == 0)
    {
        return 0;
    }

    for (reduct_uint32_t i = 0; i <= REDUCT_REGISTER_MAX - count; i++)
    {
        reduct_uint32_t length;
        for (length = 0; length < count; length++)
        {
            reduct_uint32_t reg = i + length;
            if (REDUCT_REG_IS_ALLOCATED(compiler, reg))
            {
                break;
            }
        }
        if (length == count)
        {
            for (reduct_uint32_t j = 0; j < count; j++)
            {
                reduct_uint32_t reg = i + j;
                REDUCT_REG_SET_ALLOCATED(compiler, reg);
            }
            return i;
        }
        i += length;
    }

    REDUCT_ERROR_COMPILE(compiler, compiler->lastItem, "too many registers in function");
    return (reduct_reg_t)-1;
}

REDUCT_API void reduct_reg_free(reduct_compiler_t* compiler, reduct_reg_t reg)
{
    REDUCT_ASSERT(compiler != REDUCT_NULL);

    if (REDUCT_REG_IS_LOCAL(compiler, reg))
    {
        return;
    }

    REDUCT_REG_CLEAR_ALLOCATED(compiler, reg);
}

REDUCT_API void reduct_reg_free_range(reduct_compiler_t* compiler, reduct_reg_t start, reduct_uint32_t count)
{
    REDUCT_ASSERT(compiler != REDUCT_NULL);

    for (reduct_uint32_t i = 0; i < count; i++)
    {
        reduct_reg_free(compiler, start + i);
    }
}

static inline void reduct_expr_build_atom(reduct_compiler_t* compiler, reduct_item_t* atom, reduct_expr_t* out)
{
    REDUCT_ASSERT(compiler != REDUCT_NULL);
    REDUCT_ASSERT(atom != REDUCT_NULL);
    REDUCT_ASSERT(out != REDUCT_NULL);

    if (atom->flags &
        (REDUCT_ITEM_FLAG_QUOTED | REDUCT_ITEM_FLAG_NATIVE | REDUCT_ITEM_FLAG_INTRINSIC | REDUCT_ITEM_FLAG_FLOAT_SHAPED |
            REDUCT_ITEM_FLAG_INT_SHAPED))
    {
        *out = REDUCT_EXPR_CONST_ITEM(compiler, atom);
        return;
    }

    reduct_local_t* local = reduct_local_lookup(compiler, &atom->atom);
    if (local != REDUCT_NULL)
    {
        if (!REDUCT_LOCAL_IS_DEFINED(local))
        {
            REDUCT_ERROR_COMPILE(compiler, atom, "undefined variable '%.*s'", atom->atom.length, atom->atom.string);
        }

        *out = local->expr;
        return;
    }

    for (reduct_uint32_t i = 0; i < compiler->reduct->constantCount; i++)
    {
        if (&atom->atom == compiler->reduct->constants[i].name)
        {
            *out = REDUCT_EXPR_CONST_ITEM(compiler, compiler->reduct->constants[i].item);
            return;
        }
    }

    REDUCT_ERROR_COMPILE(compiler, atom, "undefined variable '%.*s'", atom->atom.length, atom->atom.string);
}

static inline void reduct_expr_build_list(reduct_compiler_t* compiler, reduct_item_t* list, reduct_expr_t* out)
{
    REDUCT_ASSERT(compiler != REDUCT_NULL);
    REDUCT_ASSERT(list != REDUCT_NULL);
    REDUCT_ASSERT(out != REDUCT_NULL);

    if (list->flags & REDUCT_ITEM_FLAG_QUOTED)
    {
        *out = REDUCT_EXPR_CONST_ITEM(compiler, list);
        return;
    }

    if (list->length == 0)
    {
        *out = REDUCT_EXPR_NIL(compiler);
        return;
    }

    reduct_item_t* head = reduct_list_nth_item(compiler->reduct, &list->list, 0);
    if ((head->flags & REDUCT_ITEM_FLAG_QUOTED) ||
        (head->type == REDUCT_ITEM_TYPE_ATOM &&
            (head->flags & (REDUCT_ITEM_FLAG_INT_SHAPED | REDUCT_ITEM_FLAG_FLOAT_SHAPED))))
    {
        reduct_reg_t target = reduct_expr_get_reg(compiler, out);
        reduct_compile_list(compiler, target);

        reduct_handle_t h;
        REDUCT_LIST_FOR_EACH(&h, &list->list)
        {
            reduct_item_t* item = REDUCT_HANDLE_TO_ITEM(&h);
            reduct_expr_t argExpr = REDUCT_EXPR_NONE();
            reduct_expr_build(compiler, item, &argExpr);

            reduct_compile_append(compiler, target, &argExpr);

            reduct_expr_done(compiler, &argExpr);
        }

        *out = REDUCT_EXPR_REG(target);
        return;
    }

    if (head->flags & REDUCT_ITEM_FLAG_INTRINSIC)
    {
        reduct_intrinsic_handler_t handler = reductIntrinsicHandlers[head->atom.intrinsic];
        if (handler != REDUCT_NULL)
        {
            handler(compiler, list, out);
            return;
        }
    }

    reduct_uint32_t arity = (reduct_uint32_t)list->length - 1;
    reduct_uint32_t regCount = arity == 0 ? 1 : arity;

    reduct_reg_t base = reduct_reg_get_base(compiler);

    if (base + regCount > REDUCT_REGISTER_MAX)
    {
        REDUCT_ERROR_COMPILE(compiler, list, "too many registers in function");
    }

    for (reduct_uint32_t i = 0; i < regCount; i++)
    {
        REDUCT_REG_SET_ALLOCATED(compiler, base + i);
    }

    reduct_expr_t callable = REDUCT_EXPR_NONE();
    reduct_expr_build(compiler, head, &callable);

    for (reduct_uint32_t i = 1; i < (reduct_uint32_t)list->length; i++)
    {
        reduct_reg_t target = base + i - 1;
        reduct_expr_t argExpr = REDUCT_EXPR_TARGET(target);
        reduct_expr_build(compiler, reduct_list_nth_item(compiler->reduct, &list->list, i), &argExpr);

        if (argExpr.mode != REDUCT_MODE_REG || argExpr.reg != target)
        {
            reduct_compile_move(compiler, target, &argExpr);
            reduct_expr_done(compiler, &argExpr);
        }
    }

    reduct_compile_call(compiler, base, &callable, arity);

    reduct_expr_done(compiler, &callable);

    if (regCount > 1)
    {
        reduct_reg_free_range(compiler, base + 1, regCount - 1);
    }

    *out = REDUCT_EXPR_REG(base);
}

REDUCT_API void reduct_expr_build(reduct_compiler_t* compiler, reduct_item_t* item, reduct_expr_t* out)
{
    REDUCT_ASSERT(compiler != REDUCT_NULL);
    REDUCT_ASSERT(item != REDUCT_NULL);
    REDUCT_ASSERT(out != REDUCT_NULL);

    reduct_item_t* previousItem = compiler->lastItem;
    if (item != REDUCT_NULL && item->input != REDUCT_NULL)
    {
        compiler->lastItem = item;
    }

    if (item->type == REDUCT_ITEM_TYPE_ATOM)
    {
        reduct_expr_build_atom(compiler, item, out);
    }
    else
    {
        reduct_expr_build_list(compiler, item, out);
    }

    compiler->lastItem = previousItem;
}

REDUCT_API reduct_local_t* reduct_local_def(reduct_compiler_t* compiler, reduct_atom_t* name)
{
    REDUCT_ASSERT(compiler != REDUCT_NULL);
    REDUCT_ASSERT(name != REDUCT_NULL);

    if (compiler->localCount >= REDUCT_REGISTER_MAX)
    {
        REDUCT_ERROR_COMPILE(compiler, compiler->lastItem, "too many local variables");
    }

    compiler->locals[compiler->localCount].name = name;
    compiler->locals[compiler->localCount].expr = REDUCT_EXPR_NONE();
    return &compiler->locals[compiler->localCount++];
}

REDUCT_API void reduct_local_def_done(reduct_compiler_t* compiler, reduct_local_t* local, reduct_expr_t* expr)
{
    REDUCT_ASSERT(compiler != REDUCT_NULL);
    REDUCT_ASSERT(local != REDUCT_NULL);
    REDUCT_ASSERT(expr != REDUCT_NULL);

    if (local->expr.mode != REDUCT_MODE_NONE)
    {
        return;
    }

    if (expr->mode == REDUCT_MODE_REG)
    {
        REDUCT_REG_SET_LOCAL(compiler, expr->reg);
    }

    local->expr = *expr;
}

REDUCT_API reduct_local_t* reduct_local_add_arg(reduct_compiler_t* compiler, reduct_atom_t* name)
{
    REDUCT_ASSERT(compiler != REDUCT_NULL);
    REDUCT_ASSERT(name != REDUCT_NULL);

    if (compiler->localCount >= REDUCT_REGISTER_MAX)
    {
        REDUCT_ERROR_COMPILE(compiler, compiler->lastItem, "too many local variables");
    }

    reduct_reg_t reg = reduct_reg_alloc(compiler);
    compiler->locals[compiler->localCount].name = name;
    compiler->locals[compiler->localCount].expr = REDUCT_EXPR_REG(reg);
    REDUCT_REG_SET_LOCAL(compiler, reg);

    return &compiler->locals[compiler->localCount++];
}

REDUCT_API reduct_local_t* reduct_local_lookup(reduct_compiler_t* compiler, reduct_atom_t* name)
{
    REDUCT_ASSERT(compiler != REDUCT_NULL);
    REDUCT_ASSERT(name != REDUCT_NULL);

    for (reduct_int16_t i = compiler->localCount - 1; i >= 0; i--)
    {
        if (compiler->locals[i].name == name)
        {
            return &compiler->locals[i];
        }
    }

    reduct_compiler_t* current = compiler->enclosing;
    while (current != REDUCT_NULL)
    {
        for (reduct_int16_t i = current->localCount - 1; i >= 0; i--)
        {
            if (current->locals[i].name != name)
            {
                continue;
            }

            if (current->locals[i].expr.mode == REDUCT_MODE_CONST)
            {
                reduct_const_t constant = reduct_function_lookup_constant(compiler->reduct, compiler->function,
                    &current->function->constants[current->locals[i].expr.constant]);
                reduct_expr_t constExpr = REDUCT_EXPR_CONST(constant);

                reduct_local_t* local = reduct_local_def(compiler, name);
                reduct_local_def_done(compiler, local, &constExpr);
                return local;
            }

            reduct_const_slot_t slot = REDUCT_CONST_SLOT_CAPTURE(name);
            reduct_const_t constant = reduct_function_lookup_constant(compiler->reduct, compiler->function, &slot);
            reduct_expr_t constExpr = REDUCT_EXPR_CONST(constant);

            reduct_local_t* local = reduct_local_def(compiler, name);
            reduct_local_def_done(compiler, local, &constExpr);
            return local;
        }
        current = current->enclosing;
    }

    return REDUCT_NULL;
}

#endif
