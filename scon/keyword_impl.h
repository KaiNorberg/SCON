#include "item.h"
#ifndef SCON_KEYWORD_IMPL_H
#define SCON_KEYWORD_IMPL_H 1

#include "compile.h"
#include "keyword.h"

static void scon_keyword_quote(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    if (list->length != 2)
    {
        SCON_THROW_ITEM(compiler->scon, "quote requires exactly one argument", list);
    }

    scon_const_slot_t slot = SCON_CONST_SLOT_ITEM(list->list.items[1]);
    *out = SCON_EXPR_CONST(scon_function_lookup_constant(compiler->scon, compiler->function, &slot));
}

static void scon_keyword_list(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_reg_t target = scon_reg_alloc(compiler);
    scon_emit_list(compiler, target);

    for (scon_uint32_t i = 1; i < list->length; i++)
    {
        scon_expr_t argExpr;
        scon_expr_compile(compiler, list->list.items[i], &argExpr);

        scon_emit_append(compiler, target, &argExpr);

        scon_expr_done(compiler, &argExpr);
    }

    *out = SCON_EXPR_REG(target);
}

static void scon_compile_block(scon_compiler_t* compiler, scon_item_t* list, scon_uint32_t startIdx, scon_expr_t* out)
{
    if (startIdx >= list->length)
    {
        *out = SCON_EXPR_NONE();
        return;
    }

    for (scon_uint32_t i = startIdx; i < list->length; i++)
    {
        scon_expr_compile(compiler, list->list.items[i], out);
        if (i != list->length - 1)
        {
            scon_expr_done(compiler, out);
        }
    }
}

static void scon_keyword_do(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_compile_block(compiler, list, 1, out);
}

static void scon_keyword_lambda(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    if (list->length < 3)
    {
        SCON_THROW_ITEM(compiler->scon, "lambda requires at least an argument list and a body", list);
    }

    scon_item_t* args = list->list.items[1];
    if (args->type != SCON_ITEM_TYPE_LIST)
    {
        SCON_THROW_ITEM(compiler->scon, "lambda arguments must be a list", args);
    }

    if (args->length > 255)
    {
        SCON_THROW_ITEM(compiler->scon, "too many arguments for lambda", args);
    }

    scon_function_t* func = scon_function_new(compiler->scon);
    func->arity = (scon_uint8_t)args->length;

    scon_compiler_t childCompiler;
    scon_compiler_init(&childCompiler, compiler->scon, func, compiler);

    for (scon_uint32_t i = 0; i < args->length; i++)
    {
        scon_item_t* argName = args->list.items[i];
        if (argName->type != SCON_ITEM_TYPE_ATOM)
        {
            SCON_THROW_ITEM(compiler->scon, "lambda argument must be an atom", argName);
        }
        scon_local_add_arg(&childCompiler, &argName->atom);
    }

    scon_expr_t bodyExpr;
    scon_compile_block(&childCompiler, list, 2, &bodyExpr);
    scon_emit_return(&childCompiler, &bodyExpr);
    scon_expr_done(&childCompiler, &bodyExpr);

    scon_compiler_deinit(&childCompiler);

    scon_const_slot_t slot = SCON_CONST_SLOT_ITEM(SCON_CONTAINER_OF(func, scon_item_t, function));
    scon_const_t funcConst = scon_function_lookup_constant(compiler->scon, compiler->function, &slot);

    scon_reg_t target = scon_reg_alloc(compiler);
    scon_emit_closure(compiler, target, funcConst);

    for (scon_uint32_t i = 0; i < func->constantCount; i++)
    {
        if (func->constants[i].type == SCON_CONST_SLOT_ITEM)
        {
            continue;
        }
        scon_local_t* local = scon_local_lookup(compiler, func->constants[i].capture);

        if (local->expr.mode == SCON_MODE_SELF)
        {
            scon_expr_t selfExpr = SCON_EXPR_REG(target);
            scon_emit_capture(compiler, target, i, &selfExpr);
        }
        else
        {
            scon_emit_capture(compiler, target, i, &local->expr);
        }
    }

    *out = SCON_EXPR_REG(target);
}

static void scon_keyword_def(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    if (list->length != 3)
    {
        SCON_THROW_ITEM(compiler->scon, "def requires exactly a name and a value", list);
    }

    scon_item_t* name = list->list.items[1];
    if (name->type != SCON_ITEM_TYPE_ATOM)
    {
        SCON_THROW_ITEM(compiler->scon, "def name must be an atom", name);
    }

    scon_expr_t tempExpr = SCON_EXPR_SELF();
    scon_local_t* local = scon_local_add(compiler, &name->atom, &tempExpr);
    scon_expr_done(compiler, &tempExpr);

    scon_expr_t valExpr;
    scon_expr_compile(compiler, list->list.items[2], &valExpr);

    local->expr = valExpr;

    scon_expr_done(compiler, &valExpr);

    *out = valExpr;
}

