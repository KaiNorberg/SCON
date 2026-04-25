#ifndef SCON_INTRINSIC_IMPL_H
#define SCON_INTRINSIC_IMPL_H 1

#include "compile.h"
#include "intrinsic.h"
#include "item.h"
#include "stdlib.h"

static inline void scon_intrinsic_check_arity(scon_compiler_t* compiler, scon_item_t* list, scon_size_t expected,
    const char* name)
{
    if (SCON_UNLIKELY(list->length != (scon_uint32_t)expected + 1))
    {
        SCON_ERROR_COMPILE(compiler, list, "%s expects exactly %zu argument(s), got %zu", name, (scon_size_t)expected,
            (scon_size_t)list->length - 1);
    }
}

static inline void scon_intrinsic_check_min_arity(scon_compiler_t* compiler, scon_item_t* list, scon_size_t min,
    const char* name)
{
    if (SCON_UNLIKELY(list->length < (scon_uint32_t)min + 1))
    {
        SCON_ERROR_COMPILE(compiler, list, "%s expects at least %zu argument(s), got %zu", name, (scon_size_t)min,
            (scon_size_t)list->length - 1);
    }
}

static inline void scon_intrinsic_check_arity_range(scon_compiler_t* compiler, scon_item_t* list, scon_size_t min,
    scon_size_t max, const char* name)
{
    if (SCON_UNLIKELY(list->length < (scon_uint32_t)min + 1 || list->length > (scon_uint32_t)max + 1))
    {
        SCON_ERROR_COMPILE(compiler, list, "%s expects between %zu and %zu argument(s), got %zu", name, (scon_size_t)min,
            (scon_size_t)max, (scon_size_t)list->length - 1);
    }
}

void scon_intrinsic_quote(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    scon_intrinsic_check_arity(compiler, list, 1, "quote");
    *out = SCON_EXPR_CONST_ITEM(compiler, scon_list_nth_item(compiler->scon, &list->list, 1));
}

void scon_intrinsic_list(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    scon_reg_t target = scon_expr_get_reg(compiler, out);
    scon_compile_list(compiler, target);

    scon_handle_t h;
    scon_list_iter_t iter = SCON_LIST_ITER_AT(&list->list, 1);
    while (scon_list_iter_next(&iter, &h))
    {
        scon_expr_t argExpr = SCON_EXPR_NONE();
        scon_expr_build(compiler, SCON_HANDLE_TO_ITEM(&h), &argExpr);

        scon_compile_append(compiler, target, &argExpr);

        scon_expr_done(compiler, &argExpr);
    }

    *out = SCON_EXPR_REG(target);
}

static inline void scon_compile_build_into_target(scon_compiler_t* compiler, scon_item_t* item, scon_reg_t target)
{
    scon_expr_t expr = SCON_EXPR_TARGET(target);
    scon_expr_build(compiler, item, &expr);
    if (expr.mode == SCON_MODE_NONE)
    {
        scon_expr_t nil = SCON_EXPR_NIL(compiler);
        scon_compile_move(compiler, target, &nil);
    }
    else if (expr.mode != SCON_MODE_REG || expr.reg != target)
    {
        scon_compile_move(compiler, target, &expr);
        scon_expr_done(compiler, &expr);
    }
}

void scon_intrinsic_block_generic(scon_compiler_t* compiler, scon_item_t* list, scon_uint32_t startIdx,
    scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    if (startIdx >= list->length)
    {
        *out = SCON_EXPR_NONE();
        return;
    }

    scon_reg_t targetHint = SCON_EXPR_GET_TARGET(out);
    scon_list_iter_t iter = SCON_LIST_ITER_AT(&list->list, startIdx);
    scon_handle_t h;

    scon_uint32_t i = startIdx;
    while (scon_list_iter_next(&iter, &h))
    {
        scon_item_t* item = SCON_HANDLE_TO_ITEM(&h);

        if (i == list->length - 1)
        {
            if (targetHint != (scon_reg_t)-1)
            {
                scon_compile_build_into_target(compiler, item, targetHint);
                *out = SCON_EXPR_REG(targetHint);
            }
            else
            {
                scon_expr_build(compiler, item, out);
            }
        }
        else
        {
            scon_expr_t expr = SCON_EXPR_NONE();
            scon_expr_build(compiler, item, &expr);
            scon_expr_done(compiler, &expr);
        }
        i++;
    }
}

void scon_intrinsic_do(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_block_generic(compiler, list, 1, out);
}

void scon_intrinsic_lambda(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    scon_intrinsic_check_min_arity(compiler, list, 2, "lambda");

    scon_item_t* args = scon_list_nth_item(compiler->scon, &list->list, 1);
    if (args->type != SCON_ITEM_TYPE_LIST)
    {
        SCON_ERROR_COMPILE(compiler, args, "lambda expects a list of arguments, got %s",
            scon_item_type_str(args->type));
    }

    if (args->length > 255)
    {
        SCON_ERROR_COMPILE(compiler, args, "lambda expects at most 255 arguments, got %d", args->length);
    }

    scon_function_t* func = scon_function_new(compiler->scon);
    scon_item_t* funcItem = SCON_CONTAINER_OF(func, scon_item_t, function);
    funcItem->input = list->input;
    scon_const_slot_t slot = SCON_CONST_SLOT_ITEM(funcItem);
    scon_const_t funcConst = scon_function_lookup_constant(compiler->scon, compiler->function, &slot);

    func->arity = (scon_uint8_t)args->length;

    scon_compiler_t childCompiler;
    scon_compiler_init(&childCompiler, compiler->scon, func, compiler);

    scon_handle_t h;
    SCON_LIST_FOR_EACH(&h, &args->list)
    {
        scon_item_t* argName = SCON_HANDLE_TO_ITEM(&h);
        if (argName->type != SCON_ITEM_TYPE_ATOM)
        {
            SCON_ERROR_COMPILE(compiler, argName, "lambda expects a list of atoms as arguments, got %s",
                scon_item_type_str(argName->type));
        }
        scon_local_add_arg(&childCompiler, &argName->atom);
    }

    scon_expr_t bodyExpr = SCON_EXPR_NONE();
    scon_intrinsic_block_generic(&childCompiler, list, 2, &bodyExpr);
    scon_compile_return(&childCompiler, &bodyExpr);
    scon_expr_done(&childCompiler, &bodyExpr);

    scon_compiler_deinit(&childCompiler);

    scon_reg_t target = scon_expr_get_reg(compiler, out);
    scon_compile_closure(compiler, target, funcConst);

    for (scon_uint32_t i = 0; i < func->constantCount; i++)
    {
        if (func->constants[i].type == SCON_CONST_SLOT_ITEM)
        {
            continue;
        }
        scon_atom_t* captureName = func->constants[i].capture;
        scon_local_t* captured = scon_local_lookup(compiler, func->constants[i].capture);
        if (captured == SCON_NULL)
        {
            SCON_ERROR_COMPILE(compiler, SCON_CONTAINER_OF(captureName, scon_item_t, atom), "undefined variable '%s'",
                captureName->string);
        }

        if (!SCON_LOCAL_IS_DEFINED(captured))
        {
            scon_expr_t selfExpr = SCON_EXPR_REG(target);
            scon_compile_capture(compiler, target, i, &selfExpr);
            continue;
        }

        scon_compile_capture(compiler, target, i, &captured->expr);
    }

    *out = SCON_EXPR_REG(target);
}

