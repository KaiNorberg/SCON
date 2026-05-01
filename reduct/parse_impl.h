#ifndef REDUCT_PARSE_IMPL_H
#define REDUCT_PARSE_IMPL_H 1

#include "atom.h"
#include "char.h"
#include "core.h"
#include "error.h"
#include "gc.h"
#include "item.h"
#include "list.h"
#include "parse.h"

#define REDUCT_PARSE_STACK_MAX 256

typedef struct
{
    const char* ptr;
    reduct_input_t* input;
    reduct_size_t current;
    reduct_list_t* stack[REDUCT_PARSE_STACK_MAX];
    reduct_bool_t isInfix[REDUCT_PARSE_STACK_MAX];
} reduct_parse_ctx_t;

typedef enum
{
    REDUCT_PARSE_PRECEDENCE_NONE = 0,
    REDUCT_PARSE_PRECEDENCE_LOGICAL_OR,
    REDUCT_PARSE_PRECEDENCE_LOGICAL_AND,
    REDUCT_PARSE_PRECEDENCE_COMPARISON,
    REDUCT_PARSE_PRECEDENCE_SHIFT,
    REDUCT_PARSE_PRECEDENCE_BITWISE,
    REDUCT_PARSE_PRECEDENCE_ADD_SUB,
    REDUCT_PARSE_PRECEDENCE_MUL_DIV,
    REDUCT_PARSE_PRECEDENCE_UNARY,
    REDUCT_PARSE_PRECEDENCE_MAX,
} reduct_parse_precedence_t;

static reduct_parse_precedence_t reduct_parse_get_precedence(reduct_handle_t handle, reduct_bool_t unary)
{
    if (!REDUCT_HANDLE_IS_ITEM(&handle))
    {
        return REDUCT_PARSE_PRECEDENCE_NONE;
    }
    reduct_item_t* item = REDUCT_HANDLE_TO_ITEM(&handle);
    if (item->type != REDUCT_ITEM_TYPE_ATOM)
    {
        return REDUCT_PARSE_PRECEDENCE_NONE;
    }
    reduct_atom_t* atom = &item->atom;

    if (unary)
    {
        if (reduct_atom_is_equal(atom, "not", 3) || reduct_atom_is_equal(atom, "-", 1))
        {
            return REDUCT_PARSE_PRECEDENCE_UNARY;
        }
        return REDUCT_PARSE_PRECEDENCE_NONE;
    }

    if (reduct_atom_is_equal(atom, "or", 2))
    {
        return REDUCT_PARSE_PRECEDENCE_LOGICAL_OR;
    }
    if (reduct_atom_is_equal(atom, "and", 3))
    {
        return REDUCT_PARSE_PRECEDENCE_LOGICAL_AND;
    }
    if (reduct_atom_is_equal(atom, "==", 2) || reduct_atom_is_equal(atom, "!=", 2) ||
        reduct_atom_is_equal(atom, "<", 1) || reduct_atom_is_equal(atom, "<=", 2) ||
        reduct_atom_is_equal(atom, ">", 1) || reduct_atom_is_equal(atom, ">=", 2))
    {
        return REDUCT_PARSE_PRECEDENCE_COMPARISON;
    }
    if (reduct_atom_is_equal(atom, "<<", 2) || reduct_atom_is_equal(atom, ">>", 2))
    {
        return REDUCT_PARSE_PRECEDENCE_SHIFT;
    }
    if (reduct_atom_is_equal(atom, "&", 1) || reduct_atom_is_equal(atom, "|", 1) || reduct_atom_is_equal(atom, "^", 1))
    {
        return REDUCT_PARSE_PRECEDENCE_BITWISE;
    }
    if (reduct_atom_is_equal(atom, "*", 1) || reduct_atom_is_equal(atom, "/", 1) || reduct_atom_is_equal(atom, "%", 1))
    {
        return REDUCT_PARSE_PRECEDENCE_MUL_DIV;
    }
    if (reduct_atom_is_equal(atom, "+", 1) || reduct_atom_is_equal(atom, "-", 1))
    {
        return REDUCT_PARSE_PRECEDENCE_ADD_SUB;
    }

    return REDUCT_PARSE_PRECEDENCE_NONE;
}