static void scon_keyword_let(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    if (list->length < 2)
    {
        SCON_THROW_ITEM(compiler->scon, "let requires at least a bindings list", list);
    }

    scon_item_t* bindings = list->list.items[1];
    if (bindings->type != SCON_ITEM_TYPE_LIST)
    {
        SCON_THROW_ITEM(compiler->scon, "let bindings must be a list", bindings);
    }

    for (scon_uint32_t i = 0; i < bindings->length; i++)
    {
        scon_item_t* bindNode = bindings->list.items[i];
        if (bindNode->type != SCON_ITEM_TYPE_LIST || bindNode->length != 2)
        {
            SCON_THROW_ITEM(compiler->scon, "let binding must be a list of two items", bindNode);
        }

        scon_item_t* name = bindNode->list.items[0];
        if (name->type != SCON_ITEM_TYPE_ATOM)
        {
            SCON_THROW_ITEM(compiler->scon, "let binding name must be an atom", name);
        }

        scon_expr_t valExpr;
        scon_expr_compile(compiler, bindNode->list.items[1], &valExpr);

        scon_local_add(compiler, &name->atom, &valExpr);

        scon_expr_done(compiler, &valExpr);
    }

    scon_expr_t lastExpr = SCON_EXPR_NONE();

    for (scon_uint32_t i = 2; i < list->length; i++)
    {
        scon_expr_compile(compiler, list->list.items[i], &lastExpr);
        if (i != list->length - 1)
        {
            scon_expr_done(compiler, &lastExpr);
        }
    }

    *out = lastExpr;
}

static inline scon_bool_t scon_expr_is_known_truthy(scon_compiler_t* compiler, scon_expr_t* expr, scon_bool_t* isTruthy)
{
    if (expr->mode == SCON_MODE_CONST)
    {
        if (compiler->function->constants[expr->constant].type != SCON_CONST_SLOT_ITEM)
        {
            return SCON_FALSE;
        }

        scon_handle_t item = SCON_HANDLE_FROM_ITEM(compiler->function->constants[expr->constant].item);
        *isTruthy = scon_handle_is_truthy(compiler->scon, &item);
        return SCON_TRUE;
    }
    return SCON_FALSE;
}

static void scon_keyword_if(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    if (list->length < 3 || list->length > 4)
    {
        SCON_THROW_ITEM(compiler->scon, "if requires a condition, a then-branch, and an optional else-branch", list);
    }

    scon_expr_t condExpr;
    scon_expr_compile(compiler, list->list.items[1], &condExpr);

    scon_bool_t isTruthy;
    if (scon_expr_is_known_truthy(compiler, &condExpr, &isTruthy))
    {
        scon_expr_done(compiler, &condExpr);
        if (isTruthy)
        {
            scon_expr_compile(compiler, list->list.items[2], out);
        }
        else if (list->length == 4)
        {
            scon_expr_compile(compiler, list->list.items[3], out);
        }
        else
        {
            *out = SCON_EXPR_CONST(scon_const_nil(compiler->scon, compiler->function));
        }
        return;
    }

    scon_reg_t target = scon_reg_alloc(compiler);
    scon_reg_t condReg = scon_emit_move_or_alloc(compiler, &condExpr);
    scon_size_t jumpElse = scon_emit_jump(compiler, SCON_OPCODE_JMP_FALSE, condReg);
    scon_expr_done(compiler, &condExpr);

    scon_expr_t thenExpr;
    scon_expr_compile(compiler, list->list.items[2], &thenExpr);
    if (thenExpr.mode != SCON_MODE_NONE)
    {
        scon_emit_move(compiler, target, &thenExpr);
        scon_expr_done(compiler, &thenExpr);
    }

    scon_size_t jumpEnd = 0;
    if (list->length == 4)
    {
        jumpEnd = scon_emit_jump(compiler, SCON_OPCODE_JMP, 0);
    }

    scon_emit_jump_patch(compiler, jumpElse);

    if (list->length == 4)
    {
        scon_expr_t elseExpr;
        scon_expr_compile(compiler, list->list.items[3], &elseExpr);
        if (elseExpr.mode != SCON_MODE_NONE)
        {
            scon_emit_move(compiler, target, &elseExpr);
            scon_expr_done(compiler, &elseExpr);
        }
        scon_emit_jump_patch(compiler, jumpEnd);
    }
    else
    {
        scon_expr_t nilExpr = SCON_EXPR_CONST(scon_const_nil(compiler->scon, compiler->function));
        scon_emit_move(compiler, target, &nilExpr);
    }

    *out = SCON_EXPR_REG(target);
}