void scon_intrinsic_thread(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    if (list->length < 2)
    {
        *out = SCON_EXPR_NONE();
        return;
    }

    scon_expr_t current = SCON_EXPR_NONE();
    scon_expr_build(compiler, scon_list_nth_item(compiler->scon, &list->list, 1), &current);

    scon_list_iter_t iter = SCON_LIST_ITER_AT(&list->list, 2);
    scon_handle_t h;
    while (scon_list_iter_next(&iter, &h))
    {
        scon_item_t* step = SCON_HANDLE_TO_ITEM(&h);
        scon_item_t* head;
        scon_uint32_t arity;

        if (step->type == SCON_ITEM_TYPE_LIST)
        {
            if (step->length == 0)
            {
                continue;
            }
            head = scon_list_nth_item(compiler->scon, &step->list, 0);
            arity = step->length;
        }
        else
        {
            head = step;
            arity = 1;
        }

        scon_reg_t base = scon_reg_alloc_range(compiler, arity);

        scon_compile_move(compiler, base, &current);
        scon_expr_done(compiler, &current);

        if (step->type == SCON_ITEM_TYPE_LIST)
        {
            for (scon_uint32_t j = 1; j < step->length; j++)
            {
                scon_reg_t argReg = (scon_reg_t)(base + j);
                scon_expr_t argExpr = SCON_EXPR_TARGET(argReg);
                scon_expr_build(compiler, scon_list_nth_item(compiler->scon, &step->list, j), &argExpr);

                if (argExpr.mode != SCON_MODE_REG || argExpr.reg != argReg)
                {
                    scon_compile_move(compiler, argReg, &argExpr);
                    scon_expr_done(compiler, &argExpr);
                }
            }
        }

        scon_expr_t callable = SCON_EXPR_NONE();
        scon_expr_build(compiler, head, &callable);
        scon_compile_call(compiler, base, &callable, arity);
        scon_expr_done(compiler, &callable);

        if (arity > 1)
        {
            scon_reg_free_range(compiler, (scon_reg_t)(base + 1), arity - 1);
        }

        current = SCON_EXPR_REG(base);
    }

    *out = current;
}

void scon_intrinsic_def(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    scon_intrinsic_check_arity(compiler, list, 2, "def");

    scon_item_t* name = scon_list_nth_item(compiler->scon, &list->list, 1);
    if (name->type != SCON_ITEM_TYPE_ATOM)
    {
        SCON_ERROR_COMPILE(compiler, name, "def expects an atom as the name, got %s", scon_item_type_str(name->type));
    }

    scon_local_t* local = scon_local_def(compiler, &name->atom);

    scon_expr_t valExpr = SCON_EXPR_NONE();
    scon_expr_build(compiler, scon_list_nth_item(compiler->scon, &list->list, 2), &valExpr);

    scon_local_def_done(compiler, local, &valExpr);
    scon_expr_done(compiler, &valExpr);

    *out = valExpr;
}

void scon_intrinsic_let(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    scon_intrinsic_check_min_arity(compiler, list, 1, "let");

    scon_item_t* bindings = scon_list_nth_item(compiler->scon, &list->list, 1);
    if (bindings->type != SCON_ITEM_TYPE_LIST)
    {
        SCON_ERROR_COMPILE(compiler, bindings, "let expects a list of bindings, got %s",
            scon_item_type_str(bindings->type));
    }

    scon_uint16_t initialLocalCount = compiler->localCount;

    scon_handle_t bh;
    SCON_LIST_FOR_EACH(&bh, &bindings->list)
    {
        scon_item_t* bindItem = SCON_HANDLE_TO_ITEM(&bh);
        if (bindItem->type != SCON_ITEM_TYPE_LIST || bindItem->length != 2)
        {
            SCON_ERROR_COMPILE(compiler, bindItem, "let binding must be a list of two items, got %s (length %u)",
                scon_item_type_str(bindItem->type), bindItem->length);
        }

        scon_item_t* name = scon_list_nth_item(compiler->scon, &bindItem->list, 0);
        if (name->type != SCON_ITEM_TYPE_ATOM)
        {
            SCON_ERROR_COMPILE(compiler, name, "let binding name must be an atom, got %s",
                scon_item_type_str(name->type));
        }

        scon_local_t* local = scon_local_def(compiler, &name->atom);

        scon_expr_t valExpr = SCON_EXPR_NONE();
        scon_expr_build(compiler, scon_list_nth_item(compiler->scon, &bindItem->list, 1), &valExpr);

        scon_local_def_done(compiler, local, &valExpr);

        scon_expr_done(compiler, &valExpr);
    }

    scon_intrinsic_block_generic(compiler, list, 2, out);

    for (scon_uint16_t i = initialLocalCount; i < compiler->localCount; i++)
    {
        if (compiler->locals[i].expr.mode == SCON_MODE_REG)
        {
            SCON_REG_CLEAR_LOCAL(compiler, compiler->locals[i].expr.reg);
            scon_reg_free(compiler, compiler->locals[i].expr.reg);
        }
    }

    compiler->localCount = initialLocalCount;
}

static inline scon_bool_t scon_expr_get_item(scon_compiler_t* compiler, scon_expr_t* expr, scon_handle_t* outItem)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(expr != SCON_NULL);
    SCON_ASSERT(outItem != SCON_NULL);

    if (expr->mode == SCON_MODE_CONST)
    {
        if (compiler->function->constants[expr->constant].type != SCON_CONST_SLOT_ITEM)
        {
            return SCON_FALSE;
        }

        *outItem = SCON_HANDLE_FROM_ITEM(compiler->function->constants[expr->constant].item);
        return SCON_TRUE;
    }
    return SCON_FALSE;
}

static inline scon_bool_t scon_expr_is_known_truthy(scon_compiler_t* compiler, scon_expr_t* expr, scon_bool_t* isTruthy)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(expr != SCON_NULL);
    SCON_ASSERT(isTruthy != SCON_NULL);

    scon_handle_t item;
    if (scon_expr_get_item(compiler, expr, &item))
    {
        *isTruthy = SCON_HANDLE_IS_TRUTHY(&item);
        return SCON_TRUE;
    }
    return SCON_FALSE;
}

static scon_item_t* scon_intrinsic_get_pair(scon_compiler_t* compiler, scon_handle_t* h, const char* name)
{
    scon_item_t* pair = SCON_HANDLE_TO_ITEM(h);
    if (pair->type != SCON_ITEM_TYPE_LIST || pair->length != 2)
    {
        SCON_ERROR_COMPILE(compiler, pair, "%s clauses must be lists of exactly two items, got %s (length %u)", name,
            scon_item_type_str(pair->type), pair->length);
    }
    return pair;
}

static scon_bool_t scon_fold_comparison(scon_t* scon, scon_opcode_t opBase, scon_handle_t left, scon_handle_t right,
    scon_bool_t* result)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(result != SCON_NULL);

    scon_int64_t cmp = scon_handle_compare(scon, &left, &right);
    switch (opBase)
    {
    case SCON_OPCODE_EQ:
        *result = (cmp == 0);
        return SCON_TRUE;
    case SCON_OPCODE_NEQ:
        *result = (cmp != 0);
        return SCON_TRUE;
    case SCON_OPCODE_SEQ:
        *result = scon_handle_is_equal(scon, &left, &right);
        return SCON_TRUE;
    case SCON_OPCODE_SNEQ:
        *result = !scon_handle_is_equal(scon, &left, &right);
        return SCON_TRUE;
    case SCON_OPCODE_LT:
        *result = (cmp < 0);
        return SCON_TRUE;
    case SCON_OPCODE_LE:
        *result = (cmp <= 0);
        return SCON_TRUE;
    case SCON_OPCODE_GT:
        *result = (cmp > 0);
        return SCON_TRUE;
    case SCON_OPCODE_GE:
        *result = (cmp >= 0);
        return SCON_TRUE;
    default:
        return SCON_FALSE;
    }
}

