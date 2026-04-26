#ifndef SCON_PARSE_IMPL_H
#define SCON_PARSE_IMPL_H 1

#include "atom.h"
#include "char.h"
#include "core.h"
#include "error.h"
#include "gc.h"
#include "item.h"
#include "list.h"
#include "parse.h"

#define SCON_PARSE_STACK_MAX 256

typedef struct
{
    const char* ptr;
    scon_input_t* input;
    scon_size_t current;
    scon_list_t* stack[SCON_PARSE_STACK_MAX];
    scon_bool_t isInfix[SCON_PARSE_STACK_MAX];
} scon_parse_ctx_t;

typedef enum
{
    SCON_PARSE_PRECEDENCE_NONE = 0,
    SCON_PARSE_PRECEDENCE_LOGICAL_OR,
    SCON_PARSE_PRECEDENCE_LOGICAL_AND,
    SCON_PARSE_PRECEDENCE_COMPARISON,
    SCON_PARSE_PRECEDENCE_SHIFT,
    SCON_PARSE_PRECEDENCE_BITWISE,
    SCON_PARSE_PRECEDENCE_ADD_SUB,
    SCON_PARSE_PRECEDENCE_MUL_DIV,
    SCON_PARSE_PRECEDENCE_UNARY,   
    SCON_PARSE_PRECEDENCE_MAX,
} scon_parse_precedence_t;

static scon_parse_precedence_t scon_parse_get_precedence(scon_handle_t handle, scon_bool_t unary)
{
    if (!SCON_HANDLE_IS_ITEM(&handle))
    {
        return SCON_PARSE_PRECEDENCE_NONE;
    }
    scon_item_t* item = SCON_HANDLE_TO_ITEM(&handle);
    if (item->type != SCON_ITEM_TYPE_ATOM)
    {
        return SCON_PARSE_PRECEDENCE_NONE;
    }
    scon_atom_t* atom = &item->atom;

    if (unary)
    {
        if (scon_atom_is_equal(atom, "not", 3) || scon_atom_is_equal(atom, "-", 1))
        {
            return SCON_PARSE_PRECEDENCE_UNARY;
        }
        return SCON_PARSE_PRECEDENCE_NONE;
    }

    if (scon_atom_is_equal(atom, "or", 2))
    {
        return SCON_PARSE_PRECEDENCE_LOGICAL_OR;
    }
    if (scon_atom_is_equal(atom, "and", 3))
    {
        return SCON_PARSE_PRECEDENCE_LOGICAL_AND;
    }
    if (scon_atom_is_equal(atom, "==", 2) || scon_atom_is_equal(atom, "!=", 2) ||
        scon_atom_is_equal(atom, "<", 1) || scon_atom_is_equal(atom, "<=", 2) ||
        scon_atom_is_equal(atom, ">", 1) || scon_atom_is_equal(atom, ">=", 2))
    {
        return SCON_PARSE_PRECEDENCE_COMPARISON;
    }
    if (scon_atom_is_equal(atom, "<<", 2) || scon_atom_is_equal(atom, ">>", 2))
    {
        return SCON_PARSE_PRECEDENCE_SHIFT;
    }
    if (scon_atom_is_equal(atom, "&", 1) || scon_atom_is_equal(atom, "|", 1) ||
        scon_atom_is_equal(atom, "^", 1))
    {
        return SCON_PARSE_PRECEDENCE_BITWISE;
    }
    if (scon_atom_is_equal(atom, "*", 1) || scon_atom_is_equal(atom, "/", 1) ||
        scon_atom_is_equal(atom, "%", 1))
    {
        return SCON_PARSE_PRECEDENCE_MUL_DIV;
    }
    if (scon_atom_is_equal(atom, "+", 1) || scon_atom_is_equal(atom, "-", 1))
    {
        return SCON_PARSE_PRECEDENCE_ADD_SUB;
    }

    return SCON_PARSE_PRECEDENCE_NONE;
}

