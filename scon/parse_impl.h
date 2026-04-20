#ifndef SCON_PARSE_IMPL_H
#define SCON_PARSE_IMPL_H 1

#include "atom.h"
#include "char.h"
#include "core.h"
#include "gc.h"
#include "item.h"
#include "list.h"
#include "parse.h"

#define SCON_PARSE_STACK_MAX 256

typedef struct
{
    const char* ptr;
    const char* end;
    scon_size_t current;
    scon_list_t* stack[SCON_PARSE_STACK_MAX];
} scon_parse_ctx_t;

static void scon_parse_whitespace(scon_parse_ctx_t* ctx)
{
    while (ctx->ptr < ctx->end)
    {
        if (SCON_CHAR_IS_WHITESPACE(*ctx->ptr))
        {
            ctx->ptr++;
        }
        else if (*ctx->ptr == '/' && ctx->ptr + 1 < ctx->end && *(ctx->ptr + 1) == '/')
        {
            while (ctx->ptr < ctx->end && *ctx->ptr != '\n')
            {
                ctx->ptr++;
            }
        }
        else if (*ctx->ptr == '/' && ctx->ptr + 1 < ctx->end && *(ctx->ptr + 1) == '*')
        {
            ctx->ptr += 2;
            while (ctx->ptr < ctx->end)
            {
                if (*ctx->ptr == '*' && ctx->ptr + 1 < ctx->end && *(ctx->ptr + 1) == '/')
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
    ctx->ptr++;
    const char* start = ctx->ptr;
    while (ctx->ptr < ctx->end)
    {
        if (*ctx->ptr == '\\' && ctx->ptr + 1 < ctx->end)
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

    if (ctx->ptr >= ctx->end)
    {
        SCON_THROW_POS(scon, "unexpected end of file, missing '\"'", (scon_size_t)(start - ctx->ptr));
    }

    scon_size_t len = (scon_size_t)(ctx->ptr - start);
    scon_atom_t* atom = scon_atom_lookup(scon, start, len);

    scon_item_t* item = SCON_CONTAINER_OF(atom, scon_item_t, atom);
    item->position = (scon_size_t)(start - ctx->end);
    item->flags |= SCON_ITEM_FLAG_QUOTED;

    scon_atom_normalize(scon, atom);

    scon_list_append(scon, ctx->stack[ctx->current], item);
    ctx->ptr++;
}

static void scon_parse_unquoted_atom(scon_t* scon, scon_parse_ctx_t* ctx)
{
    const char* start = ctx->ptr;
    while (ctx->ptr < ctx->end && !SCON_CHAR_IS_WHITESPACE(*ctx->ptr) && *ctx->ptr != '(' && *ctx->ptr != ')')
    {
        ctx->ptr++;
    }

    scon_size_t len = (scon_size_t)(ctx->ptr - start);
    if (len == 0)
    {
        return;
    }

    scon_atom_t* atom = scon_atom_lookup(scon, start, len);
    scon_item_t* item = SCON_CONTAINER_OF(atom, scon_item_t, atom);
    item->position = (scon_size_t)(start - ctx->end + scon->input->length);

    scon_atom_normalize(scon, atom);

    scon_list_append(scon, ctx->stack[ctx->current], item);
}

SCON_API scon_handle_t scon_parse(scon_t* scon, const char* str, scon_size_t len, const char* path)
{
    if (scon == SCON_NULL || str == SCON_NULL)
    {
        SCON_THROW(scon, "invalid arguments");
    }

    scon_input_t* input = scon_input_new(scon, str, len, path);
    if (input == SCON_NULL)
    {
        SCON_THROW(scon, "out of memory");
    }

    scon_list_t* root = scon_list_new(scon, 0);
    if (root == SCON_NULL)
    {
        SCON_THROW(scon, "out of memory");
    }

    scon_parse_ctx_t ctx;
    ctx.ptr = str;
    ctx.end = str + len;
    ctx.current = 0;
    ctx.stack[0] = root;

    while (1)
    {
        scon_parse_whitespace(&ctx);

        if (ctx.ptr >= ctx.end)
        {
            break;
        }

        switch (*ctx.ptr)
        {
        case '(':
        {
            if (ctx.current + 1 >= SCON_PARSE_STACK_MAX)
            {
                SCON_THROW_POS(scon, "maximum nesting depth exceeded", (scon_size_t)(ctx.ptr - str));
            }

            scon_list_t* child = scon_list_new(scon, 0);
            scon_item_t* item = SCON_CONTAINER_OF(child, scon_item_t, list);
            item->input = input;
            item->position = (scon_size_t)(ctx.ptr - str) + 1;

            scon_list_append(scon, ctx.stack[ctx.current], item);
            ctx.stack[++ctx.current] = child;
            ctx.ptr++;
        }
        break;
        case ')':
        {
            if (ctx.current == 0)
            {
                SCON_THROW_POS(scon, "unexpected ')'", (scon_size_t)(ctx.ptr - str));
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
        SCON_THROW_POS(scon, "unexpected end of file, missing ')'", len);
    }

    scon_handle_t result = SCON_HANDLE_FROM_ITEM(SCON_CONTAINER_OF(root, scon_item_t, list));
    scon_gc_retain(scon, result);
    return result;
}

SCON_API scon_handle_t scon_parse_file(scon_t* scon, const char* path)
{
    scon_file_t file = SCON_FOPEN(path, "rb");
    if (file == SCON_NULL)
    {
        SCON_THROW(scon, "could not open file");
    }

    fseek(file, 0, SEEK_END);
    scon_size_t len = (scon_size_t)ftell(file);
    fseek(file, 0, SEEK_SET);

    if (len == (scon_size_t)-1)
    {
        SCON_FCLOSE(file);
        SCON_THROW(scon, "could not read file");
    }

    scon_size_t alloc_len = len == 0 ? 1 : len;
    char* buffer = SCON_MALLOC(alloc_len);
    if (buffer == SCON_NULL)
    {
        SCON_FCLOSE(file);
        SCON_THROW(scon, "out of memory");
    }

    if (len > 0 && SCON_FREAD(buffer, 1, len, file) != len)
    {
        SCON_FREE(buffer);
        SCON_FCLOSE(file);
        SCON_THROW(scon, "could not read file");
    }
    SCON_FCLOSE(file);

    return scon_parse(scon, buffer, len, path);
}

#endif