static void scon_keyword_when_unless(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out,
    scon_opcode_t jumpOp, scon_const_t defaultVal)
{
    if (list->length < 2)
    {
        *out = SCON_EXPR_NONE();
        return;
    }

    scon_expr_t condExpr;
    scon_expr_compile(compiler, list->list.items[1], &condExpr);

    scon_bool_t isTruthy;
    if (scon_expr_is_known_truthy(compiler, &condExpr, &isTruthy))
    {
        scon_expr_done(compiler, &condExpr);
        scon_bool_t shouldEval = (jumpOp == SCON_OPCODE_JMP_FALSE) ? isTruthy : !isTruthy;

        if (shouldEval)
        {
            scon_compile_block(compiler, list, 2, out);
        }
        else
        {
            *out = SCON_EXPR_CONST(defaultVal);
        }
        return;
    }

    scon_reg_t target = scon_reg_alloc(compiler);
    scon_expr_t defaultExpr = SCON_EXPR_CONST(defaultVal);
    scon_emit_move(compiler, target, &defaultExpr);
    scon_reg_t condReg = scon_emit_move_or_alloc(compiler, &condExpr);
    scon_size_t jumpEnd = scon_emit_jump(compiler, jumpOp, condReg);
    scon_expr_done(compiler, &condExpr);

    scon_expr_t bodyExpr;
    scon_compile_block(compiler, list, 2, &bodyExpr);
    if (bodyExpr.mode != SCON_MODE_NONE)
    {
        scon_emit_move(compiler, target, &bodyExpr);
        scon_expr_done(compiler, &bodyExpr);
    }

    scon_emit_jump_patch(compiler, jumpEnd);

    *out = SCON_EXPR_REG(target);
}

static void scon_keyword_when(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_keyword_when_unless(compiler, list, out, SCON_OPCODE_JMP_FALSE,
        scon_const_false(compiler->scon, compiler->function));
}

static void scon_keyword_unless(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_keyword_when_unless(compiler, list, out, SCON_OPCODE_JMP_TRUE,
        scon_const_nil(compiler->scon, compiler->function));
}

static void scon_keyword_cond(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    if (list->length < 2)
    {
        *out = SCON_EXPR_CONST(scon_const_nil(compiler->scon, compiler->function));
        return;
    }

    scon_reg_t target = (scon_reg_t)-1;
    scon_size_t jumpsEnd[256];
    scon_size_t jumpCount = 0;
    scon_bool_t alwaysHit = SCON_FALSE;

    for (scon_uint32_t i = 1; i < list->length; i++)
    {
        scon_item_t* pair = list->list.items[i];
        if (pair->type != SCON_ITEM_TYPE_LIST || pair->length != 2)
        {
            SCON_THROW_ITEM(compiler->scon, "cond clauses must be lists of exactly two items", pair);
        }

        scon_expr_t condExpr;
        scon_expr_compile(compiler, pair->list.items[0], &condExpr);

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
                scon_expr_compile(compiler, pair->list.items[1], out);
                return;
            }

            scon_expr_t valExpr;
            scon_expr_compile(compiler, pair->list.items[1], &valExpr);
            if (valExpr.mode != SCON_MODE_NONE)
            {
                scon_emit_move(compiler, target, &valExpr);
                scon_expr_done(compiler, &valExpr);
            }
            alwaysHit = SCON_TRUE;
            break;
        }

        if (target == (scon_reg_t)-1)
        {
            target = scon_reg_alloc(compiler);
        }

        scon_reg_t condReg = scon_emit_move_or_alloc(compiler, &condExpr);
        scon_size_t jumpNext = scon_emit_jump(compiler, SCON_OPCODE_JMP_FALSE, condReg);
        scon_expr_done(compiler, &condExpr);

        scon_expr_t valExpr;
        scon_expr_compile(compiler, pair->list.items[1], &valExpr);
        if (valExpr.mode != SCON_MODE_NONE)
        {
            scon_emit_move(compiler, target, &valExpr);
            scon_expr_done(compiler, &valExpr);
        }

        if (jumpCount >= 256)
        {
            SCON_THROW_ITEM(compiler->scon, "too many clauses in cond", list);
        }
        jumpsEnd[jumpCount++] = scon_emit_jump(compiler, SCON_OPCODE_JMP, 0);

        scon_emit_jump_patch(compiler, jumpNext);
    }

    if (target == (scon_reg_t)-1)
    {
        *out = SCON_EXPR_CONST(scon_const_nil(compiler->scon, compiler->function));
        return;
    }

    if (!alwaysHit)
    {
        scon_expr_t nilConst = SCON_EXPR_CONST(scon_const_nil(compiler->scon, compiler->function));
        scon_emit_move(compiler, target, &nilConst);
    }

    for (scon_size_t i = 0; i < jumpCount; i++)
    {
        scon_emit_jump_patch(compiler, jumpsEnd[i]);
    }

    *out = SCON_EXPR_REG(target);
}

