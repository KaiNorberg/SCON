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
} scon_parse_ctx_t;

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
    while (ctx->ptr < ctx->input->end && !SCON_CHAR_IS_WHITESPACE(*ctx->ptr) && *ctx->ptr != '(' && *ctx->ptr != ')')
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

SCON_API scon_handle_t scon_parse(scon_t* scon, const char* str, scon_size_t len, const char* path)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(str != SCON_NULL);

    if (scon == SCON_NULL || str == SCON_NULL)
    {
        SCON_ERROR_INTERNAL(scon, "invalid arguments");
    }

    scon_input_t* input = scon_input_new(scon, str, len, path);
    if (input == SCON_NULL)
    {
        SCON_ERROR_INTERNAL(scon, "out of memory");
    }

    scon_list_t* root = scon_list_new(scon, 0);
    if (root == SCON_NULL)
    {
        SCON_ERROR_INTERNAL(scon, "out of memory");
    }

    scon_item_t* rootItem = SCON_CONTAINER_OF(root, scon_item_t, list);
    rootItem->input = input;
    scon_handle_t result = SCON_HANDLE_FROM_ITEM(rootItem);
    SCON_GC_RETAIN(scon, result);

    scon_parse_ctx_t ctx;
    ctx.ptr = str;
    ctx.input = input;
    ctx.current = 0;
    ctx.stack[0] = root;

    while (1)
    {
        scon_parse_whitespace(&ctx);

        if (ctx.ptr >= ctx.input->end)
        {
            break;
        }

        switch (*ctx.ptr)
        {
        case '(':
        {
            if (ctx.current + 1 >= SCON_PARSE_STACK_MAX)
            {
                SCON_ERROR_SYNTAX(scon->error, input, ctx.ptr, "maximum nesting depth exceeded");
            }

            scon_list_t* child = scon_list_new(scon, 0);
            scon_item_t* item = SCON_CONTAINER_OF(child, scon_item_t, list);
            item->input = input;
            item->position = (scon_size_t)(ctx.ptr - str) + 1;

            scon_list_append(scon, ctx.stack[ctx.current], SCON_HANDLE_FROM_ITEM(item));
            ctx.stack[++ctx.current] = child;
            ctx.ptr++;
        }
        break;
        case ')':
        {
            if (ctx.current == 0)
            {
                SCON_ERROR_SYNTAX(scon->error, input, ctx.ptr, "unexpected ')'");
            }

            ctx.current--;
            ctx.ptr++;
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

    scon_size_t alloc_len = len == 0 ? 1 : len;
    char* buffer = SCON_MALLOC(alloc_len);
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

    return scon_parse(scon, buffer, len, path);
}

#endif