void scon_intrinsic_if(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    scon_intrinsic_check_arity_range(compiler, list, 2, 3, "if");

    scon_expr_t condExpr = SCON_EXPR_NONE();
    scon_expr_build(compiler, scon_list_nth_item(compiler->scon, &list->list, 1), &condExpr);

    scon_bool_t isTruthy;
    if (scon_expr_is_known_truthy(compiler, &condExpr, &isTruthy))
    {
        scon_expr_done(compiler, &condExpr);
        if (isTruthy)
        {
            scon_expr_build(compiler, scon_list_nth_item(compiler->scon, &list->list, 2), out);
        }
        else if (list->length == 4)
        {
            scon_expr_build(compiler, scon_list_nth_item(compiler->scon, &list->list, 3), out);
        }
        else
        {
            *out = SCON_EXPR_NIL(compiler);
        }
        return;
    }

    scon_reg_t target = scon_expr_get_reg(compiler, out);

    scon_reg_t condReg = scon_compile_move_or_alloc(compiler, &condExpr);
    scon_size_t jumpElse = scon_compile_jump(compiler, SCON_OPCODE_JMPF, condReg);
    scon_expr_done(compiler, &condExpr);

    scon_compile_build_into_target(compiler, scon_list_nth_item(compiler->scon, &list->list, 2), target);

    scon_size_t jumpEnd = 0;
    if (list->length == 4)
    {
        jumpEnd = scon_compile_jump(compiler, SCON_OPCODE_JMP, 0);
    }

    scon_compile_jump_patch(compiler, jumpElse);

    if (list->length == 4)
    {
        scon_compile_build_into_target(compiler, scon_list_nth_item(compiler->scon, &list->list, 3), target);
        scon_compile_jump_patch(compiler, jumpEnd);
    }
    else
    {
        scon_expr_t nilExpr = SCON_EXPR_NIL(compiler);
        scon_compile_move(compiler, target, &nilExpr);
    }

    *out = SCON_EXPR_REG(target);
}

static void scon_intrinsic_when_unless(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out,
    scon_opcode_t jumpOp, scon_expr_t defaultExpr)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    if (list->length < 2)
    {
        *out = SCON_EXPR_NONE();
        return;
    }

    scon_expr_t condExpr = SCON_EXPR_NONE();
    scon_expr_build(compiler, scon_list_nth_item(compiler->scon, &list->list, 1), &condExpr);

    scon_bool_t isTruthy;
    if (scon_expr_is_known_truthy(compiler, &condExpr, &isTruthy))
    {
        scon_expr_done(compiler, &condExpr);
        scon_bool_t shouldEval = (jumpOp == SCON_OPCODE_JMPF) ? isTruthy : !isTruthy;

        if (shouldEval)
        {
            scon_intrinsic_block_generic(compiler, list, 2, out);
        }
        else
        {
            *out = defaultExpr;
        }
        return;
    }

    scon_reg_t target = scon_expr_get_reg(compiler, out);
    scon_compile_move(compiler, target, &defaultExpr);

    scon_reg_t condReg = scon_compile_move_or_alloc(compiler, &condExpr);
    scon_size_t jumpEnd = scon_compile_jump(compiler, jumpOp, condReg);
    scon_expr_done(compiler, &condExpr);

    *out = SCON_EXPR_TARGET(target);
    scon_intrinsic_block_generic(compiler, list, 2, out);

    scon_compile_jump_patch(compiler, jumpEnd);
}

void scon_intrinsic_when(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_when_unless(compiler, list, out, SCON_OPCODE_JMPF, SCON_EXPR_FALSE(compiler));
}

void scon_intrinsic_unless(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_when_unless(compiler, list, out, SCON_OPCODE_JMPT, SCON_EXPR_NIL(compiler));
}

void scon_intrinsic_cond(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    if (list->length < 2)
    {
        *out = SCON_EXPR_NIL(compiler);
        return;
    }

    scon_reg_t targetHint = SCON_EXPR_GET_TARGET(out);
    scon_reg_t target = (scon_reg_t)-1;
    scon_size_t jumpsEnd[SCON_REGISTER_MAX];
    scon_size_t jumpCount = 0;
    scon_bool_t alwaysHit = SCON_FALSE;

    scon_list_iter_t iter = SCON_LIST_ITER_AT(&list->list, 1);
    scon_handle_t h;
    for (scon_uint32_t i = 1; i < list->length; i++)
    {
        scon_list_iter_next(&iter, &h);
        scon_item_t* pair = scon_intrinsic_get_pair(compiler, &h, "cond");

        scon_expr_t condExpr = SCON_EXPR_NONE();
        scon_expr_build(compiler, scon_list_nth_item(compiler->scon, &pair->list, 0), &condExpr);

        scon_bool_t isTruthy;
        if (scon_expr_is_known_truthy(compiler, &condExpr, &isTruthy))
        {
            scon_expr_done(compiler, &condExpr);
            if (!isTruthy)
            {
                continue;
            }

            if (target == (scon_reg_t)-1)
            {
                if (targetHint != (scon_reg_t)-1)
                {
                    *out = SCON_EXPR_TARGET(targetHint);
                }
                else
                {
                    *out = SCON_EXPR_NONE();
                }
                scon_expr_build(compiler, scon_list_nth_item(compiler->scon, &pair->list, 1), out);
                return;
            }

            scon_compile_build_into_target(compiler, scon_list_nth_item(compiler->scon, &pair->list, 1), target);
            alwaysHit = SCON_TRUE;
            break;
        }

        if (target == (scon_reg_t)-1)
        {
            target = (targetHint != (scon_reg_t)-1) ? targetHint : scon_reg_alloc(compiler);
        }

        scon_reg_t condReg = scon_compile_move_or_alloc(compiler, &condExpr);
        scon_size_t jumpNext = scon_compile_jump(compiler, SCON_OPCODE_JMPF, condReg);
        scon_expr_done(compiler, &condExpr);

        scon_compile_build_into_target(compiler, scon_list_nth_item(compiler->scon, &pair->list, 1), target);

        if (jumpCount >= SCON_REGISTER_MAX)
        {
            SCON_ERROR_COMPILE(compiler, list, "too many clauses in cond, got %u", jumpCount);
        }
        jumpsEnd[jumpCount++] = scon_compile_jump(compiler, SCON_OPCODE_JMP, 0);

        scon_compile_jump_patch(compiler, jumpNext);
    }

    if (target == (scon_reg_t)-1)
    {
        *out = SCON_EXPR_NIL(compiler);
        return;
    }

    if (!alwaysHit)
    {
        scon_expr_t nilConst = SCON_EXPR_NIL(compiler);
        scon_compile_move(compiler, target, &nilConst);
    }

    scon_compile_jump_patch_list(compiler, jumpsEnd, jumpCount);

    *out = SCON_EXPR_REG(target);
}