static void scon_keyword_and_or(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out, scon_opcode_t jumpOp)
{
    if (list->length < 2)
    {
        *out = SCON_EXPR_CONST(scon_const_false(compiler->scon, compiler->function));
        return;
    }

    scon_reg_t target = (scon_reg_t)-1;
    scon_size_t jumps[256];
    scon_size_t jumpCount = 0;

    for (scon_uint32_t i = 1; i < list->length; i++)
    {
        scon_expr_t argExpr;
        scon_expr_compile(compiler, list->list.items[i], &argExpr);

        scon_bool_t isTruthy;
        if (scon_expr_is_known_truthy(compiler, &argExpr, &isTruthy))
        {
            scon_bool_t shortCircuits = (jumpOp == SCON_OPCODE_JMP_TRUE) ? isTruthy : !isTruthy;

            if (shortCircuits || i == list->length - 1)
            {
                if (target == (scon_reg_t)-1)
                {
                    *out = argExpr;
                    return;
                }

                scon_emit_move(compiler, target, &argExpr);
                scon_expr_done(compiler, &argExpr);
                break;
            }

            scon_expr_done(compiler, &argExpr);
            continue;
        }

        if (target == (scon_reg_t)-1)
        {
            target = scon_reg_alloc(compiler);
        }

        scon_emit_move(compiler, target, &argExpr);
        scon_expr_done(compiler, &argExpr);

        if (i != list->length - 1)
        {
            if (jumpCount >= 256)
            {
                SCON_THROW_ITEM(compiler->scon, "too many arguments for logical operator", list);
            }
            jumps[jumpCount++] = scon_emit_jump(compiler, jumpOp, target);
        }
    }

    for (scon_size_t i = 0; i < jumpCount; i++)
    {
        scon_emit_jump_patch(compiler, jumps[i]);
    }

    *out = SCON_EXPR_REG(target);
}

static void scon_keyword_and(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_keyword_and_or(compiler, list, out, SCON_OPCODE_JMP_FALSE);
}

static void scon_keyword_or(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_keyword_and_or(compiler, list, out, SCON_OPCODE_JMP_TRUE);
}

static void scon_keyword_not(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    if (list->length != 2)
    {
        SCON_THROW_ITEM(compiler->scon, "not requires exactly one argument", list);
    }

    scon_reg_t target = scon_reg_alloc(compiler);

    scon_expr_t argExpr;
    scon_expr_compile(compiler, list->list.items[1], &argExpr);

    scon_bool_t isTruthy;
    if (scon_expr_is_known_truthy(compiler, &argExpr, &isTruthy))
    {
        scon_expr_done(compiler, &argExpr);
        *out = SCON_EXPR_CONST(isTruthy ? scon_const_false(compiler->scon, compiler->function)
                                        : scon_const_true(compiler->scon, compiler->function));
        return;
    }

    scon_reg_t argReg = scon_emit_move_or_alloc(compiler, &argExpr);
    scon_size_t jumpTrue = scon_emit_jump(compiler, SCON_OPCODE_JMP_TRUE, argReg);
    scon_expr_done(compiler, &argExpr);

    scon_expr_t trueExpr = SCON_EXPR_CONST(scon_const_true(compiler->scon, compiler->function));
    scon_emit_move(compiler, target, &trueExpr);

    scon_size_t jumpEnd = scon_emit_jump(compiler, SCON_OPCODE_JMP, 0);

    scon_emit_jump_patch(compiler, jumpTrue);
    scon_expr_t falseExpr = SCON_EXPR_CONST(scon_const_false(compiler->scon, compiler->function));
    scon_emit_move(compiler, target, &falseExpr);

    scon_emit_jump_patch(compiler, jumpEnd);

    *out = SCON_EXPR_REG(target);
}

static inline scon_bool_t scon_expr_get_item(scon_compiler_t* compiler, scon_expr_t* expr, scon_handle_t* outItem)
{
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

static scon_bool_t scon_fold_comparison(scon_t* scon, scon_opcode_t opBase, scon_handle_t left, scon_handle_t right,
    scon_bool_t* result)
{
    scon_int64_t cmp = scon_handle_compare(scon, &left, &right);
    switch (opBase)
    {
    case SCON_OPCODE_EQUAL:
        *result = (cmp == 0);
        return SCON_TRUE;
    case SCON_OPCODE_NOT_EQUAL:
        *result = (cmp != 0);
        return SCON_TRUE;
    case SCON_OPCODE_STRICT_EQUAL:
        *result = scon_handle_is_equal(scon, &left, &right);
        return SCON_TRUE;
    case SCON_OPCODE_LESS:
        *result = (cmp < 0);
        return SCON_TRUE;
    case SCON_OPCODE_LESS_EQUAL:
        *result = (cmp <= 0);
        return SCON_TRUE;
    case SCON_OPCODE_GREATER:
        *result = (cmp > 0);
        return SCON_TRUE;
    case SCON_OPCODE_GREATER_EQUAL:
        *result = (cmp >= 0);
        return SCON_TRUE;
    default:
        return SCON_FALSE;
    }
}

static void scon_compile_comparison(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out,
    scon_opcode_t opBase)
{
    if (list->length < 3)
    {
        SCON_THROW_ITEM(compiler->scon, "comparison requires at least two arguments", list);
    }

    scon_expr_t leftExpr;
    scon_expr_compile(compiler, list->list.items[1], &leftExpr);

    scon_reg_t target = (scon_reg_t)-1;
    scon_size_t jumps[256];
    scon_size_t jumpCount = 0;

    for (scon_uint32_t i = 2; i < list->length; i++)
    {
        scon_expr_t rightExpr;
        scon_expr_compile(compiler, list->list.items[i], &rightExpr);

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
                        scon_expr_t falseExpr = SCON_EXPR_CONST(scon_const_false(compiler->scon, compiler->function));
                        scon_emit_move(compiler, target, &falseExpr);
                        for (scon_size_t j = 0; j < jumpCount; j++)
                        {
                            scon_emit_jump_patch(compiler, jumps[j]);
                        }
                        *out = SCON_EXPR_REG(target);
                    }
                    else
                    {
                        if (target != (scon_reg_t)-1)
                        {
                            scon_reg_free(compiler, target);
                        }
                        *out = SCON_EXPR_CONST(scon_const_false(compiler->scon, compiler->function));
                    }
                    return;
                }
            }
        }

        if (target == (scon_reg_t)-1)
        {
            target = scon_reg_alloc(compiler);
        }

        if (leftExpr.mode != SCON_MODE_REG)
        {
            scon_emit_move_or_alloc(compiler, &leftExpr);
        }

        scon_emit_binary(compiler, opBase, target, leftExpr.reg, &rightExpr);

        if (i != list->length - 1)
        {
            if (jumpCount >= 256)
            {
                SCON_THROW_ITEM(compiler->scon, "too many arguments for comparison", list);
            }
            jumps[jumpCount++] = scon_emit_jump(compiler, SCON_OPCODE_JMP_FALSE, target);

            scon_expr_done(compiler, &leftExpr);
            leftExpr = rightExpr;
        }
        else
        {
            scon_expr_done(compiler, &leftExpr);
            scon_expr_done(compiler, &rightExpr);
            leftExpr = SCON_EXPR_NONE();
        }
    }

    scon_expr_done(compiler, &leftExpr);

    if (jumpCount > 0)
    {
        for (scon_size_t i = 0; i < jumpCount; i++)
        {
            scon_emit_jump_patch(compiler, jumps[i]);
        }
        *out = SCON_EXPR_REG(target);
    }
    else if (target == (scon_reg_t)-1)
    {
        *out = SCON_EXPR_CONST(scon_const_true(compiler->scon, compiler->function));
    }
    else
    {
        *out = SCON_EXPR_REG(target);
    }
}