static scon_handle_t scon_parse_infix_transform_recursive(scon_t* scon, scon_list_t* list, scon_size_t* pos, scon_parse_precedence_t minPrecedence)
{
    if (*pos >= list->length)
    {
        return scon_handle_nil(scon);
    }

    scon_handle_t first = scon_list_nth(scon, list, *pos);
    scon_handle_t left;
    scon_parse_precedence_t unaryPrecedence = scon_parse_get_precedence(first, SCON_TRUE);

    if (unaryPrecedence > SCON_PARSE_PRECEDENCE_NONE)
    {
        (*pos)++;
        scon_handle_t operand = scon_parse_infix_transform_recursive(scon, list, pos, unaryPrecedence);
        scon_list_t* call = scon_list_new(scon);
        scon_list_append(scon, call, first);
        scon_list_append(scon, call, operand);
        left = SCON_HANDLE_FROM_ITEM(SCON_CONTAINER_OF(call, scon_item_t, list));
    }
    else
    {
        left = first;
        (*pos)++;
    }

    while (*pos < list->length)
    {
        scon_handle_t op = scon_list_nth(scon, list, *pos);
        scon_parse_precedence_t prec = scon_parse_get_precedence(op, SCON_FALSE);
        if (prec == SCON_PARSE_PRECEDENCE_NONE || prec < minPrecedence)
        {
            break;
        }

        (*pos)++;
        if (*pos >= list->length)
        {
            break;
        }

        scon_handle_t right = scon_parse_infix_transform_recursive(scon, list, pos, prec + 1);

        scon_list_t* call = scon_list_new(scon);
        scon_list_append(scon, call, op);
        scon_list_append(scon, call, left);
        scon_list_append(scon, call, right);
        left = SCON_HANDLE_FROM_ITEM(SCON_CONTAINER_OF(call, scon_item_t, list));
    }

    return left;
}

static scon_handle_t scon_parse_infix_transform(scon_t* scon, scon_list_t* list)
{
    if (list->length == 0)
    {
        return scon_handle_nil(scon);
    }

    scon_size_t pos = 0;
    return scon_parse_infix_transform_recursive(scon, list, &pos, SCON_PARSE_PRECEDENCE_LOGICAL_OR);
}

static void scon_parse_whitespace(scon_parse_ctx_t* ctx)
{
    SCON_ASSERT(ctx != SCON_NULL);

    while (ctx->ptr < ctx->input->end)
    {
        if (SCON_CHAR_IS_WHITESPACE(*ctx->ptr))
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

static void scon_parse_quoted_atom(scon_t* scon, scon_parse_ctx_t* ctx)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(ctx != SCON_NULL);

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
        SCON_ERROR_SYNTAX(scon->error, ctx->input, start, "unexpected end of file, missing '\"'");
    }

    scon_size_t len = (scon_size_t)(ctx->ptr - start);
    scon_atom_t* atom = scon_atom_lookup(scon, start, len, SCON_ATOM_LOOKUP_QUOTED);

    scon_item_t* item = SCON_CONTAINER_OF(atom, scon_item_t, atom);
    item->input = ctx->input;
    item->position = (scon_size_t)(start - ctx->input->buffer);
    item->flags |= SCON_ITEM_FLAG_QUOTED;

    scon_atom_normalize(scon, atom);

    scon_list_append(scon, ctx->stack[ctx->current], SCON_HANDLE_FROM_ITEM(item));
    ctx->ptr++;
}

static void scon_parse_unquoted_atom(scon_t* scon, scon_parse_ctx_t* ctx)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(ctx != SCON_NULL);

    const char* start = ctx->ptr;
    while (ctx->ptr < ctx->input->end && !SCON_CHAR_IS_WHITESPACE(*ctx->ptr) && 
           *ctx->ptr != '(' && *ctx->ptr != ')' && *ctx->ptr != '{' && *ctx->ptr != '}')
    {
        ctx->ptr++;
    }

    scon_size_t len = (scon_size_t)(ctx->ptr - start);
    if (len == 0)
    {
        return;
    }

    scon_atom_t* atom = scon_atom_lookup(scon, start, len, SCON_ATOM_LOOKUP_NONE);
    scon_item_t* item = SCON_CONTAINER_OF(atom, scon_item_t, atom);
    item->input = ctx->input;
    item->position = (scon_size_t)(start - ctx->input->buffer);

    scon_atom_normalize(scon, atom);

    scon_list_append(scon, ctx->stack[ctx->current], SCON_HANDLE_FROM_ITEM(item));
}