static reduct_handle_t reduct_parse_infix_transform_recursive(reduct_t* reduct, reduct_list_t* list, reduct_size_t* pos,
    reduct_parse_precedence_t minPrecedence)
{
    if (*pos >= list->length)
    {
        return reduct_handle_nil(reduct);
    }

    reduct_handle_t first = reduct_list_nth(reduct, list, *pos);
    reduct_handle_t left;
    reduct_parse_precedence_t unaryPrecedence = reduct_parse_get_precedence(first, REDUCT_TRUE);

    if (unaryPrecedence > REDUCT_PARSE_PRECEDENCE_NONE)
    {
        (*pos)++;
        reduct_handle_t operand = reduct_parse_infix_transform_recursive(reduct, list, pos, unaryPrecedence);
        reduct_list_t* call = reduct_list_new(reduct);
        reduct_list_append(reduct, call, first);
        reduct_list_append(reduct, call, operand);
        left = REDUCT_HANDLE_FROM_ITEM(REDUCT_CONTAINER_OF(call, reduct_item_t, list));
    }
    else
    {
        left = first;
        (*pos)++;
    }

    while (*pos < list->length)
    {
        reduct_handle_t op = reduct_list_nth(reduct, list, *pos);
        reduct_parse_precedence_t prec = reduct_parse_get_precedence(op, REDUCT_FALSE);
        if (prec == REDUCT_PARSE_PRECEDENCE_NONE || prec < minPrecedence)
        {
            break;
        }

        (*pos)++;
        if (*pos >= list->length)
        {
            break;
        }

        reduct_handle_t right = reduct_parse_infix_transform_recursive(reduct, list, pos, prec + 1);

        reduct_list_t* call = reduct_list_new(reduct);
        reduct_list_append(reduct, call, op);
        reduct_list_append(reduct, call, left);
        reduct_list_append(reduct, call, right);
        left = REDUCT_HANDLE_FROM_ITEM(REDUCT_CONTAINER_OF(call, reduct_item_t, list));
    }

    return left;
}

static reduct_handle_t reduct_parse_infix_transform(reduct_t* reduct, reduct_list_t* list)
{
    if (list->length == 0)
    {
        return reduct_handle_nil(reduct);
    }

    reduct_size_t pos = 0;
    return reduct_parse_infix_transform_recursive(reduct, list, &pos, REDUCT_PARSE_PRECEDENCE_LOGICAL_OR);
}

static void reduct_parse_whitespace(reduct_parse_ctx_t* ctx)
{
    REDUCT_ASSERT(ctx != REDUCT_NULL);

    while (ctx->ptr < ctx->input->end)
    {
        if (REDUCT_CHAR_IS_WHITESPACE(*ctx->ptr))
        {
            ctx->ptr++;
        }
        else if (*ctx->ptr == '/' && ctx->ptr + 1 < ctx->input->end && *(ctx->ptr + 1) == '/')
        {
            while (ctx->ptr < ctx->input->end && *ctx->ptr != '\n')
            {
                ctx->ptr++;
            }
        }
        else if (*ctx->ptr == '/' && ctx->ptr + 1 < ctx->input->end && *(ctx->ptr + 1) == '*')
        {
            ctx->ptr += 2;
            while (ctx->ptr < ctx->input->end)
            {
                if (*ctx->ptr == '*' && ctx->ptr + 1 < ctx->input->end && *(ctx->ptr + 1) == '/')
                {
                    ctx->ptr += 2;
                    break;
                }
                ctx->ptr++;
            }
        }
        else
        {
            break;
        }
    }
}