static void scon_keyword_equal(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_compile_comparison(compiler, list, out, SCON_OPCODE_EQUAL);
}

static void scon_keyword_strict_equal(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_compile_comparison(compiler, list, out, SCON_OPCODE_STRICT_EQUAL);
}

static void scon_keyword_not_equal(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_compile_comparison(compiler, list, out, SCON_OPCODE_NOT_EQUAL);
}

static void scon_keyword_less(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_compile_comparison(compiler, list, out, SCON_OPCODE_LESS);
}

static void scon_keyword_less_equal(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_compile_comparison(compiler, list, out, SCON_OPCODE_LESS_EQUAL);
}

static void scon_keyword_greater(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_compile_comparison(compiler, list, out, SCON_OPCODE_GREATER);
}

static void scon_keyword_greater_equal(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_compile_comparison(compiler, list, out, SCON_OPCODE_GREATER_EQUAL);
}

static scon_bool_t scon_fold_binary_expr(scon_compiler_t* compiler, scon_opcode_t opBase, scon_expr_t* leftExpr,
    scon_expr_t* rightExpr, scon_expr_t* outExpr)
{
    if (leftExpr->mode != SCON_MODE_CONST || rightExpr->mode != SCON_MODE_CONST)
    {
        return SCON_FALSE;
    }

    if (compiler->function->constants[leftExpr->constant].type != SCON_CONST_SLOT_ITEM ||
        compiler->function->constants[rightExpr->constant].type != SCON_CONST_SLOT_ITEM)
    {
        return SCON_FALSE;
    }

    scon_item_t* leftNode = compiler->function->constants[leftExpr->constant].item;
    scon_item_t* rightNode = compiler->function->constants[rightExpr->constant].item;

    scon_bool_t isBitwise = (opBase >= SCON_OPCODE_BIT_AND && opBase <= SCON_OPCODE_BIT_SHR);

    if (isBitwise)
    {
        if (!(leftNode->flags & SCON_ITEM_FLAG_INT_SHAPED) || !(rightNode->flags & SCON_ITEM_FLAG_INT_SHAPED))
        {
            return SCON_FALSE;
        }

        scon_int64_t li = leftNode->atom.integerValue;
        scon_int64_t ri = rightNode->atom.integerValue;
        scon_atom_t* result;

        switch (opBase)
        {
        case SCON_OPCODE_BIT_AND:
            result = scon_atom_lookup_int(compiler->scon, li & ri);
            break;
        case SCON_OPCODE_BIT_OR:
            result = scon_atom_lookup_int(compiler->scon, li | ri);
            break;
        case SCON_OPCODE_BIT_XOR:
            result = scon_atom_lookup_int(compiler->scon, li ^ ri);
            break;
        case SCON_OPCODE_BIT_SHL:
            if (ri < 0 || ri >= 64)
            {
                return SCON_FALSE;
            }
            result = scon_atom_lookup_int(compiler->scon, li << ri);
            break;
        case SCON_OPCODE_BIT_SHR:
            if (ri < 0 || ri >= 64)
            {
                return SCON_FALSE;
            }
            result = scon_atom_lookup_int(compiler->scon, li >> ri);
            break;
        default:
            return SCON_FALSE;
        }

        scon_const_slot_t slot = SCON_CONST_SLOT_ITEM(SCON_CONTAINER_OF(result, scon_item_t, atom));
        *outExpr = SCON_EXPR_CONST(scon_function_lookup_constant(compiler->scon, compiler->function, &slot));
        return SCON_TRUE;
    }

    if (!(leftNode->flags & (SCON_ITEM_FLAG_INT_SHAPED | SCON_ITEM_FLAG_FLOAT_SHAPED)))
    {
        return SCON_FALSE;
    }
    if (!(rightNode->flags & (SCON_ITEM_FLAG_INT_SHAPED | SCON_ITEM_FLAG_FLOAT_SHAPED)))
    {
        return SCON_FALSE;
    }

    scon_bool_t isFloat =
        (leftNode->flags & SCON_ITEM_FLAG_FLOAT_SHAPED) || (rightNode->flags & SCON_ITEM_FLAG_FLOAT_SHAPED);

    scon_float_t lf;
    if (leftNode->flags & SCON_ITEM_FLAG_FLOAT_SHAPED)
    {
        lf = leftNode->atom.floatValue;
    }
    else
    {
        lf = (scon_float_t)leftNode->atom.integerValue;
    }

    scon_float_t rf;
    if (rightNode->flags & SCON_ITEM_FLAG_FLOAT_SHAPED)
    {
        rf = rightNode->atom.floatValue;
    }
    else
    {
        rf = (scon_float_t)rightNode->atom.integerValue;
    }

    scon_int64_t li;
    if (leftNode->flags & SCON_ITEM_FLAG_INT_SHAPED)
    {
        li = leftNode->atom.integerValue;
    }
    else
    {
        li = (scon_int64_t)leftNode->atom.floatValue;
    }

    scon_int64_t ri;
    if (rightNode->flags & SCON_ITEM_FLAG_INT_SHAPED)
    {
        ri = rightNode->atom.integerValue;
    }
    else
    {
        ri = (scon_int64_t)rightNode->atom.floatValue;
    }

    scon_atom_t* result;
    if (isFloat)
    {
        switch (opBase)
        {
        case SCON_OPCODE_ADD:
            result = scon_atom_lookup_float(compiler->scon, lf + rf);
            break;
        case SCON_OPCODE_SUB:
            result = scon_atom_lookup_float(compiler->scon, lf - rf);
            break;
        case SCON_OPCODE_MUL:
            result = scon_atom_lookup_float(compiler->scon, lf * rf);
            break;
        case SCON_OPCODE_DIV:
            result = scon_atom_lookup_float(compiler->scon, lf / rf);
            break;
        case SCON_OPCODE_MOD:
            return SCON_FALSE;
        default:
            return SCON_FALSE;
        }
    }
    else
    {
        switch (opBase)
        {
        case SCON_OPCODE_ADD:
            result = scon_atom_lookup_int(compiler->scon, li + ri);
            break;
        case SCON_OPCODE_SUB:
            result = scon_atom_lookup_int(compiler->scon, li - ri);
            break;
        case SCON_OPCODE_MUL:
            result = scon_atom_lookup_int(compiler->scon, li * ri);
            break;
        case SCON_OPCODE_DIV:
            if (ri == 0)
            {
                return SCON_FALSE;
            }
            result = scon_atom_lookup_int(compiler->scon, li / ri);
            break;
        case SCON_OPCODE_MOD:
            if (ri == 0)
            {
                return SCON_FALSE;
            }
            result = scon_atom_lookup_int(compiler->scon, li % ri);
            break;
        default:
            return SCON_FALSE;
        }
    }

    scon_const_slot_t slot = SCON_CONST_SLOT_ITEM(SCON_CONTAINER_OF(result, scon_item_t, atom));
    *outExpr = SCON_EXPR_CONST(scon_function_lookup_constant(compiler->scon, compiler->function, &slot));
    return SCON_TRUE;
}

