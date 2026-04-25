#include "error.h"
#ifndef SCON_CORE_IMPL_H
#define SCON_CORE_IMPL_H 1

#include "atom.h"
#include "core.h"
#include "eval.h"
#include "gc.h"
#include "item.h"

SCON_API scon_t* scon_new(scon_error_t* error)
{
    scon_t* scon = SCON_CALLOC(1, sizeof(scon_t));
    if (scon == SCON_NULL)
    {
        SCON_ERROR_THROW(error, SCON_NULL, INTERNAL, "out of memory");
    }
    scon->error = error;

    scon->gcThreshold = SCON_GC_THRESHOLD_INITIAL;

    scon->trueItem = SCON_CONTAINER_OF(scon_atom_lookup_int(scon, 1), scon_item_t, atom);
    scon->falseItem = SCON_CONTAINER_OF(scon_atom_lookup_int(scon, 0), scon_item_t, atom);
    scon->nilItem = SCON_CONTAINER_OF(scon_list_new(scon), scon_item_t, list);
    scon->piItem = SCON_CONTAINER_OF(scon_atom_lookup_float(scon, SCON_PI), scon_item_t, atom);
    scon->eItem = SCON_CONTAINER_OF(scon_atom_lookup_float(scon, SCON_E), scon_item_t, atom);

    scon->falseItem->flags |= SCON_ITEM_FLAG_FALSY;
    scon->nilItem->flags |= SCON_ITEM_FLAG_FALSY;
    scon->trueItem->flags &= ~SCON_ITEM_FLAG_FALSY;

    scon_constant_register(scon, "true", scon->trueItem);
    scon_constant_register(scon, "false", scon->falseItem);
    scon_constant_register(scon, "nil", scon->nilItem);
    scon_constant_register(scon, "pi", scon->piItem);
    scon_constant_register(scon, "e", scon->eItem);

    scon_intrinsic_register_all(scon);

    scon->evalState = SCON_NULL;
    scon->argc = 0;
    scon->argv = SCON_NULL;

    return scon;
}

SCON_API void scon_free(scon_t* scon)
{
    if (scon == SCON_NULL)
    {
        return;
    }

    scon_item_block_t* block = scon->block;
    while (block != SCON_NULL)
    {
        scon_item_block_t* next = block->next;
        for (int i = 0; i < SCON_ITEM_BLOCK_MAX; i++)
        {
            scon_item_t* item = &block->items[i];
            scon_item_free(scon, item);
        }

        if (block != &scon->firstBlock)
        {
            SCON_FREE(block);
        }
        block = next;
    }

    scon_input_t* input = scon->input;
    while (input != SCON_NULL)
    {
        scon_input_t* next = input->prev;
        if (input->flags & SCON_INPUT_FLAG_OWNED)
        {
            SCON_FREE((void*)input->buffer);
        }
        if (input != &scon->firstInput)
        {
            SCON_FREE(input);
        }
        input = next;
    }

    if (scon->evalState != SCON_NULL)
    {
        scon_eval_state_deinit(scon->evalState);
        SCON_FREE(scon->evalState);
    }

    SCON_FREE(scon);
}

SCON_API void scon_args_set(scon_t* scon, int argc, char** argv)
{
    scon->argc = argc;
    scon->argv = argv;
}

SCON_API void scon_constant_register(scon_t* scon, const char* name, scon_item_t* item)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(name != SCON_NULL);
    SCON_ASSERT(item != SCON_NULL);

    if (scon->constantCount >= SCON_CONSTANTS_MAX)
    {
        SCON_ERROR_THROW(scon->error, item, INTERNAL, "too many constants");
    }

    scon_size_t len = SCON_STRLEN(name);
    scon_atom_t* atom = scon_atom_lookup(scon, name, len, SCON_ATOM_LOOKUP_NONE);

    scon->constants[scon->constantCount].name = atom;
    scon->constants[scon->constantCount].item = item;
    scon->constantCount++;

    SCON_GC_RETAIN_ITEM(scon, SCON_CONTAINER_OF(atom, scon_item_t, atom));
    SCON_GC_RETAIN_ITEM(scon, item);
}

SCON_API scon_input_t* scon_input_new(scon_t* scon, const char* buffer, scon_size_t length, const char* path,
    scon_input_flags_t flags)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(buffer != SCON_NULL);
    SCON_ASSERT(path != SCON_NULL);

    scon_input_t* input;
    if (scon->input == SCON_NULL)
    {
        input = &scon->firstInput;
    }
    else
    {
        input = SCON_MALLOC(sizeof(scon_input_t));
        if (input == SCON_NULL)
        {
            SCON_ERROR_INTERNAL(scon, "out of memory");
        }
    }
    input->prev = scon->input;
    input->buffer = buffer;
    input->end = buffer + length;
    input->flags = flags;
    SCON_STRNCPY(input->path, path, SCON_PATH_MAX - 1);
    input->path[SCON_PATH_MAX - 1] = '\0';
    scon->input = input;
    return input;
}

#endif
