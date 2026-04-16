#ifndef SCON_PARSE_IMPL_H
#define SCON_PARSE_IMPL_H 1

#include "core_internal.h"
#include "item_api.h"
#include "list_internal.h"
#include "parse_api.h"
#include "parse_internal.h"
#include "char_internal.h"
#include "atom_internal.h"
#include "gc_internal.h"

#define _SCON_PARSE_STACK_MAX 256

typedef struct
{
    const char* ptr;
    const char* end;
    scon_size_t current;
    _scon_list_t* stack[_SCON_PARSE_STACK_MAX];
} scon_parse_ctx_t;

static void _scon_parse_whitespace(scon_parse_ctx_t* ctx)
{
    while (ctx->ptr < ctx->end)
    {
        if (_SCON_CHAR_IS_WHITESPACE(*ctx->ptr))
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

static void _scon_parse_quoted_atom(scon_t* scon, scon_parse_ctx_t* ctx)
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
    _scon_atom_t* atom = _scon_atom_lookup(scon, start, len);
    _scon_atom_normalize_escape(scon, atom);
    
    _scon_node_t* node = SCON_CONTAINER_OF(atom, _scon_node_t, atom);
    node->position = (scon_size_t)(start - ctx->end);
    node->flags |= _SCON_NODE_FLAG_QUOTED;
    
    _scon_list_push_back(scon, ctx->stack[ctx->current], _SCON_ITEM_FROM_NODE(node));
    ctx->ptr++;
}

static void _scon_parse_unquoted_atom(scon_t* scon, scon_parse_ctx_t* ctx)
{
    const char* start = ctx->ptr;
    while (ctx->ptr < ctx->end && !_SCON_CHAR_IS_WHITESPACE(*ctx->ptr) && *ctx->ptr != '(' && *ctx->ptr != ')')
    {
        ctx->ptr++;
    }

    scon_size_t len = (scon_size_t)(ctx->ptr - start);
    if (len == 0)
    {
        return;
    }

    _scon_atom_t* atom = _scon_atom_lookup(scon, start, len);
    _scon_node_t* node = SCON_CONTAINER_OF(atom, _scon_node_t, atom);
    node->position = (scon_size_t)(start - ctx->end + scon->input->length);
    _scon_list_push_back(scon, ctx->stack[ctx->current], _SCON_ITEM_FROM_NODE(node));
}

SCON_API scon_item_t scon_parse(scon_t* scon, const char* str, scon_size_t len, const char* path)
{
    if (scon == SCON_NULL || str == SCON_NULL)
    {
        SCON_THROW(scon, "invalid arguments");
    }

    _scon_input_t* input = scon_input_new(scon, str, len, path);
    if (input == SCON_NULL)
    {
        SCON_THROW(scon, "out of memory");
    }

    _scon_list_t* root = _scon_list_new(scon, 0);
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
        _scon_parse_whitespace(&ctx);

        if (ctx.ptr >= ctx.end)
        {
            break;
        }

        switch (*ctx.ptr)
        {
        case '(':
        {
            if (ctx.current + 1 >= _SCON_PARSE_STACK_MAX)
            {
                SCON_THROW_POS(scon, "maximum nesting depth exceeded", (scon_size_t)(ctx.ptr - str));
            }

            _scon_list_t* child = _scon_list_new(scon, 0);
            _scon_node_t* node = SCON_CONTAINER_OF(child, _scon_node_t, list);
            node->input = input;
            node->position = (scon_size_t)(ctx.ptr - str) + 1;

            _scon_list_push_back(scon, ctx.stack[ctx.current], _SCON_ITEM_FROM_NODE(node));
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
                _scon_parse_quoted_atom(scon, &ctx);
            }
            else
            {
                _scon_parse_unquoted_atom(scon, &ctx);
            }
        }
        break;
        }
    }

    if (ctx.current > 0)
    {
        SCON_THROW_POS(scon, "unexpected end of file, missing ')'", len);
    }

    scon_item_t result = _SCON_ITEM_FROM_NODE(SCON_CONTAINER_OF(root, _scon_node_t, list));
    scon_gc_retain(scon, result);
    return result;
}

SCON_API scon_item_t scon_parse_file(scon_t* scon, const char* path)
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