static void scon_compile_binary(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out, scon_opcode_t opBase)
{
    if (opBase == SCON_OPCODE_MOD || opBase == SCON_OPCODE_BIT_SHL || opBase == SCON_OPCODE_BIT_SHR)
    {
        if (list->length != 3)
        {
            SCON_THROW_ITEM(compiler->scon, "operator requires exactly two arguments", list);
        }
    }
    else if (opBase >= SCON_OPCODE_BIT_AND && opBase <= SCON_OPCODE_BIT_XOR)
    {
        if (list->length < 3)
        {
            SCON_THROW_ITEM(compiler->scon, "bitwise operator requires at least two arguments", list);
        }
    }
    else if (list->length < 2)
    {
        SCON_THROW_ITEM(compiler->scon, "arithmetic operator requires at least one argument", list);
    }

    scon_expr_t leftExpr;
    scon_expr_compile(compiler, list->list.items[1], &leftExpr);

    if (list->length == 2)
    {
        if (opBase == SCON_OPCODE_SUB || opBase == SCON_OPCODE_DIV)
        {
            scon_item_t* initialNode = scon_item_new(compiler->scon);
            initialNode->type = SCON_ITEM_TYPE_ATOM;
            initialNode->flags = SCON_ITEM_FLAG_INT_SHAPED;
            initialNode->atom.integerValue = (opBase == SCON_OPCODE_SUB) ? 0 : 1;

            scon_const_slot_t slot = SCON_CONST_SLOT_ITEM(initialNode);
            scon_expr_t initialExpr =
                SCON_EXPR_CONST(scon_function_lookup_constant(compiler->scon, compiler->function, &slot));

            scon_expr_t foldedExpr;
            if (scon_fold_binary_expr(compiler, opBase, &initialExpr, &leftExpr, &foldedExpr))
            {
                scon_expr_done(compiler, &leftExpr);
                *out = foldedExpr;
                return;
            }

            scon_reg_t initialReg = scon_emit_move_or_alloc(compiler, &initialExpr);
            scon_reg_t target = scon_reg_alloc(compiler);
            scon_emit_binary(compiler, opBase, target, initialReg, &leftExpr);

            scon_expr_done(compiler, &leftExpr);
            scon_expr_done(compiler, &initialExpr);
            *out = SCON_EXPR_REG(target);
            return;
        }

        *out = leftExpr;
        return;
    }

    scon_bool_t hasAccumulator = SCON_FALSE;
    for (scon_uint32_t i = 2; i < list->length; i++)
    {
        scon_expr_t rightExpr;
        scon_expr_compile(compiler, list->list.items[i], &rightExpr);

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
            if (leftExpr.mode != SCON_MODE_REG || SCON_REG_IS_LOCAL(compiler, leftExpr.reg))
            {
                scon_reg_t target = scon_reg_alloc(compiler);
                scon_emit_move(compiler, target, &leftExpr);
                scon_expr_done(compiler, &leftExpr);
                leftExpr = SCON_EXPR_REG(target);
            }
            hasAccumulator = SCON_TRUE;
        }

        scon_emit_binary(compiler, opBase, leftExpr.reg, leftExpr.reg, &rightExpr);

        scon_expr_done(compiler, &rightExpr);
    }

    *out = leftExpr;
}