void scon_intrinsic_match(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    scon_intrinsic_check_min_arity(compiler, list, 2, "match");

    scon_expr_t targetExpr = SCON_EXPR_NONE();
    scon_expr_build(compiler, scon_list_nth_item(compiler->scon, &list->list, 1), &targetExpr);

    scon_handle_t targetItem;
    scon_bool_t targetKnown = scon_expr_get_item(compiler, &targetExpr, &targetItem);
    scon_reg_t targetReg = (scon_reg_t)-1;
    scon_reg_t resultReg = scon_expr_get_reg(compiler, out);

    scon_size_t jumpsEnd[SCON_REGISTER_MAX];
    scon_size_t jumpCount = 0;

    scon_list_iter_t iter = SCON_LIST_ITER_AT(&list->list, 2);
    scon_handle_t h;
    for (scon_uint32_t i = 2; i < list->length - 1; i++)
    {
        scon_list_iter_next(&iter, &h);
        scon_item_t* pair = scon_intrinsic_get_pair(compiler, &h, "match");
        scon_expr_t valExpr = SCON_EXPR_NONE();
        scon_expr_build(compiler, scon_list_nth_item(compiler->scon, &pair->list, 0), &valExpr);

        scon_handle_t valItem;
        if (targetKnown && scon_expr_get_item(compiler, &valExpr, &valItem))
        {
            scon_bool_t cmpResult = SCON_FALSE;
            if (scon_fold_comparison(compiler->scon, SCON_OPCODE_EQ, targetItem, valItem, &cmpResult))
            {
                scon_expr_done(compiler, &valExpr);
                if (!cmpResult)
                {
                    continue;
                }

                scon_compile_build_into_target(compiler, scon_list_nth_item(compiler->scon, &pair->list, 1), resultReg);
                scon_expr_done(compiler, &targetExpr);
                *out = SCON_EXPR_REG(resultReg);
                return;
            }
        }

        if (targetReg == (scon_reg_t)-1)
        {
            targetReg = scon_compile_move_or_alloc(compiler, &targetExpr);
        }

        scon_reg_t cmpResultReg = scon_reg_alloc(compiler);
        scon_compile_binary(compiler, SCON_OPCODE_EQ, cmpResultReg, targetReg, &valExpr);
        scon_expr_done(compiler, &valExpr);

        scon_size_t jumpNext = scon_compile_jump(compiler, SCON_OPCODE_JMPF, cmpResultReg);
        scon_reg_free(compiler, cmpResultReg);

        scon_compile_build_into_target(compiler, scon_list_nth_item(compiler->scon, &pair->list, 1), resultReg);
        if (SCON_UNLIKELY(jumpCount >= SCON_REGISTER_MAX))
        {
            SCON_ERROR_COMPILE(compiler, list, "too many clauses in match, limit is %u", SCON_REGISTER_MAX);
        }
        jumpsEnd[jumpCount++] = scon_compile_jump(compiler, SCON_OPCODE_JMP, 0);
        scon_compile_jump_patch(compiler, jumpNext);
    }

    scon_compile_build_into_target(compiler, scon_list_nth_item(compiler->scon, &list->list, list->length - 1),
        resultReg);

    scon_compile_jump_patch_list(compiler, jumpsEnd, jumpCount);

    scon_expr_done(compiler, &targetExpr);
    *out = SCON_EXPR_REG(resultReg);
}

static void scon_intrinsic_and_or(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out, scon_opcode_t jumpOp)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    if (list->length < 2)
    {
        *out = SCON_EXPR_FALSE(compiler);
        return;
    }

    scon_reg_t targetHint = SCON_EXPR_GET_TARGET(out);
    scon_reg_t target = (scon_reg_t)-1;
    scon_size_t jumps[SCON_REGISTER_MAX];
    scon_size_t jumpCount = 0;
    scon_uint32_t i = 1;

    scon_list_iter_t iter = SCON_LIST_ITER_AT(&list->list, 1);
    scon_handle_t h;
    while (scon_list_iter_next(&iter, &h))
    {
        scon_expr_t argExpr = SCON_EXPR_NONE();
        if (target != (scon_reg_t)-1)
        {
            argExpr = SCON_EXPR_TARGET(target);
        }

        scon_expr_build(compiler, SCON_HANDLE_TO_ITEM(&h), &argExpr);

        scon_bool_t isTruthy;
        if (scon_expr_is_known_truthy(compiler, &argExpr, &isTruthy))
        {
            scon_bool_t shortCircuits = (jumpOp == SCON_OPCODE_JMPT) ? isTruthy : !isTruthy;

            if (shortCircuits || i == list->length - 1)
            {
                if (target == (scon_reg_t)-1)
                {
                    *out = argExpr;
                    return;
                }

                scon_compile_move(compiler, target, &argExpr);
                scon_expr_done(compiler, &argExpr);
                break;
            }

            scon_expr_done(compiler, &argExpr);
            continue;
        }

        if (target == (scon_reg_t)-1)
        {
            target = (targetHint != (scon_reg_t)-1) ? targetHint : scon_reg_alloc(compiler);
        }

        if (argExpr.mode != SCON_MODE_REG || argExpr.reg != target)
        {
            scon_compile_move(compiler, target, &argExpr);
            scon_expr_done(compiler, &argExpr);
        }

        if (i != list->length - 1)
        {
            if (SCON_UNLIKELY(jumpCount >= SCON_REGISTER_MAX))
            {
                SCON_ERROR_COMPILE(compiler, list, "too many arguments for logical operator, limit is %u",
                    SCON_REGISTER_MAX);
            }
            jumps[jumpCount++] = scon_compile_jump(compiler, jumpOp, target);
        }
        i++;
    }

    scon_compile_jump_patch_list(compiler, jumps, jumpCount);

    *out = SCON_EXPR_REG(target);
}

void scon_intrinsic_and(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_and_or(compiler, list, out, SCON_OPCODE_JMPF);
}

void scon_intrinsic_or(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_and_or(compiler, list, out, SCON_OPCODE_JMPT);
}

void scon_intrinsic_not(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    scon_intrinsic_check_arity(compiler, list, 1, "not");

    scon_reg_t target = scon_expr_get_reg(compiler, out);

    scon_expr_t argExpr = SCON_EXPR_NONE();
    scon_expr_build(compiler, scon_list_nth_item(compiler->scon, &list->list, 1), &argExpr);

    scon_bool_t isTruthy;
    if (scon_expr_is_known_truthy(compiler, &argExpr, &isTruthy))
    {
        scon_expr_done(compiler, &argExpr);
        *out = isTruthy ? SCON_EXPR_FALSE(compiler) : SCON_EXPR_TRUE(compiler);
        return;
    }

    scon_reg_t argReg = scon_compile_move_or_alloc(compiler, &argExpr);
    scon_size_t jumpTrue = scon_compile_jump(compiler, SCON_OPCODE_JMPT, argReg);
    scon_expr_done(compiler, &argExpr);

    scon_expr_t trueExpr = SCON_EXPR_TRUE(compiler);
    scon_compile_move(compiler, target, &trueExpr);

    scon_size_t jumpEnd = scon_compile_jump(compiler, SCON_OPCODE_JMP, 0);

    scon_compile_jump_patch(compiler, jumpTrue);
    scon_expr_t falseExpr = SCON_EXPR_FALSE(compiler);
    scon_compile_move(compiler, target, &falseExpr);

    scon_compile_jump_patch(compiler, jumpEnd);

    *out = SCON_EXPR_REG(target);
}

static inline scon_atom_t* scon_fold_binary_calc(scon_compiler_t* compiler, scon_opcode_t op, scon_float_t lf,
    scon_float_t rf, scon_int64_t li, scon_int64_t ri, scon_bool_t isFloat)
{
    if (isFloat)
    {
        switch (op)
        {
        case SCON_OPCODE_ADD:
            return scon_atom_lookup_float(compiler->scon, lf + rf);
        case SCON_OPCODE_SUB:
            return scon_atom_lookup_float(compiler->scon, lf - rf);
        case SCON_OPCODE_MUL:
            return scon_atom_lookup_float(compiler->scon, lf * rf);
        case SCON_OPCODE_DIV:
            return (rf == 0.0) ? SCON_NULL : scon_atom_lookup_float(compiler->scon, lf / rf);
        default:
            return SCON_NULL;
        }
    }
    else
    {
        switch (op)
        {
        case SCON_OPCODE_ADD:
            return scon_atom_lookup_int(compiler->scon, li + ri);
        case SCON_OPCODE_SUB:
            return scon_atom_lookup_int(compiler->scon, li - ri);
        case SCON_OPCODE_MUL:
            return scon_atom_lookup_int(compiler->scon, li * ri);
        case SCON_OPCODE_DIV:
            return (ri == 0) ? SCON_NULL : scon_atom_lookup_int(compiler->scon, li / ri);
        case SCON_OPCODE_MOD:
            return (ri == 0) ? SCON_NULL : scon_atom_lookup_int(compiler->scon, li % ri);
        case SCON_OPCODE_BAND:
            return scon_atom_lookup_int(compiler->scon, li & ri);
        case SCON_OPCODE_BOR:
            return scon_atom_lookup_int(compiler->scon, li | ri);
        case SCON_OPCODE_BXOR:
            return scon_atom_lookup_int(compiler->scon, li ^ ri);
        case SCON_OPCODE_SHL:
            return (ri < 0 || ri >= 64) ? SCON_NULL : scon_atom_lookup_int(compiler->scon, li << ri);
        case SCON_OPCODE_SHR:
            return (ri < 0 || ri >= 64) ? SCON_NULL : scon_atom_lookup_int(compiler->scon, li >> ri);
        default:
            return SCON_NULL;
        }
    }
}