static void reduct_parse_quoted_atom(reduct_t* reduct, reduct_parse_ctx_t* ctx)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(ctx != REDUCT_NULL);

    ctx->ptr++;
    const char* start = ctx->ptr;
    while (ctx->ptr < ctx->input->end)
    {
        if (*ctx->ptr == '\\' && ctx->ptr + 1 < ctx->input->end)
        {
            ctx->ptr += 2;
        }
        else if (*ctx->ptr == '"')
        {
            break;
        }
        else
        {
            ctx->ptr++;
        }
    }

    if (ctx->ptr >= ctx->input->end)
    {
        REDUCT_ERROR_SYNTAX(reduct->error, ctx->input, start, "unexpected end of file, missing '\"'");
    }

    reduct_size_t len = (reduct_size_t)(ctx->ptr - start);
    reduct_atom_t* atom = reduct_atom_lookup(reduct, start, len, REDUCT_ATOM_LOOKUP_QUOTED);

    reduct_item_t* item = REDUCT_CONTAINER_OF(atom, reduct_item_t, atom);
    item->inputId = ctx->input->id;
    item->position = (reduct_size_t)(start - ctx->input->buffer);

    reduct_list_append(reduct, ctx->stack[ctx->current], REDUCT_HANDLE_FROM_ITEM(item));
    ctx->ptr++;
}

static void reduct_parse_unquoted_atom(reduct_t* reduct, reduct_parse_ctx_t* ctx)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(ctx != REDUCT_NULL);

    const char* start = ctx->ptr;
    while (ctx->ptr < ctx->input->end && !REDUCT_CHAR_IS_WHITESPACE(*ctx->ptr) && *ctx->ptr != '(' &&
        *ctx->ptr != ')' && *ctx->ptr != '{' && *ctx->ptr != '}')
    {
        ctx->ptr++;
    }

    reduct_size_t len = (reduct_size_t)(ctx->ptr - start);
    if (len == 0)
    {
        return;
    }

    reduct_atom_t* atom = reduct_atom_lookup(reduct, start, len, REDUCT_ATOM_LOOKUP_NONE);
    reduct_item_t* item = REDUCT_CONTAINER_OF(atom, reduct_item_t, atom);
    item->inputId = ctx->input->id;
    item->position = (reduct_size_t)(start - ctx->input->buffer);

    reduct_list_append(reduct, ctx->stack[ctx->current], REDUCT_HANDLE_FROM_ITEM(item));
}

REDUCT_API reduct_handle_t reduct_parse_input(reduct_t* reduct, reduct_input_t* input)
{
    reduct_list_t* root = reduct_list_new(reduct);
    if (root == REDUCT_NULL)
    {
        REDUCT_ERROR_INTERNAL(reduct, "out of memory");
    }

    reduct_item_t* rootItem = REDUCT_CONTAINER_OF(root, reduct_item_t, list);
    rootItem->inputId = input->id;
    reduct_handle_t result = REDUCT_HANDLE_FROM_ITEM(rootItem);
    input->ast = result;

    reduct_parse_ctx_t ctx;
    ctx.ptr = input->buffer;
    ctx.input = input;
    ctx.current = 0;
    ctx.stack[0] = root;
    ctx.isInfix[0] = REDUCT_FALSE;

    while (1)
    {
        reduct_parse_whitespace(&ctx);

        if (ctx.ptr >= ctx.input->end)
        {
            break;
        }

        switch (*ctx.ptr)
        {
        case '{':
        case '(':
        {
            if (ctx.current + 1 >= REDUCT_PARSE_STACK_MAX)
            {
                REDUCT_ERROR_SYNTAX(reduct->error, input, ctx.ptr, "maximum nesting depth exceeded");
            }

            reduct_list_t* child = reduct_list_new(reduct);
            reduct_item_t* item = REDUCT_CONTAINER_OF(child, reduct_item_t, list);
            item->inputId = input->id;
            item->position = (reduct_size_t)(ctx.ptr - input->buffer) + 1;

            ctx.current++;
            ctx.stack[ctx.current] = child;
            ctx.isInfix[ctx.current] = (*ctx.ptr == '{');
            ctx.ptr++;
        }
        break;
        case '}':
        case ')':
        {
            if (ctx.current == 0)
            {
                REDUCT_ERROR_SYNTAX(reduct->error, input, ctx.ptr, "unexpected ')'");
            }

            if ((*ctx.ptr == ')' && ctx.isInfix[ctx.current]) || (*ctx.ptr == '}' && !ctx.isInfix[ctx.current]))
            {
                REDUCT_ERROR_SYNTAX(reduct->error, input, ctx.ptr, "mismatched parentheses");
            }

            reduct_list_t* child = ctx.stack[ctx.current];
            reduct_bool_t childIsInfix = ctx.isInfix[ctx.current];

            ctx.current--;
            ctx.ptr++;

            if (childIsInfix)
            {
                reduct_handle_t transformed = reduct_parse_infix_transform(reduct, child);
                reduct_list_append(reduct, ctx.stack[ctx.current], transformed);
            }
            else
            {
                reduct_list_append(reduct, ctx.stack[ctx.current], REDUCT_HANDLE_FROM_LIST(child));
            }
        }
        break;
        default:
        {
            if (*ctx.ptr == '"')
            {
                reduct_parse_quoted_atom(reduct, &ctx);
            }
            else
            {
                reduct_parse_unquoted_atom(reduct, &ctx);
            }
        }
        break;
        }
    }

    if (ctx.current > 0)
    {
        REDUCT_ERROR_SYNTAX(reduct->error, ctx.input, ctx.ptr, "unexpected end of file, missing ')'");
    }

    return result;
}

