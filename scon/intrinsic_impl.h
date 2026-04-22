#ifndef SCON_INTRINSIC_IMPL_H
#define SCON_INTRINSIC_IMPL_H 1

#include "compile.h"
#include "intrinsic.h"
#include "item.h"

static void scon_intrinsic_quote(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    if (list->length != 2)
    {
        SCON_ERROR_COMPILE(compiler, list, "quote expects exactly one argument, got %u", list->length);
    }

    *out = SCON_EXPR_CONST_ITEM(compiler, SCON_LIST_GET_ITEM(list, 1));
}

static void scon_intrinsic_list(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    scon_reg_t target = scon_expr_get_reg(compiler, out);
    scon_compile_list(compiler, target);

    for (scon_uint32_t i = 1; i < list->length; i++)
    {
        scon_expr_t argExpr = SCON_EXPR_NONE();
        scon_expr_build(compiler, SCON_LIST_GET_ITEM(list, i), &argExpr);

        scon_compile_append(compiler, target, &argExpr);

        scon_expr_done(compiler, &argExpr);
    }

    *out = SCON_EXPR_REG(target);
}

static void scon_intrinsic_block_generic(scon_compiler_t* compiler, scon_item_t* list, scon_uint32_t startIdx,
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

    scon_reg_t targetHint = (out->mode == SCON_MODE_TARGET) ? out->reg : (scon_reg_t)-1;

    for (scon_uint32_t i = startIdx; i < list->length; i++)
    {
        if (i == list->length - 1 && targetHint != (scon_reg_t)-1)
        {
            *out = SCON_EXPR_TARGET(targetHint);
        }
        else
        {
            *out = SCON_EXPR_NONE();
        }

        scon_expr_build(compiler, SCON_LIST_GET_ITEM(list, i), out);
        if (i != list->length - 1)
        {
            scon_expr_done(compiler, out);
        }
    }
}

static void scon_intrinsic_do(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_block_generic(compiler, list, 1, out);
}