static scon_bool_t scon_fold_binary_expr(scon_compiler_t* compiler, scon_opcode_t opBase, scon_expr_t* leftExpr,
    scon_expr_t* rightExpr, scon_expr_t* outExpr)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(leftExpr != SCON_NULL);
    SCON_ASSERT(rightExpr != SCON_NULL);

    SCON_ASSERT(outExpr != SCON_NULL);
    if (leftExpr->mode != SCON_MODE_CONST || rightExpr->mode != SCON_MODE_CONST)
    {
        return SCON_FALSE;
    }

    if (compiler->function->constants[leftExpr->constant].type != SCON_CONST_SLOT_ITEM ||
        compiler->function->constants[rightExpr->constant].type != SCON_CONST_SLOT_ITEM)
    {
        return SCON_FALSE;
    }

    scon_item_t* leftItem = compiler->function->constants[leftExpr->constant].item;
    scon_item_t* rightItem = compiler->function->constants[rightExpr->constant].item;

    scon_bool_t isFloat =
        (leftItem->flags & SCON_ITEM_FLAG_FLOAT_SHAPED) || (rightItem->flags & SCON_ITEM_FLAG_FLOAT_SHAPED);

    scon_float_t lf = scon_item_get_float(leftItem);
    scon_float_t rf = scon_item_get_float(rightItem);
    scon_int64_t li = scon_item_get_int(leftItem);
    scon_int64_t ri = scon_item_get_int(rightItem);

    scon_atom_t* result = scon_fold_binary_calc(compiler, opBase, lf, rf, li, ri, isFloat);
    if (result == SCON_NULL)
    {
        return SCON_FALSE;
    }

    *outExpr = SCON_EXPR_CONST_ATOM(compiler, result);
    return SCON_TRUE;
}

void scon_intrinsic_binary_generic(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out, scon_opcode_t opBase)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    if (opBase == SCON_OPCODE_MOD || opBase == SCON_OPCODE_SHL || opBase == SCON_OPCODE_SHR)
    {
        scon_intrinsic_check_arity(compiler, list, 2, "operator");
    }
    else if (opBase >= SCON_OPCODE_BAND && opBase <= SCON_OPCODE_BXOR)
    {
        scon_intrinsic_check_min_arity(compiler, list, 2, "bitwise operator");
    }
    else
    {
        scon_intrinsic_check_min_arity(compiler, list, 1, "arithmetic operator");
    }

    scon_reg_t targetHint = SCON_EXPR_GET_TARGET(out);
    scon_expr_t leftExpr = SCON_EXPR_NONE();
    scon_expr_build(compiler, scon_list_nth_item(compiler->scon, &list->list, 1), &leftExpr);

    if (list->length == 2)
    {
        if (opBase == SCON_OPCODE_SUB || opBase == SCON_OPCODE_DIV)
        {
            scon_expr_t initialExpr = (opBase == SCON_OPCODE_SUB) ? SCON_EXPR_INT(compiler, 0) : SCON_EXPR_INT(compiler, 1);
            scon_expr_t foldedExpr;
            if (scon_fold_binary_expr(compiler, opBase, &initialExpr, &leftExpr, &foldedExpr))
            {
                scon_expr_done(compiler, &leftExpr);
                *out = foldedExpr;
                return;
            }

            scon_reg_t initialReg = scon_compile_move_or_alloc(compiler, &initialExpr);
            scon_reg_t target = (targetHint != (scon_reg_t)-1) ? targetHint : scon_reg_alloc(compiler);
            scon_compile_binary(compiler, opBase, target, initialReg, &leftExpr);

            scon_expr_done(compiler, &leftExpr);
            scon_expr_done(compiler, &initialExpr);
            *out = SCON_EXPR_REG(target);
            return;
        }

        *out = leftExpr;
        return;
    }

    scon_bool_t hasAccumulator = SCON_FALSE;
    scon_list_iter_t iter = SCON_LIST_ITER_AT(&list->list, 2);
    scon_handle_t h;
    while (scon_list_iter_next(&iter, &h))
    {
        scon_expr_t rightExpr = SCON_EXPR_NONE();
        scon_expr_build(compiler, SCON_HANDLE_TO_ITEM(&h), &rightExpr);

        scon_expr_t foldedExpr;
        if (scon_fold_binary_expr(compiler, opBase, &leftExpr, &rightExpr, &foldedExpr))
        {
            scon_expr_done(compiler, &leftExpr);
            scon_expr_done(compiler, &rightExpr);
            leftExpr = foldedExpr;
            continue;
        }

        if (!hasAccumulator)
        {
            if (leftExpr.mode != SCON_MODE_REG)
            {
                scon_compile_move_or_alloc(compiler, &leftExpr);
            }

            scon_reg_t target = (targetHint != (scon_reg_t)-1) ? targetHint : scon_reg_alloc(compiler);
            scon_compile_binary(compiler, opBase, target, leftExpr.reg, &rightExpr);
            scon_expr_done(compiler, &leftExpr);
            leftExpr = SCON_EXPR_REG(target);
            hasAccumulator = SCON_TRUE;
        }
        else
        {
            scon_compile_binary(compiler, opBase, leftExpr.reg, leftExpr.reg, &rightExpr);
        }

        scon_expr_done(compiler, &rightExpr);
    }

    *out = leftExpr;
}

void scon_intrinsic_add(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_binary_generic(compiler, list, out, SCON_OPCODE_ADD);
}

void scon_intrinsic_sub(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_binary_generic(compiler, list, out, SCON_OPCODE_SUB);
}

void scon_intrinsic_mul(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_binary_generic(compiler, list, out, SCON_OPCODE_MUL);
}

void scon_intrinsic_div(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_binary_generic(compiler, list, out, SCON_OPCODE_DIV);
}

void scon_intrinsic_mod(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_binary_generic(compiler, list, out, SCON_OPCODE_MOD);
}

static void scon_intrinsic_unary_op_generic(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out,
    scon_opcode_t op, scon_expr_t rightExpr, const char* name)
{
    scon_intrinsic_check_arity(compiler, list, 1, name);

    scon_expr_t leftExpr = SCON_EXPR_NONE();
    scon_expr_build(compiler, scon_list_nth_item(compiler->scon, &list->list, 1), &leftExpr);

    scon_expr_t foldedExpr;
    if (scon_fold_binary_expr(compiler, op, &leftExpr, &rightExpr, &foldedExpr))
    {
        scon_expr_done(compiler, &leftExpr);
        scon_expr_done(compiler, &rightExpr);
        *out = foldedExpr;
        return;
    }

    scon_reg_t target = scon_expr_get_reg(compiler, out);
    scon_compile_binary(compiler, op, target, scon_compile_move_or_alloc(compiler, &leftExpr), &rightExpr);
    scon_expr_done(compiler, &leftExpr);
    scon_expr_done(compiler, &rightExpr);
    *out = SCON_EXPR_REG(target);
}

void scon_intrinsic_inc(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_unary_op_generic(compiler, list, out, SCON_OPCODE_ADD, SCON_EXPR_INT(compiler, 1), "inc");
}

void scon_intrinsic_dec(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_unary_op_generic(compiler, list, out, SCON_OPCODE_SUB, SCON_EXPR_INT(compiler, 1), "dec");
}

