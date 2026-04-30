#include "error.h"
#ifndef REDUCT_CORE_IMPL_H
#define REDUCT_CORE_IMPL_H 1

#include "atom.h"
#include "core.h"
#include "eval.h"
#include "gc.h"
#include "item.h"

REDUCT_API reduct_t* reduct_new(reduct_error_t* error)
{
    reduct_t* reduct = REDUCT_CALLOC(1, sizeof(reduct_t));
    if (reduct == REDUCT_NULL)
    {
        REDUCT_ERROR_THROW(error, REDUCT_NULL, INTERNAL, "out of memory");
    }
    reduct->error = error;

    reduct->gcThreshold = REDUCT_GC_THRESHOLD_INITIAL;

    reduct->trueItem = REDUCT_CONTAINER_OF(reduct_atom_new_int(reduct, 1), reduct_item_t, atom);
    reduct->falseItem = REDUCT_CONTAINER_OF(reduct_atom_new_int(reduct, 0), reduct_item_t, atom);
    reduct->nilItem = REDUCT_CONTAINER_OF(reduct_list_new(reduct), reduct_item_t, list);
    reduct->piItem = REDUCT_CONTAINER_OF(reduct_atom_new_float(reduct, REDUCT_PI), reduct_item_t, atom);
    reduct->eItem = REDUCT_CONTAINER_OF(reduct_atom_new_float(reduct, REDUCT_E), reduct_item_t, atom);

    reduct->falseItem->flags |= REDUCT_ITEM_FLAG_FALSY;
    reduct->nilItem->flags |= REDUCT_ITEM_FLAG_FALSY;
    reduct->trueItem->flags &= ~REDUCT_ITEM_FLAG_FALSY;

    reduct_constant_register(reduct, "true", reduct->trueItem);
    reduct_constant_register(reduct, "false", reduct->falseItem);
    reduct_constant_register(reduct, "nil", reduct->nilItem);
    reduct_constant_register(reduct, "pi", reduct->piItem);
    reduct_constant_register(reduct, "e", reduct->eItem);

    reduct_intrinsic_register_all(reduct);

    reduct->evalState = REDUCT_NULL;
    reduct->argc = 0;
    reduct->argv = REDUCT_NULL;

    return reduct;
}

REDUCT_API void reduct_free(reduct_t* reduct)
{
    if (reduct == REDUCT_NULL)
    {
        return;
    }

    for (reduct_size_t i = 0; i < reduct->scratchCapacity; i++)
    {
        if (reduct->scratch[i].buffer != REDUCT_NULL)
        {
            REDUCT_FREE(reduct->scratch[i].buffer);
        }
    }
    reduct->scratchCapacity = 0;

    if (reduct->atomMap != REDUCT_NULL)
    {
        REDUCT_FREE(reduct->atomMap);
        reduct->atomMap = REDUCT_NULL;
        reduct->atomMapCapacity = 0;
        reduct->atomMapSize = 0;
    }

    reduct_item_block_t* block = reduct->block;
    while (block != REDUCT_NULL)
    {
        reduct_item_block_t* next = block->next;
        for (int i = 0; i < REDUCT_ITEM_BLOCK_MAX; i++)
        {
            reduct_item_t* item = &block->items[i];
            reduct_item_free(reduct, item);
        }

        if (block != &reduct->firstBlock)
        {
            REDUCT_FREE(block);
        }
        block = next;
    }

    reduct_input_t* input = reduct->input;
    while (input != REDUCT_NULL)
    {
        reduct_input_t* next = input->prev;
        if (input->flags & REDUCT_INPUT_FLAG_OWNED)
        {
            REDUCT_FREE((void*)input->buffer);
        }
        if (input != &reduct->firstInput)
        {
            REDUCT_FREE(input);
        }
        input = next;
    }

    if (reduct->evalState != REDUCT_NULL)
    {
        reduct_eval_state_deinit(reduct->evalState);
        REDUCT_FREE(reduct->evalState);
    }

    REDUCT_FREE(reduct);
}

REDUCT_API void reduct_args_set(reduct_t* reduct, int argc, char** argv)
{
    reduct->argc = argc;
    reduct->argv = argv;
}

REDUCT_API void reduct_constant_register(reduct_t* reduct, const char* name, reduct_item_t* item)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(name != REDUCT_NULL);
    REDUCT_ASSERT(item != REDUCT_NULL);

    if (reduct->constantCount >= REDUCT_CONSTANTS_MAX)
    {
        REDUCT_ERROR_THROW(reduct->error, item, INTERNAL, "too many constants");
    }

    reduct_size_t len = REDUCT_STRLEN(name);
    reduct_atom_t* atom = reduct_atom_lookup(reduct, name, len, REDUCT_ATOM_LOOKUP_NONE);

    reduct->constants[reduct->constantCount].name = atom;
    reduct->constants[reduct->constantCount].item = item;
    reduct->constantCount++;
}

REDUCT_API reduct_input_t* reduct_input_new(reduct_t* reduct, const char* buffer, reduct_size_t length,
    const char* path, reduct_input_flags_t flags)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(buffer != REDUCT_NULL);
    REDUCT_ASSERT(path != REDUCT_NULL);

    reduct_input_t* input;
    if (reduct->input == REDUCT_NULL)
    {
        input = &reduct->firstInput;
    }
    else
    {
        input = REDUCT_MALLOC(sizeof(reduct_input_t));
        if (input == REDUCT_NULL)
        {
            REDUCT_ERROR_INTERNAL(reduct, "out of memory");
        }
    }
    input->prev = reduct->input;
    input->buffer = buffer;
    input->end = buffer + length;
    input->flags = flags;
    REDUCT_STRNCPY(input->path, path, REDUCT_PATH_MAX - 1);
    input->path[REDUCT_PATH_MAX - 1] = '\0';
    reduct->input = input;
    return input;
}

#endif