SCON_API scon_handle_t scon_parse_input(scon_t* scon, scon_input_t* input)
{
    scon_list_t* root = scon_list_new(scon);
    if (root == SCON_NULL)
    {
        SCON_ERROR_INTERNAL(scon, "out of memory");
    }

    scon_item_t* rootItem = SCON_CONTAINER_OF(root, scon_item_t, list);
    rootItem->input = input;
    scon_handle_t result = SCON_HANDLE_FROM_ITEM(rootItem);
    SCON_GC_RETAIN(scon, result);

    scon_parse_ctx_t ctx;
    ctx.ptr = input->buffer;
    ctx.input = input;
    ctx.current = 0;
    ctx.stack[0] = root;
    ctx.isInfix[0] = SCON_FALSE;

    while (1)
    {
        scon_parse_whitespace(&ctx);

        if (ctx.ptr >= ctx.input->end)
        {
            break;
        }

        switch (*ctx.ptr)
        {
        case '{':
        case '(':
        {
            if (ctx.current + 1 >= SCON_PARSE_STACK_MAX)
            {
                SCON_ERROR_SYNTAX(scon->error, input, ctx.ptr, "maximum nesting depth exceeded");
            }

            scon_list_t* child = scon_list_new(scon);
            scon_item_t* item = SCON_CONTAINER_OF(child, scon_item_t, list);
            item->input = input;
            item->position = (scon_size_t)(ctx.ptr - input->buffer) + 1;

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
                SCON_ERROR_SYNTAX(scon->error, input, ctx.ptr, "unexpected ')'");
            }

            if ((*ctx.ptr == ')' && ctx.isInfix[ctx.current]) || (*ctx.ptr == '}' && !ctx.isInfix[ctx.current]))
            {
                SCON_ERROR_SYNTAX(scon->error, input, ctx.ptr, "mismatched parentheses");
            }

            scon_list_t* child = ctx.stack[ctx.current];
            scon_bool_t childIsInfix = ctx.isInfix[ctx.current];

            ctx.current--;
            ctx.ptr++;            
            
            if (childIsInfix)
            {
                scon_handle_t transformed = scon_parse_infix_transform(scon, child);
                scon_list_append(scon, ctx.stack[ctx.current], transformed);
            }
            else
            {
                scon_list_append(scon, ctx.stack[ctx.current], SCON_HANDLE_FROM_LIST(child));
            }
        }
        break;
        default:
        {
            if (*ctx.ptr == '"')
            {
                scon_parse_quoted_atom(scon, &ctx);
            }
            else
            {
                scon_parse_unquoted_atom(scon, &ctx);
            }
        }
        break;
        }
    }

    if (ctx.current > 0)
    {
        SCON_ERROR_SYNTAX(scon->error, ctx.input, ctx.ptr, "unexpected end of file, missing ')'");
    }

    return result;
}

SCON_API scon_handle_t scon_parse(scon_t* scon, const char* str, scon_size_t len, const char* path)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(str != SCON_NULL);

    if (scon == SCON_NULL || str == SCON_NULL)
    {
        SCON_ERROR_INTERNAL(scon, "invalid arguments");
    }

    scon_input_t* input = scon_input_new(scon, str, len, path, SCON_INPUT_FLAG_NONE);
    if (input == SCON_NULL)
    {
        SCON_ERROR_INTERNAL(scon, "out of memory");
    }

    return scon_parse_input(scon, input);
}

SCON_API scon_handle_t scon_parse_file(scon_t* scon, const char* path)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(path != SCON_NULL);

    scon_file_t file = SCON_FOPEN(path, "rb");
    if (file == SCON_NULL)
    {
        SCON_ERROR_INTERNAL(scon, "could not open file '%s'", path);
    }

    fseek(file, 0, SEEK_END);
    scon_size_t len = (scon_size_t)ftell(file);
    fseek(file, 0, SEEK_SET);

    if (len == (scon_size_t)-1)
    {
        SCON_FCLOSE(file);
        SCON_ERROR_INTERNAL(scon, "could not read file '%s'", path);
    }

    scon_size_t allocLen = len == 0 ? 1 : len;
    char* buffer = SCON_MALLOC(allocLen);
    if (buffer == SCON_NULL)
    {
        SCON_FCLOSE(file);
        SCON_ERROR_INTERNAL(scon, "out of memory");
    }

    if (len > 0 && SCON_FREAD(buffer, 1, len, file) != len)
    {
        SCON_FREE(buffer);
        SCON_FCLOSE(file);
        SCON_ERROR_INTERNAL(scon, "could not read file '%s'", path);
    }
    SCON_FCLOSE(file);

    scon_input_t* input = scon_input_new(scon, buffer, len, path, SCON_INPUT_FLAG_OWNED);
    if (input == SCON_NULL)
    {
        SCON_FREE(buffer);
        SCON_ERROR_INTERNAL(scon, "out of memory");
    }

    return scon_parse_input(scon, input);
}

#endif