void scon_intrinsic_bit_and(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_binary_generic(compiler, list, out, SCON_OPCODE_BAND);
}

void scon_intrinsic_bit_or(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_binary_generic(compiler, list, out, SCON_OPCODE_BOR);
}

void scon_intrinsic_bit_xor(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_binary_generic(compiler, list, out, SCON_OPCODE_BXOR);
}

void scon_intrinsic_bit_not(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    scon_intrinsic_check_arity(compiler, list, 1, "bitwise not");

    scon_expr_t argExpr = SCON_EXPR_NONE();
    scon_expr_build(compiler, scon_list_nth_item(compiler->scon, &list->list, 1), &argExpr);

    if (argExpr.mode == SCON_MODE_CONST && compiler->function->constants[argExpr.constant].type == SCON_CONST_SLOT_ITEM)
    {
        scon_item_t* argItem = compiler->function->constants[argExpr.constant].item;
        if (argItem->flags & SCON_ITEM_FLAG_INT_SHAPED)
        {
            scon_atom_t* result = scon_atom_lookup_int(compiler->scon, ~argItem->atom.integerValue);
            scon_expr_done(compiler, &argExpr);
            *out = SCON_EXPR_CONST_ATOM(compiler, result);
            return;
        }
    }

    scon_reg_t target = scon_expr_get_reg(compiler, out);
    scon_compile_inst(compiler,
        SCON_INST_MAKE_ABC((scon_opcode_t)(SCON_OPCODE_BNOT | argExpr.mode), target, 0, argExpr.value));
    scon_expr_done(compiler, &argExpr);
    *out = SCON_EXPR_REG(target);
}

void scon_intrinsic_bit_shl(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_binary_generic(compiler, list, out, SCON_OPCODE_SHL);
}

void scon_intrinsic_bit_shr(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_binary_generic(compiler, list, out, SCON_OPCODE_SHR);
}

static void scon_intrinsic_comparison_generic(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out,
    scon_opcode_t opBase)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    scon_intrinsic_check_min_arity(compiler, list, 2, "comparison");

    scon_reg_t targetHint = SCON_EXPR_GET_TARGET(out);
    scon_expr_t leftExpr = SCON_EXPR_NONE();
    scon_expr_build(compiler, scon_list_nth_item(compiler->scon, &list->list, 1), &leftExpr);

    scon_reg_t target = (scon_reg_t)-1;
    scon_size_t jumps[SCON_REGISTER_MAX];
    scon_size_t jumpCount = 0;
    scon_uint32_t i = 2;

    scon_list_iter_t iter = SCON_LIST_ITER_AT(&list->list, 2);
    scon_handle_t h;
    while (scon_list_iter_next(&iter, &h))
    {
        scon_expr_t rightExpr = SCON_EXPR_NONE();
        scon_expr_build(compiler, SCON_HANDLE_TO_ITEM(&h), &rightExpr);

        scon_handle_t leftItem, rightItem;
        if (scon_expr_get_item(compiler, &leftExpr, &leftItem) && scon_expr_get_item(compiler, &rightExpr, &rightItem))
        {
            scon_bool_t cmpResult = SCON_FALSE;
            if (scon_fold_comparison(compiler->scon, opBase, leftItem, rightItem, &cmpResult))
            {
                if (cmpResult)
                {
                    scon_expr_done(compiler, &leftExpr);
                    leftExpr = rightExpr;
                    continue;
                }
                else
                {
                    scon_expr_done(compiler, &leftExpr);
                    scon_expr_done(compiler, &rightExpr);

                    if (jumpCount > 0)
                    {
                        scon_expr_t falseExpr = SCON_EXPR_FALSE(compiler);
                        scon_compile_move(compiler, target, &falseExpr);
                        scon_compile_jump_patch_list(compiler, jumps, jumpCount);
                        *out = SCON_EXPR_REG(target);
                    }
                    else
                    {
                        if (target != (scon_reg_t)-1)
                        {
                            scon_reg_free(compiler, target);
                        }
                        *out = SCON_EXPR_FALSE(compiler);
                    }
                    return;
                }
            }
        }

        if (target == (scon_reg_t)-1)
        {
            target = (targetHint != (scon_reg_t)-1) ? targetHint : scon_reg_alloc(compiler);
        }

        if (leftExpr.mode != SCON_MODE_REG)
        {
            scon_compile_move_or_alloc(compiler, &leftExpr);
        }

        scon_compile_binary(compiler, opBase, target, leftExpr.reg, &rightExpr);

        if (i != list->length - 1)
        {
            if (jumpCount >= SCON_REGISTER_MAX)
            {
                SCON_ERROR_COMPILE(compiler, list, "comparison expects at most 256 arguments, got %u", jumpCount);
            }
            jumps[jumpCount++] = scon_compile_jump(compiler, SCON_OPCODE_JMPF, target);

            scon_expr_done(compiler, &leftExpr);
            leftExpr = rightExpr;
        }
        else
        {
            scon_expr_done(compiler, &leftExpr);
            scon_expr_done(compiler, &rightExpr);
            leftExpr = SCON_EXPR_NONE();
        }
        i++;
    }

    scon_expr_done(compiler, &leftExpr);

    if (jumpCount > 0)
    {
        scon_compile_jump_patch_list(compiler, jumps, jumpCount);
        *out = SCON_EXPR_REG(target);
    }
    else if (target == (scon_reg_t)-1)
    {
        *out = SCON_EXPR_TRUE(compiler);
    }
    else
    {
        *out = SCON_EXPR_REG(target);
    }
}

void scon_intrinsic_equal(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_comparison_generic(compiler, list, out, SCON_OPCODE_EQ);
}

void scon_intrinsic_strict_equal(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_comparison_generic(compiler, list, out, SCON_OPCODE_SEQ);
}

void scon_intrinsic_not_equal(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_comparison_generic(compiler, list, out, SCON_OPCODE_NEQ);
}

void scon_intrinsic_strict_not_equal(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_comparison_generic(compiler, list, out, SCON_OPCODE_SNEQ);
}

void scon_intrinsic_less(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_comparison_generic(compiler, list, out, SCON_OPCODE_LT);
}

void scon_intrinsic_less_equal(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_comparison_generic(compiler, list, out, SCON_OPCODE_LE);
}

void scon_intrinsic_greater(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_comparison_generic(compiler, list, out, SCON_OPCODE_GT);
}

void scon_intrinsic_greater_equal(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_comparison_generic(compiler, list, out, SCON_OPCODE_GE);
}