static void scon_keyword_bit_and(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_compile_binary(compiler, list, out, SCON_OPCODE_BIT_AND);
}

static void scon_keyword_bit_or(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_compile_binary(compiler, list, out, SCON_OPCODE_BIT_OR);
}

static void scon_keyword_bit_xor(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_compile_binary(compiler, list, out, SCON_OPCODE_BIT_XOR);
}

static void scon_keyword_bit_not(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    if (list->length != 2)
    {
        SCON_THROW_ITEM(compiler->scon, "~ requires exactly one argument", list);
    }

    scon_expr_t argExpr;
    scon_expr_compile(compiler, list->list.items[1], &argExpr);

    if (argExpr.mode == SCON_MODE_CONST && compiler->function->constants[argExpr.constant].type == SCON_CONST_SLOT_ITEM)
    {
        scon_item_t* argNode = compiler->function->constants[argExpr.constant].item;
        if (argNode->flags & SCON_ITEM_FLAG_INT_SHAPED)
        {
            scon_atom_t* result = scon_atom_lookup_int(compiler->scon, ~argNode->atom.integerValue);
            scon_const_slot_t slot = SCON_CONST_SLOT_ITEM(SCON_CONTAINER_OF(result, scon_item_t, atom));
            scon_expr_done(compiler, &argExpr);
            *out = SCON_EXPR_CONST(scon_function_lookup_constant(compiler->scon, compiler->function, &slot));
            return;
        }
    }

    scon_reg_t target = scon_reg_alloc(compiler);
    scon_emit(compiler,
        SCON_INST_MAKE_ABC((scon_opcode_t)(SCON_OPCODE_BIT_NOT | argExpr.mode), target, 0, argExpr.value));
    scon_expr_done(compiler, &argExpr);
    *out = SCON_EXPR_REG(target);
}

static void scon_keyword_bit_shl(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_compile_binary(compiler, list, out, SCON_OPCODE_BIT_SHL);
}

static void scon_keyword_bit_shr(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_compile_binary(compiler, list, out, SCON_OPCODE_BIT_SHR);
}

static void scon_keyword_add(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_compile_binary(compiler, list, out, SCON_OPCODE_ADD);
}

static void scon_keyword_sub(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_compile_binary(compiler, list, out, SCON_OPCODE_SUB);
}

static void scon_keyword_mul(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_compile_binary(compiler, list, out, SCON_OPCODE_MUL);
}

static void scon_keyword_div(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_compile_binary(compiler, list, out, SCON_OPCODE_DIV);
}

static void scon_keyword_mod(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_compile_binary(compiler, list, out, SCON_OPCODE_MOD);
}