REDUCT_API reduct_handle_t reduct_parse(reduct_t* reduct, const char* str, reduct_size_t len, const char* path)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(str != REDUCT_NULL);

    if (reduct == REDUCT_NULL || str == REDUCT_NULL)
    {
        REDUCT_ERROR_INTERNAL(reduct, "invalid arguments");
    }

    reduct_input_t* input = reduct_input_new(reduct, str, len, path, REDUCT_INPUT_FLAG_NONE);
    if (input == REDUCT_NULL)
    {
        REDUCT_ERROR_INTERNAL(reduct, "out of memory");
    }

    return reduct_parse_input(reduct, input);
}

REDUCT_API reduct_handle_t reduct_parse_file(reduct_t* reduct, const char* path)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(path != REDUCT_NULL);

    reduct_file_t file = REDUCT_FOPEN(path, "rb");
    if (file == REDUCT_NULL)
    {
        REDUCT_ERROR_INTERNAL(reduct, "could not open file '%s'", path);
    }

    fseek(file, 0, SEEK_END);
    reduct_size_t len = (reduct_size_t)ftell(file);
    fseek(file, 0, SEEK_SET);

    if (len == (reduct_size_t)-1)
    {
        REDUCT_FCLOSE(file);
        REDUCT_ERROR_INTERNAL(reduct, "could not read file '%s'", path);
    }

    reduct_size_t allocLen = len == 0 ? 1 : len;
    char* buffer = REDUCT_MALLOC(allocLen);
    if (buffer == REDUCT_NULL)
    {
        REDUCT_FCLOSE(file);
        REDUCT_ERROR_INTERNAL(reduct, "out of memory");
    }

    if (len > 0 && REDUCT_FREAD(buffer, 1, len, file) != len)
    {
        REDUCT_FREE(buffer);
        REDUCT_FCLOSE(file);
        REDUCT_ERROR_INTERNAL(reduct, "could not read file '%s'", path);
    }
    REDUCT_FCLOSE(file);

    reduct_input_t* input = reduct_input_new(reduct, buffer, len, path, REDUCT_INPUT_FLAG_OWNED);
    if (input == REDUCT_NULL)
    {
        REDUCT_FREE(buffer);
        REDUCT_ERROR_INTERNAL(reduct, "out of memory");
    }

    return reduct_parse_input(reduct, input);
}

#endif