scon_intrinsic_handler_t sconIntrinsicHandlers[SCON_INTRINSIC_MAX] = {
    [SCON_INTRINSIC_NONE] = SCON_NULL,

    [SCON_INTRINSIC_QUOTE] = scon_intrinsic_quote,
    [SCON_INTRINSIC_LIST] = scon_intrinsic_list,
    [SCON_INTRINSIC_DO] = scon_intrinsic_do,
    [SCON_INTRINSIC_LAMBDA] = scon_intrinsic_lambda,
    [SCON_INTRINSIC_THREAD] = scon_intrinsic_thread,

    [SCON_INTRINSIC_DEF] = scon_intrinsic_def,
    [SCON_INTRINSIC_LET] = scon_intrinsic_let,

    [SCON_INTRINSIC_IF] = scon_intrinsic_if,
    [SCON_INTRINSIC_WHEN] = scon_intrinsic_when,
    [SCON_INTRINSIC_UNLESS] = scon_intrinsic_unless,
    [SCON_INTRINSIC_COND] = scon_intrinsic_cond,
    [SCON_INTRINSIC_MATCH] = scon_intrinsic_match,
    [SCON_INTRINSIC_AND] = scon_intrinsic_and,
    [SCON_INTRINSIC_OR] = scon_intrinsic_or,
    [SCON_INTRINSIC_NOT] = scon_intrinsic_not,

    [SCON_INTRINSIC_ADD] = scon_intrinsic_add,
    [SCON_INTRINSIC_SUB] = scon_intrinsic_sub,
    [SCON_INTRINSIC_MUL] = scon_intrinsic_mul,
    [SCON_INTRINSIC_DIV] = scon_intrinsic_div,
    [SCON_INTRINSIC_MOD] = scon_intrinsic_mod,
    [SCON_INTRINSIC_INC] = scon_intrinsic_inc,
    [SCON_INTRINSIC_DEC] = scon_intrinsic_dec,
    [SCON_INTRINSIC_BAND] = scon_intrinsic_bit_and,
    [SCON_INTRINSIC_BOR] = scon_intrinsic_bit_or,
    [SCON_INTRINSIC_BXOR] = scon_intrinsic_bit_xor,
    [SCON_INTRINSIC_BNOT] = scon_intrinsic_bit_not,
    [SCON_INTRINSIC_SHL] = scon_intrinsic_bit_shl,
    [SCON_INTRINSIC_SHR] = scon_intrinsic_bit_shr,

    [SCON_INTRINSIC_EQ] = scon_intrinsic_equal,
    [SCON_INTRINSIC_NEQ] = scon_intrinsic_not_equal,
    [SCON_INTRINSIC_SNEQ] = scon_intrinsic_strict_not_equal,
    [SCON_INTRINSIC_SEQ] = scon_intrinsic_strict_equal,
    [SCON_INTRINSIC_LT] = scon_intrinsic_less,
    [SCON_INTRINSIC_LE] = scon_intrinsic_less_equal,
    [SCON_INTRINSIC_GT] = scon_intrinsic_greater,
    [SCON_INTRINSIC_GE] = scon_intrinsic_greater_equal,
};

const char* sconIntrinsics[SCON_INTRINSIC_MAX] = {
    [SCON_INTRINSIC_NONE] = "",
    [SCON_INTRINSIC_QUOTE] = "quote",
    [SCON_INTRINSIC_LIST] = "list",
    [SCON_INTRINSIC_DO] = "do",
    [SCON_INTRINSIC_DEF] = "def",
    [SCON_INTRINSIC_LAMBDA] = "lambda",
    [SCON_INTRINSIC_THREAD] = "->",
    [SCON_INTRINSIC_LET] = "let",
    [SCON_INTRINSIC_IF] = "if",
    [SCON_INTRINSIC_WHEN] = "when",
    [SCON_INTRINSIC_UNLESS] = "unless",
    [SCON_INTRINSIC_COND] = "cond",
    [SCON_INTRINSIC_MATCH] = "match",
    [SCON_INTRINSIC_AND] = "and",
    [SCON_INTRINSIC_OR] = "or",
    [SCON_INTRINSIC_NOT] = "not",
    [SCON_INTRINSIC_ADD] = "+",
    [SCON_INTRINSIC_SUB] = "-",
    [SCON_INTRINSIC_MUL] = "*",
    [SCON_INTRINSIC_DIV] = "/",
    [SCON_INTRINSIC_MOD] = "%",
    [SCON_INTRINSIC_INC] = "++",
    [SCON_INTRINSIC_DEC] = "--",
    [SCON_INTRINSIC_SEQ] = "eq?",
    [SCON_INTRINSIC_SNEQ] = "ne?",
    [SCON_INTRINSIC_EQ] = "==",
    [SCON_INTRINSIC_NEQ] = "!=",
    [SCON_INTRINSIC_LT] = "<",
    [SCON_INTRINSIC_LE] = "<=",
    [SCON_INTRINSIC_GT] = ">",
    [SCON_INTRINSIC_GE] = ">=",
    [SCON_INTRINSIC_BAND] = "&",
    [SCON_INTRINSIC_BOR] = "|",
    [SCON_INTRINSIC_BXOR] = "^",
    [SCON_INTRINSIC_BNOT] = "~",
    [SCON_INTRINSIC_SHL] = "<<",
    [SCON_INTRINSIC_SHR] = ">>",
};

#define SCON_INTRINSIC_NATIVE_ARITH(_name, _op, _identity) \
    static scon_handle_t scon_intrinsic_native_##_name(scon_t* scon, scon_size_t argc, scon_handle_t* argv) \
    { \
        if (argc == 0) \
        { \
            return SCON_HANDLE_FROM_INT(_identity); \
        } \
        if (argc == 1) \
        { \
            scon_handle_t res; \
            scon_handle_t id = SCON_HANDLE_FROM_INT(_identity); \
            SCON_HANDLE_ARITHMETIC_FAST(scon, &res, &id, &argv[0], _op); \
            return res; \
        } \
        scon_handle_t res = argv[0]; \
        for (scon_size_t i = 1; i < argc; i++) \
        { \
            SCON_HANDLE_ARITHMETIC_FAST(scon, &res, &res, &argv[i], _op); \
        } \
        return res; \
    }

#define SCON_INTRINSIC_NATIVE_LOGIC(_name, _short_circuit_truth) \
    static scon_handle_t scon_intrinsic_native_##_name(scon_t* scon, scon_size_t argc, scon_handle_t* argv) \
    { \
        if (argc == 0) \
        { \
            return SCON_HANDLE_FALSE(); \
        } \
        scon_handle_t res = argv[0]; \
        for (scon_size_t i = 0; i < argc; i++) \
        { \
            res = argv[i]; \
            if (SCON_HANDLE_IS_TRUTHY(&res) == (_short_circuit_truth)) \
            { \
                return res; \
            } \
        } \
        return res; \
    }