scon_keyword_handler_t sconKeywordHandlers[SCON_KEYWORD_MAX] = {
    [SCON_KEYWORD_NONE] = SCON_NULL,

    [SCON_KEYWORD_QUOTE] = scon_keyword_quote,
    [SCON_KEYWORD_LIST] = scon_keyword_list,
    [SCON_KEYWORD_DO] = scon_keyword_do,
    [SCON_KEYWORD_LAMBDA] = scon_keyword_lambda,

    [SCON_KEYWORD_DEF] = scon_keyword_def,
    [SCON_KEYWORD_LET] = scon_keyword_let,

    [SCON_KEYWORD_IF] = scon_keyword_if,
    [SCON_KEYWORD_WHEN] = scon_keyword_when,
    [SCON_KEYWORD_UNLESS] = scon_keyword_unless,
    [SCON_KEYWORD_COND] = scon_keyword_cond,
    [SCON_KEYWORD_AND] = scon_keyword_and,
    [SCON_KEYWORD_OR] = scon_keyword_or,
    [SCON_KEYWORD_NOT] = scon_keyword_not,

    [SCON_KEYWORD_EQUAL] = scon_keyword_equal,
    [SCON_KEYWORD_STRICT_EQUAL] = scon_keyword_strict_equal,
    [SCON_KEYWORD_NOT_EQUAL] = scon_keyword_not_equal,
    [SCON_KEYWORD_LESS] = scon_keyword_less,
    [SCON_KEYWORD_LESS_EQUAL] = scon_keyword_less_equal,
    [SCON_KEYWORD_GREATER] = scon_keyword_greater,
    [SCON_KEYWORD_GREATER_EQUAL] = scon_keyword_greater_equal,

    [SCON_KEYWORD_BIT_AND] = scon_keyword_bit_and,
    [SCON_KEYWORD_BIT_OR] = scon_keyword_bit_or,
    [SCON_KEYWORD_BIT_XOR] = scon_keyword_bit_xor,
    [SCON_KEYWORD_BIT_NOT] = scon_keyword_bit_not,
    [SCON_KEYWORD_BIT_SHL] = scon_keyword_bit_shl,
    [SCON_KEYWORD_BIT_SHR] = scon_keyword_bit_shr,

    [SCON_KEYWORD_ADD] = scon_keyword_add,
    [SCON_KEYWORD_SUB] = scon_keyword_sub,
    [SCON_KEYWORD_MUL] = scon_keyword_mul,
    [SCON_KEYWORD_DIV] = scon_keyword_div,
    [SCON_KEYWORD_MOD] = scon_keyword_mod,
};

const char* sconKeywords[SCON_KEYWORD_MAX] = {
    [SCON_KEYWORD_NONE] = "",
    [SCON_KEYWORD_QUOTE] = "quote",
    [SCON_KEYWORD_LIST] = "list",
    [SCON_KEYWORD_DO] = "do",
    [SCON_KEYWORD_DEF] = "def",
    [SCON_KEYWORD_LAMBDA] = "lambda",
    [SCON_KEYWORD_LET] = "let",
    [SCON_KEYWORD_IF] = "if",
    [SCON_KEYWORD_WHEN] = "when",
    [SCON_KEYWORD_UNLESS] = "unless",
    [SCON_KEYWORD_COND] = "cond",
    [SCON_KEYWORD_AND] = "and",
    [SCON_KEYWORD_OR] = "or",
    [SCON_KEYWORD_NOT] = "not",
    [SCON_KEYWORD_ADD] = "+",
    [SCON_KEYWORD_SUB] = "-",
    [SCON_KEYWORD_MUL] = "*",
    [SCON_KEYWORD_DIV] = "/",
    [SCON_KEYWORD_MOD] = "%",
    [SCON_KEYWORD_EQUAL] = "=",
    [SCON_KEYWORD_STRICT_EQUAL] = "==",
    [SCON_KEYWORD_NOT_EQUAL] = "!=",
    [SCON_KEYWORD_LESS] = "<",
    [SCON_KEYWORD_LESS_EQUAL] = "<=",
    [SCON_KEYWORD_GREATER] = ">",
    [SCON_KEYWORD_GREATER_EQUAL] = ">=",
    [SCON_KEYWORD_BIT_AND] = "&",
    [SCON_KEYWORD_BIT_OR] = "|",
    [SCON_KEYWORD_BIT_XOR] = "^",
    [SCON_KEYWORD_BIT_NOT] = "~",
    [SCON_KEYWORD_BIT_SHL] = "<<",
    [SCON_KEYWORD_BIT_SHR] = ">>",
};

static inline void scon_keyword_register(scon_t* scon, scon_keyword_t keyword)
{
    const char* str = sconKeywords[keyword];
    scon_size_t len = SCON_STRLEN(str);
    scon_atom_t* atom = scon_atom_lookup(scon, str, len);
    atom->keyword = keyword;
    scon_item_t* item = SCON_CONTAINER_OF(atom, scon_item_t, atom);
    item->flags |= SCON_ITEM_FLAG_KEYWORD;
    item->flags &= ~(SCON_ITEM_FLAG_INT_SHAPED | SCON_ITEM_FLAG_FLOAT_SHAPED);
    scon_gc_retain_item(scon, item);
}

SCON_API void scon_keyword_register_all(scon_t* scon)
{
    for (scon_size_t i = SCON_KEYWORD_NONE + 1; i < SCON_KEYWORD_MAX; i++)
    {
        scon_keyword_register(scon, (scon_keyword_t)i);
    }
}

#endif