static void scon_intrinsic_lambda(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    if (list->length < 3)
    {
        SCON_ERROR_COMPILE(compiler, list, "lambda expects at least two arguments: an argument list and a body, got %d", list->length);
    }

    scon_item_t* args = SCON_LIST_GET_ITEM(list, 1);
    if (args->type != SCON_ITEM_TYPE_LIST)
    {
        SCON_ERROR_COMPILE(compiler, args, "lambda expects a list of arguments, got %s", scon_item_type_str(args->type));
    }

    if (args->length > 255)
    {
        SCON_ERROR_COMPILE(compiler, args, "lambda expects at most 255 arguments, got %d", args->length);
    }

    scon_function_t* func = scon_function_new(compiler->scon);
    scon_item_t* funcItem = SCON_CONTAINER_OF(func, scon_item_t, function);
    scon_const_slot_t slot = SCON_CONST_SLOT_ITEM(funcItem);
    scon_const_t funcConst = scon_function_lookup_constant(compiler->scon, compiler->function, &slot);

    func->arity = (scon_uint8_t)args->length;

    scon_compiler_t childCompiler;
    scon_compiler_init(&childCompiler, compiler->scon, func, compiler);

    for (scon_uint32_t i = 0; i < args->length; i++)
    {
        scon_item_t* argName = SCON_LIST_GET_ITEM(args, i);
        if (argName->type != SCON_ITEM_TYPE_ATOM)
        {
            SCON_ERROR_COMPILE(compiler, argName, "lambda expects a list of atoms as arguments, got %s", scon_item_type_str(argName->type));
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
            SCON_ERROR_COMPILE(compiler, SCON_CONTAINER_OF(captureName, scon_item_t, atom), "undefined variable '%s'", captureName->string);
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

static void scon_intrinsic_def(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    if (list->length != 3)
    {
        SCON_ERROR_COMPILE(compiler, list, "def expects two arguments, got %d", list->length);
    }

    scon_item_t* name = SCON_LIST_GET_ITEM(list, 1);
    if (name->type != SCON_ITEM_TYPE_ATOM)
    {
        SCON_ERROR_COMPILE(compiler, name, "def expects an atom as the name, got %s", scon_item_type_str(name->type));
    }

    scon_local_t* local = scon_local_def(compiler, &name->atom);

    scon_expr_t valExpr = SCON_EXPR_NONE();
    scon_expr_build(compiler, SCON_LIST_GET_ITEM(list, 2), &valExpr);

    scon_local_def_done(compiler, local, &valExpr);
    scon_expr_done(compiler, &valExpr);

    *out = valExpr;
}

static void scon_intrinsic_let(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    if (list->length < 2)
    {
        SCON_ERROR_COMPILE(compiler, list, "let expects at least two arguments, got %d", list->length);
    }

    scon_item_t* bindings = SCON_LIST_GET_ITEM(list, 1);
    if (bindings->type != SCON_ITEM_TYPE_LIST)
    {
        SCON_ERROR_COMPILE(compiler, bindings, "let expects a list of bindings, got %s", scon_item_type_str(bindings->type));
    }

    scon_uint16_t initialLocalCount = compiler->localCount;

    for (scon_uint32_t i = 0; i < bindings->length; i++)
    {
        scon_item_t* bindItem = SCON_LIST_GET_ITEM(bindings, i);
        if (bindItem->type != SCON_ITEM_TYPE_LIST || bindItem->length != 2)
        {
            SCON_ERROR_COMPILE(compiler, bindItem, "let binding must be a list of two items, got %s (length %d)", scon_item_type_str(bindItem->type), bindItem->length);
        }

        scon_item_t* name = SCON_LIST_GET_ITEM(bindItem, 0);
        if (name->type != SCON_ITEM_TYPE_ATOM)
        {
            SCON_ERROR_COMPILE(compiler, name, "let binding name must be an atom, got %s", scon_item_type_str(name->type));
        }

        scon_expr_t valExpr = SCON_EXPR_NONE();
        scon_expr_build(compiler, SCON_LIST_GET_ITEM(bindItem, 1), &valExpr);

        scon_local_t* local = scon_local_def(compiler, &name->atom);
        scon_local_def_done(compiler, local, &valExpr);

        scon_expr_done(compiler, &valExpr);
    }

    scon_reg_t targetHint = (out->mode == SCON_MODE_TARGET) ? out->reg : (scon_reg_t)-1;

    for (scon_uint32_t i = 2; i < list->length; i++)
    {
        if (i == list->length - 1 && targetHint != (scon_reg_t)-1)
        {
            *out = SCON_EXPR_TARGET(targetHint);
        }
        else
        {
            *out = SCON_EXPR_NONE();
        }

        scon_expr_build(compiler, SCON_LIST_GET_ITEM(list, i), out);
        if (i != list->length - 1)
        {
            scon_expr_done(compiler, out);
        }
    }

    if (list->length == 2)
    {
        *out = SCON_EXPR_NONE();
    }

    compiler->localCount = initialLocalCount;
}

static inline scon_bool_t scon_expr_is_known_truthy(scon_compiler_t* compiler, scon_expr_t* expr, scon_bool_t* isTruthy)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(expr != SCON_NULL);
    SCON_ASSERT(isTruthy != SCON_NULL);

    if (expr->mode == SCON_MODE_CONST)
    {
        if (compiler->function->constants[expr->constant].type != SCON_CONST_SLOT_ITEM)
        {
            return SCON_FALSE;
        }

        scon_handle_t item = SCON_HANDLE_FROM_ITEM(compiler->function->constants[expr->constant].item);
        *isTruthy = SCON_HANDLE_IS_TRUTHY(&item);
        return SCON_TRUE;
    }
    return SCON_FALSE;
}

static void scon_intrinsic_if(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    if (list->length < 3 || list->length > 4)
    {
        SCON_ERROR_COMPILE(compiler, list, "if requires a condition, a then-branch, and an optional else-branch, got %d", list->length);
    }

    scon_expr_t condExpr = SCON_EXPR_NONE();
    scon_expr_build(compiler, SCON_LIST_GET_ITEM(list, 1), &condExpr);

    scon_bool_t isTruthy;
    if (scon_expr_is_known_truthy(compiler, &condExpr, &isTruthy))
    {
        scon_expr_done(compiler, &condExpr);
        if (isTruthy)
        {
            scon_expr_build(compiler, SCON_LIST_GET_ITEM(list, 2), out);
        }
        else if (list->length == 4)
        {
            scon_expr_build(compiler, SCON_LIST_GET_ITEM(list, 3), out);
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

    scon_expr_t thenExpr = SCON_EXPR_TARGET(target);
    scon_expr_build(compiler, SCON_LIST_GET_ITEM(list, 2), &thenExpr);
    if (thenExpr.mode != SCON_MODE_NONE && (thenExpr.mode != SCON_MODE_REG || thenExpr.reg != target))
    {
        scon_compile_move(compiler, target, &thenExpr);
        scon_expr_done(compiler, &thenExpr);
    }

    scon_size_t jumpEnd = 0;
    if (list->length == 4)
    {
        jumpEnd = scon_compile_jump(compiler, SCON_OPCODE_JMP, 0);
    }

    scon_compile_jump_patch(compiler, jumpElse);

    if (list->length == 4)
    {
        scon_expr_t elseExpr = SCON_EXPR_TARGET(target);
        scon_expr_build(compiler, SCON_LIST_GET_ITEM(list, 3), &elseExpr);
        if (elseExpr.mode != SCON_MODE_NONE && (elseExpr.mode != SCON_MODE_REG || elseExpr.reg != target))
        {
            scon_compile_move(compiler, target, &elseExpr);
            scon_expr_done(compiler, &elseExpr);
        }
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
    scon_expr_build(compiler, SCON_LIST_GET_ITEM(list, 1), &condExpr);

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

    scon_expr_t bodyExpr = SCON_EXPR_TARGET(target);
    scon_intrinsic_block_generic(compiler, list, 2, &bodyExpr);
    if (bodyExpr.mode != SCON_MODE_NONE && (bodyExpr.mode != SCON_MODE_REG || bodyExpr.reg != target))
    {
        scon_compile_move(compiler, target, &bodyExpr);
        scon_expr_done(compiler, &bodyExpr);
    }

    scon_compile_jump_patch(compiler, jumpEnd);

    *out = SCON_EXPR_REG(target);
}

static void scon_intrinsic_when(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_when_unless(compiler, list, out, SCON_OPCODE_JMPF, SCON_EXPR_FALSE(compiler));
}

static void scon_intrinsic_unless(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_when_unless(compiler, list, out, SCON_OPCODE_JMPT, SCON_EXPR_NIL(compiler));
}

static void scon_intrinsic_cond(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    if (list->length < 2)
    {
        *out = SCON_EXPR_NIL(compiler);
        return;
    }

    scon_reg_t targetHint = (out->mode == SCON_MODE_TARGET) ? out->reg : (scon_reg_t)-1;
    scon_reg_t target = (scon_reg_t)-1;
    scon_size_t jumpsEnd[256];
    scon_size_t jumpCount = 0;
    scon_bool_t alwaysHit = SCON_FALSE;

    for (scon_uint32_t i = 1; i < list->length; i++)
    {
        scon_item_t* pair = SCON_LIST_GET_ITEM(list, i);
        if (pair->type != SCON_ITEM_TYPE_LIST || pair->length != 2)
        {
            SCON_ERROR_COMPILE(compiler, pair, "cond clauses must be lists of exactly two items, got %s (length %d)", scon_item_type_str(pair->type), pair->length);
        }

        scon_expr_t condExpr = SCON_EXPR_NONE();
        scon_expr_build(compiler, SCON_LIST_GET_ITEM(pair, 0), &condExpr);

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
                scon_expr_build(compiler, SCON_LIST_GET_ITEM(pair, 1), out);
                return;
            }

            scon_expr_t valExpr = SCON_EXPR_TARGET(target);
            scon_expr_build(compiler, SCON_LIST_GET_ITEM(pair, 1), &valExpr);
            if (valExpr.mode != SCON_MODE_NONE && (valExpr.mode != SCON_MODE_REG || valExpr.reg != target))
            {
                scon_compile_move(compiler, target, &valExpr);
                scon_expr_done(compiler, &valExpr);
            }
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

        scon_expr_t valExpr = SCON_EXPR_TARGET(target);
        scon_expr_build(compiler, SCON_LIST_GET_ITEM(pair, 1), &valExpr);
        if (valExpr.mode != SCON_MODE_NONE && (valExpr.mode != SCON_MODE_REG || valExpr.reg != target))
        {
            scon_compile_move(compiler, target, &valExpr);
            scon_expr_done(compiler, &valExpr);
        }

        if (jumpCount >= 256)
        {
            SCON_ERROR_COMPILE(compiler, list, "too many clauses in cond, got %d", jumpCount);
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

    for (scon_size_t i = 0; i < jumpCount; i++)
    {
        scon_compile_jump_patch(compiler, jumpsEnd[i]);
    }

    *out = SCON_EXPR_REG(target);
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

    scon_reg_t targetHint = (out->mode == SCON_MODE_TARGET) ? out->reg : (scon_reg_t)-1;
    scon_reg_t target = (scon_reg_t)-1;
    scon_size_t jumps[256];
    scon_size_t jumpCount = 0;

    for (scon_uint32_t i = 1; i < list->length; i++)
    {
        scon_expr_t argExpr = SCON_EXPR_NONE();
        if (target != (scon_reg_t)-1)
        {
            argExpr = SCON_EXPR_TARGET(target);
        }

        scon_expr_build(compiler, SCON_LIST_GET_ITEM(list, i), &argExpr);

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
            if (jumpCount >= 256)
            {
                SCON_ERROR_COMPILE(compiler, list, "too many arguments for logical operator, got %d", jumpCount);
            }
            jumps[jumpCount++] = scon_compile_jump(compiler, jumpOp, target);
        }
    }

    for (scon_size_t i = 0; i < jumpCount; i++)
    {
        scon_compile_jump_patch(compiler, jumps[i]);
    }

    *out = SCON_EXPR_REG(target);
}

static void scon_intrinsic_and(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_and_or(compiler, list, out, SCON_OPCODE_JMPF);
}

static void scon_intrinsic_or(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_and_or(compiler, list, out, SCON_OPCODE_JMPT);
}

static void scon_intrinsic_not(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    if (list->length != 2)
    {
        SCON_ERROR_COMPILE(compiler, list, "not expects exactly two arguments, got %d", list->length);
    }

    scon_reg_t target = scon_expr_get_reg(compiler, out);

    scon_expr_t argExpr = SCON_EXPR_NONE();
    scon_expr_build(compiler, SCON_LIST_GET_ITEM(list, 1), &argExpr);

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

static void scon_intrinsic_comparison_generic(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out,
    scon_opcode_t opBase)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    if (list->length < 3)
    {
        SCON_ERROR_COMPILE(compiler, list, "comparison requires at least two arguments, got %d", list->length);
    }

    scon_reg_t targetHint = (out->mode == SCON_MODE_TARGET) ? out->reg : (scon_reg_t)-1;
    scon_expr_t leftExpr = SCON_EXPR_NONE();
    scon_expr_build(compiler, SCON_LIST_GET_ITEM(list, 1), &leftExpr);

    scon_reg_t target = (scon_reg_t)-1;
    scon_size_t jumps[256];
    scon_size_t jumpCount = 0;

    for (scon_uint32_t i = 2; i < list->length; i++)
    {
        scon_expr_t rightExpr = SCON_EXPR_NONE();
        scon_expr_build(compiler, SCON_LIST_GET_ITEM(list, i), &rightExpr);

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
                        for (scon_size_t j = 0; j < jumpCount; j++)
                        {
                            scon_compile_jump_patch(compiler, jumps[j]);
                        }
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
            if (jumpCount >= 256)
            {
                SCON_ERROR_COMPILE(compiler, list, "comparison expects at most 256 arguments, got %d", jumpCount);
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
    }

    scon_expr_done(compiler, &leftExpr);

    if (jumpCount > 0)
    {
        for (scon_size_t i = 0; i < jumpCount; i++)
        {
            scon_compile_jump_patch(compiler, jumps[i]);
        }
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

static void scon_intrinsic_equal(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_comparison_generic(compiler, list, out, SCON_OPCODE_EQ);
}

static void scon_intrinsic_strict_equal(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_comparison_generic(compiler, list, out, SCON_OPCODE_SEQ);
}

static void scon_intrinsic_not_equal(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_comparison_generic(compiler, list, out, SCON_OPCODE_NEQ);
}

static void scon_intrinsic_strict_not_equal(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_comparison_generic(compiler, list, out, SCON_OPCODE_SNEQ);
}

static void scon_intrinsic_less(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_comparison_generic(compiler, list, out, SCON_OPCODE_LT);
}

static void scon_intrinsic_less_equal(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_comparison_generic(compiler, list, out, SCON_OPCODE_LE);
}

static void scon_intrinsic_greater(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_comparison_generic(compiler, list, out, SCON_OPCODE_GT);
}

static void scon_intrinsic_greater_equal(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_comparison_generic(compiler, list, out, SCON_OPCODE_GE);
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

    scon_bool_t isBitwise = (opBase >= SCON_OPCODE_BAND && opBase <= SCON_OPCODE_SHR);

    if (isBitwise)
    {
        if (!(leftItem->flags & SCON_ITEM_FLAG_INT_SHAPED) || !(rightItem->flags & SCON_ITEM_FLAG_INT_SHAPED))
        {
            return SCON_FALSE;
        }

        scon_int64_t li = leftItem->atom.integerValue;
        scon_int64_t ri = rightItem->atom.integerValue;
        scon_atom_t* result;

        switch (opBase)
        {
        case SCON_OPCODE_BAND:
            result = scon_atom_lookup_int(compiler->scon, li & ri);
            break;
        case SCON_OPCODE_BOR:
            result = scon_atom_lookup_int(compiler->scon, li | ri);
            break;
        case SCON_OPCODE_BXOR:
            result = scon_atom_lookup_int(compiler->scon, li ^ ri);
            break;
        case SCON_OPCODE_SHL:
            if (ri < 0 || ri >= 64)
            {
                return SCON_FALSE;
            }
            result = scon_atom_lookup_int(compiler->scon, li << ri);
            break;
        case SCON_OPCODE_SHR:
            if (ri < 0 || ri >= 64)
            {
                return SCON_FALSE;
            }
            result = scon_atom_lookup_int(compiler->scon, li >> ri);
            break;
        default:
            return SCON_FALSE;
        }

        *outExpr = SCON_EXPR_CONST_ATOM(compiler, result);
        return SCON_TRUE;
    }

    if (!(leftItem->flags & (SCON_ITEM_FLAG_INT_SHAPED | SCON_ITEM_FLAG_FLOAT_SHAPED)))
    {
        return SCON_FALSE;
    }
    if (!(rightItem->flags & (SCON_ITEM_FLAG_INT_SHAPED | SCON_ITEM_FLAG_FLOAT_SHAPED)))
    {
        return SCON_FALSE;
    }

    scon_bool_t isFloat =
        (leftItem->flags & SCON_ITEM_FLAG_FLOAT_SHAPED) || (rightItem->flags & SCON_ITEM_FLAG_FLOAT_SHAPED);

    scon_float_t lf;
    if (leftItem->flags & SCON_ITEM_FLAG_FLOAT_SHAPED)
    {
        lf = leftItem->atom.floatValue;
    }
    else
    {
        lf = (scon_float_t)leftItem->atom.integerValue;
    }

    scon_float_t rf;
    if (rightItem->flags & SCON_ITEM_FLAG_FLOAT_SHAPED)
    {
        rf = rightItem->atom.floatValue;
    }
    else
    {
        rf = (scon_float_t)rightItem->atom.integerValue;
    }

    scon_int64_t li;
    if (leftItem->flags & SCON_ITEM_FLAG_INT_SHAPED)
    {
        li = leftItem->atom.integerValue;
    }
    else
    {
        li = (scon_int64_t)leftItem->atom.floatValue;
    }

    scon_int64_t ri;
    if (rightItem->flags & SCON_ITEM_FLAG_INT_SHAPED)
    {
        ri = rightItem->atom.integerValue;
    }
    else
    {
        ri = (scon_int64_t)rightItem->atom.floatValue;
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
            if (rf == 0.0)
            {
                return SCON_FALSE;
            }
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

    *outExpr = SCON_EXPR_CONST_ATOM(compiler, result);
    return SCON_TRUE;
}

static void scon_intrinsic_binary_generic(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out,
    scon_opcode_t opBase)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    if (opBase == SCON_OPCODE_MOD || opBase == SCON_OPCODE_SHL || opBase == SCON_OPCODE_SHR)
    {
        if (list->length != 3)
        {
            SCON_ERROR_COMPILE(compiler, list, "operator expects exactly two arguments, got %d", list->length);
        }
    }
    else if (opBase >= SCON_OPCODE_BAND && opBase <= SCON_OPCODE_BXOR)
    {
        if (list->length < 3)
        {
            SCON_ERROR_COMPILE(compiler, list, "bitwise operator expects at least two arguments, got %d", list->length);
        }
    }
    else if (list->length < 2)
    {
        SCON_ERROR_COMPILE(compiler, list, "arithmetic operator expects at least one argument, got %d", list->length);
    }

    scon_reg_t targetHint = (out->mode == SCON_MODE_TARGET) ? out->reg : (scon_reg_t)-1;
    scon_expr_t leftExpr = SCON_EXPR_NONE();
    scon_expr_build(compiler, SCON_LIST_GET_ITEM(list, 1), &leftExpr);

    if (list->length == 2)
    {
        if (opBase == SCON_OPCODE_SUB || opBase == SCON_OPCODE_DIV)
        {
            scon_item_t* initialItem = scon_item_new(compiler->scon);
            initialItem->type = SCON_ITEM_TYPE_ATOM;
            initialItem->flags = SCON_ITEM_FLAG_INT_SHAPED;
            initialItem->atom.integerValue = (opBase == SCON_OPCODE_SUB) ? 0 : 1;
            scon_expr_t initialExpr = SCON_EXPR_CONST_ITEM(compiler, initialItem);

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
    for (scon_uint32_t i = 2; i < list->length; i++)
    {
        scon_expr_t rightExpr = SCON_EXPR_NONE();
        scon_expr_build(compiler, SCON_LIST_GET_ITEM(list, i), &rightExpr);

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

static void scon_intrinsic_bit_and(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_binary_generic(compiler, list, out, SCON_OPCODE_BAND);
}

static void scon_intrinsic_bit_or(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_binary_generic(compiler, list, out, SCON_OPCODE_BOR);
}

static void scon_intrinsic_bit_xor(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_binary_generic(compiler, list, out, SCON_OPCODE_BXOR);
}

static void scon_intrinsic_bit_not(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    SCON_ASSERT(compiler != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    if (list->length != 2)
    {
        SCON_ERROR_COMPILE(compiler, list, "bitwise not expects exactly one argument, got %d", list->length);
    }

    scon_expr_t argExpr = SCON_EXPR_NONE();
    scon_expr_build(compiler, SCON_LIST_GET_ITEM(list, 1), &argExpr);

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

static void scon_intrinsic_bit_shl(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_binary_generic(compiler, list, out, SCON_OPCODE_SHL);
}

static void scon_intrinsic_bit_shr(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_binary_generic(compiler, list, out, SCON_OPCODE_SHR);
}

static void scon_intrinsic_add(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_binary_generic(compiler, list, out, SCON_OPCODE_ADD);
}

static void scon_intrinsic_sub(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_binary_generic(compiler, list, out, SCON_OPCODE_SUB);
}

static void scon_intrinsic_mul(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_binary_generic(compiler, list, out, SCON_OPCODE_MUL);
}

static void scon_intrinsic_div(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_binary_generic(compiler, list, out, SCON_OPCODE_DIV);
}

static void scon_intrinsic_mod(scon_compiler_t* compiler, scon_item_t* list, scon_expr_t* out)
{
    scon_intrinsic_binary_generic(compiler, list, out, SCON_OPCODE_MOD);
}

scon_intrinsic_handler_t sconIntrinsicHandlers[SCON_INTRINSIC_MAX] = {
    [SCON_INTRINSIC_NONE] = SCON_NULL,

    [SCON_INTRINSIC_QUOTE] = scon_intrinsic_quote,
    [SCON_INTRINSIC_LIST] = scon_intrinsic_list,
    [SCON_INTRINSIC_DO] = scon_intrinsic_do,
    [SCON_INTRINSIC_LAMBDA] = scon_intrinsic_lambda,

    [SCON_INTRINSIC_DEF] = scon_intrinsic_def,
    [SCON_INTRINSIC_LET] = scon_intrinsic_let,

    [SCON_INTRINSIC_IF] = scon_intrinsic_if,
    [SCON_INTRINSIC_WHEN] = scon_intrinsic_when,
    [SCON_INTRINSIC_UNLESS] = scon_intrinsic_unless,
    [SCON_INTRINSIC_COND] = scon_intrinsic_cond,
    [SCON_INTRINSIC_AND] = scon_intrinsic_and,
    [SCON_INTRINSIC_OR] = scon_intrinsic_or,
    [SCON_INTRINSIC_NOT] = scon_intrinsic_not,

    [SCON_INTRINSIC_EQ] = scon_intrinsic_equal,
    [SCON_INTRINSIC_NEQ] = scon_intrinsic_not_equal,
    [SCON_INTRINSIC_SNEQ] = scon_intrinsic_strict_not_equal,
    [SCON_INTRINSIC_SEQ] = scon_intrinsic_strict_equal,
    [SCON_INTRINSIC_LT] = scon_intrinsic_less,
    [SCON_INTRINSIC_LE] = scon_intrinsic_less_equal,
    [SCON_INTRINSIC_GT] = scon_intrinsic_greater,
    [SCON_INTRINSIC_GE] = scon_intrinsic_greater_equal,

    [SCON_INTRINSIC_BAND] = scon_intrinsic_bit_and,
    [SCON_INTRINSIC_BOR] = scon_intrinsic_bit_or,
    [SCON_INTRINSIC_BXOR] = scon_intrinsic_bit_xor,
    [SCON_INTRINSIC_BNOT] = scon_intrinsic_bit_not,
    [SCON_INTRINSIC_SHL] = scon_intrinsic_bit_shl,
    [SCON_INTRINSIC_SHR] = scon_intrinsic_bit_shr,

    [SCON_INTRINSIC_ADD] = scon_intrinsic_add,
    [SCON_INTRINSIC_SUB] = scon_intrinsic_sub,
    [SCON_INTRINSIC_MUL] = scon_intrinsic_mul,
    [SCON_INTRINSIC_DIV] = scon_intrinsic_div,
    [SCON_INTRINSIC_MOD] = scon_intrinsic_mod,
};

const char* sconIntrinsics[SCON_INTRINSIC_MAX] = {
    [SCON_INTRINSIC_NONE] = "",
    [SCON_INTRINSIC_QUOTE] = "quote",
    [SCON_INTRINSIC_LIST] = "list",
    [SCON_INTRINSIC_DO] = "do",
    [SCON_INTRINSIC_DEF] = "def",
    [SCON_INTRINSIC_LAMBDA] = "lambda",
    [SCON_INTRINSIC_LET] = "let",
    [SCON_INTRINSIC_IF] = "if",
    [SCON_INTRINSIC_WHEN] = "when",
    [SCON_INTRINSIC_UNLESS] = "unless",
    [SCON_INTRINSIC_COND] = "cond",
    [SCON_INTRINSIC_AND] = "and",
    [SCON_INTRINSIC_OR] = "or",
    [SCON_INTRINSIC_NOT] = "not",
    [SCON_INTRINSIC_ADD] = "+",
    [SCON_INTRINSIC_SUB] = "-",
    [SCON_INTRINSIC_MUL] = "*",
    [SCON_INTRINSIC_DIV] = "/",
    [SCON_INTRINSIC_MOD] = "%",
    [SCON_INTRINSIC_SEQ] = "==",
    [SCON_INTRINSIC_SNEQ] = "!==",
    [SCON_INTRINSIC_EQ] = "=",
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

static inline void scon_intrinsic_register(scon_t* scon, scon_intrinsic_t intrinsic)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(intrinsic > SCON_INTRINSIC_NONE && intrinsic < SCON_INTRINSIC_MAX);

    const char* str = sconIntrinsics[intrinsic];
    scon_size_t len = SCON_STRLEN(str);
    scon_atom_t* atom = scon_atom_lookup(scon, str, len, SCON_ATOM_LOOKUP_NONE);
    atom->intrinsic = intrinsic;
    scon_item_t* item = SCON_CONTAINER_OF(atom, scon_item_t, atom);
    item->flags |= SCON_ITEM_FLAG_INTRINSIC;
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