#define SCON_INTRINSIC_NATIVE_BITWISE(_name, _op) \
    static scon_handle_t scon_intrinsic_native_##_name(scon_t* scon, scon_size_t argc, scon_handle_t* argv) \
    { \
        scon_error_check_min_arity(scon, argc, 2, #_op); \
        scon_int64_t res = scon_get_int(scon, &argv[0]); \
        for (scon_size_t i = 1; i < argc; i++) \
        { \
            res _op## = scon_get_int(scon, &argv[i]); \
        } \
        return SCON_HANDLE_FROM_INT(res); \
    }

#define SCON_INTRINSIC_NATIVE_COMPARE(_name, _op) \
    static scon_handle_t scon_intrinsic_native_##_name(scon_t* scon, scon_size_t argc, scon_handle_t* argv) \
    { \
        if (argc < 2) \
        { \
            return SCON_HANDLE_TRUE(); \
        } \
        for (scon_size_t i = 0; i < argc - 1; i++) \
        { \
            if (!(scon_handle_compare(scon, &argv[i], &argv[i + 1]) _op 0)) \
            { \
                return SCON_HANDLE_FALSE(); \
            } \
        } \
        return SCON_HANDLE_TRUE(); \
    }

#define SCON_INTRINSIC_NATIVE_COMPARE_STRICT(_name, _expected) \
    static scon_handle_t scon_intrinsic_native_##_name(scon_t* scon, scon_size_t argc, scon_handle_t* argv) \
    { \
        if (argc < 2) \
        { \
            return SCON_HANDLE_TRUE(); \
        } \
        for (scon_size_t i = 0; i < argc - 1; i++) \
        { \
            if (scon_handle_is_equal(scon, &argv[i], &argv[i + 1]) != (_expected)) \
            { \
                return SCON_HANDLE_FALSE(); \
            } \
        } \
        return SCON_HANDLE_TRUE(); \
    }

SCON_INTRINSIC_NATIVE_ARITH(add, +, 0)
SCON_INTRINSIC_NATIVE_ARITH(mul, *, 1)
SCON_INTRINSIC_NATIVE_ARITH(sub, -, 0)
SCON_INTRINSIC_NATIVE_ARITH(div, /, 1)

SCON_INTRINSIC_NATIVE_BITWISE(band, &)
SCON_INTRINSIC_NATIVE_BITWISE(bor, |)
SCON_INTRINSIC_NATIVE_BITWISE(bxor, ^)

SCON_INTRINSIC_NATIVE_COMPARE(eq, ==)
SCON_INTRINSIC_NATIVE_COMPARE(neq, !=)
SCON_INTRINSIC_NATIVE_COMPARE(lt, <)
SCON_INTRINSIC_NATIVE_COMPARE(le, <=)
SCON_INTRINSIC_NATIVE_COMPARE(gt, >)
SCON_INTRINSIC_NATIVE_COMPARE(ge, >=)

SCON_INTRINSIC_NATIVE_COMPARE_STRICT(seq, SCON_TRUE)
SCON_INTRINSIC_NATIVE_COMPARE_STRICT(sneq, SCON_FALSE)

SCON_INTRINSIC_NATIVE_LOGIC(and, SCON_FALSE)
SCON_INTRINSIC_NATIVE_LOGIC(or, SCON_TRUE)

static scon_handle_t scon_intrinsic_native_list(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    scon_list_t* list = scon_list_new(scon);
    for (scon_size_t i = 0; i < argc; i++)
    {
        scon_list_append(scon, list, argv[i]);
    }
    return SCON_HANDLE_FROM_LIST(list);
}

static scon_handle_t scon_intrinsic_native_mod(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    scon_error_check_arity(scon, argc, 2, "%");
    scon_promotion_t prom;
    scon_handle_promote(scon, &argv[0], &argv[1], &prom);
    if (prom.type != SCON_PROMOTION_TYPE_INT)
    {
        SCON_ERROR_RUNTIME(scon, "%% expects integer arguments");
    }
    if (prom.b.intVal == 0)
    {
        SCON_ERROR_RUNTIME(scon, "modulo by zero");
    }
    return SCON_HANDLE_FROM_INT(prom.a.intVal % prom.b.intVal);
}

static scon_handle_t scon_intrinsic_native_inc(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    scon_error_check_arity(scon, argc, 1, "++");
    scon_handle_t res;
    scon_handle_t one = SCON_HANDLE_FROM_INT(1);
    SCON_HANDLE_ARITHMETIC_FAST(scon, &res, &argv[0], &one, +);
    return res;
}

static scon_handle_t scon_intrinsic_native_dec(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    scon_error_check_arity(scon, argc, 1, "--");
    scon_handle_t res;
    scon_handle_t one = SCON_HANDLE_FROM_INT(1);
    SCON_HANDLE_ARITHMETIC_FAST(scon, &res, &argv[0], &one, -);
    return res;
}

static scon_handle_t scon_intrinsic_native_bnot(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    scon_error_check_arity(scon, argc, 1, "~");
    return SCON_HANDLE_FROM_INT(~scon_get_int(scon, &argv[0]));
}

static scon_handle_t scon_intrinsic_native_shl(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    scon_error_check_arity(scon, argc, 2, "<<");
    scon_int64_t left = scon_get_int(scon, &argv[0]);
    scon_int64_t right = scon_get_int(scon, &argv[1]);
    if (right < 0 || right >= 64)
    {
        SCON_ERROR_RUNTIME(scon, "shift amount out of range");
    }
    return SCON_HANDLE_FROM_INT(left << right);
}

static scon_handle_t scon_intrinsic_native_shr(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    scon_error_check_arity(scon, argc, 2, ">>");
    scon_int64_t left = scon_get_int(scon, &argv[0]);
    scon_int64_t right = scon_get_int(scon, &argv[1]);
    if (right < 0 || right >= 64)
    {
        SCON_ERROR_RUNTIME(scon, "shift amount out of range");
    }
    return SCON_HANDLE_FROM_INT(left >> right);
}

static scon_handle_t scon_intrinsic_native_do(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc == 0)
    {
        return scon_handle_nil(scon);
    }
    return argv[argc - 1];
}

static scon_handle_t scon_intrinsic_native_not(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    scon_error_check_arity(scon, argc, 1, "not");
    return SCON_HANDLE_IS_TRUTHY(&argv[0]) ? SCON_HANDLE_FALSE() : SCON_HANDLE_TRUE();
}

scon_native_fn sconIntrinsicNatives[SCON_INTRINSIC_MAX] = {
    [SCON_INTRINSIC_LIST] = scon_intrinsic_native_list,
    [SCON_INTRINSIC_DO] = scon_intrinsic_native_do,
    [SCON_INTRINSIC_AND] = scon_intrinsic_native_and,
    [SCON_INTRINSIC_OR] = scon_intrinsic_native_or,
    [SCON_INTRINSIC_NOT] = scon_intrinsic_native_not,
    [SCON_INTRINSIC_ADD] = scon_intrinsic_native_add,
    [SCON_INTRINSIC_SUB] = scon_intrinsic_native_sub,
    [SCON_INTRINSIC_MUL] = scon_intrinsic_native_mul,
    [SCON_INTRINSIC_DIV] = scon_intrinsic_native_div,
    [SCON_INTRINSIC_MOD] = scon_intrinsic_native_mod,
    [SCON_INTRINSIC_INC] = scon_intrinsic_native_inc,
    [SCON_INTRINSIC_DEC] = scon_intrinsic_native_dec,
    [SCON_INTRINSIC_BAND] = scon_intrinsic_native_band,
    [SCON_INTRINSIC_BOR] = scon_intrinsic_native_bor,
    [SCON_INTRINSIC_BXOR] = scon_intrinsic_native_bxor,
    [SCON_INTRINSIC_BNOT] = scon_intrinsic_native_bnot,
    [SCON_INTRINSIC_SHL] = scon_intrinsic_native_shl,
    [SCON_INTRINSIC_SHR] = scon_intrinsic_native_shr,
    [SCON_INTRINSIC_EQ] = scon_intrinsic_native_eq,
    [SCON_INTRINSIC_NEQ] = scon_intrinsic_native_neq,
    [SCON_INTRINSIC_SEQ] = scon_intrinsic_native_seq,
    [SCON_INTRINSIC_SNEQ] = scon_intrinsic_native_sneq,
    [SCON_INTRINSIC_LT] = scon_intrinsic_native_lt,
    [SCON_INTRINSIC_LE] = scon_intrinsic_native_le,
    [SCON_INTRINSIC_GT] = scon_intrinsic_native_gt,
    [SCON_INTRINSIC_GE] = scon_intrinsic_native_ge,
};

static inline void scon_intrinsic_register(scon_t* scon, scon_intrinsic_t intrinsic)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(intrinsic > SCON_INTRINSIC_NONE && intrinsic < SCON_INTRINSIC_MAX);

    const char* str = sconIntrinsics[intrinsic];
    scon_size_t len = SCON_STRLEN(str);
    scon_atom_t* atom = scon_atom_lookup(scon, str, len, SCON_ATOM_LOOKUP_NONE);
    atom->intrinsic = intrinsic;
    atom->native = sconIntrinsicNatives[intrinsic];
    scon_item_t* item = SCON_CONTAINER_OF(atom, scon_item_t, atom);
    item->flags |= SCON_ITEM_FLAG_INTRINSIC;
    if (atom->native != SCON_NULL)
    {
        item->flags |= SCON_ITEM_FLAG_NATIVE;
    }
    item->flags &= ~(SCON_ITEM_FLAG_INT_SHAPED | SCON_ITEM_FLAG_FLOAT_SHAPED);
    SCON_GC_RETAIN_ITEM(scon, item);
}

SCON_API void scon_intrinsic_register_all(scon_t* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    for (scon_size_t i = SCON_INTRINSIC_NONE + 1; i < SCON_INTRINSIC_MAX; i++)
    {
        scon_intrinsic_register(scon, (scon_intrinsic_t)i);
    }
}

#endif
