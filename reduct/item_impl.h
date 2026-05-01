#ifndef REDUCT_ITEM_IMPL_H
#define REDUCT_ITEM_IMPL_H 1

#include "defs.h"
#include "closure.h"
#include "core.h"
#include "function.h"
#include "gc.h"
#include "item.h"

static inline void reduct_item_init(reduct_item_t* item)
{
    item->type = REDUCT_ITEM_TYPE_NONE;
    item->flags = 0;
    item->inputId = REDUCT_INPUT_ID_NONE;
    item->position = 0;
}

static inline reduct_item_t* reduct_item_pop_free_list(reduct_t* reduct)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);

    reduct_item_t* item = reduct->freeList;
    reduct->freeList = item->free;
    reduct->freeCount--;
    return item;
}

REDUCT_API reduct_item_t* reduct_item_new(reduct_t* reduct)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);

    if (REDUCT_LIKELY(reduct->freeList != REDUCT_NULL))
    {
        return reduct_item_pop_free_list(reduct);
    }

    reduct_item_t* item = REDUCT_NULL;
    void* allocated = REDUCT_CALLOC(1, sizeof(reduct_item_block_t));
    if (allocated == REDUCT_NULL)
    {
        REDUCT_ERROR_INTERNAL(reduct, "out of memory");
    }
    reduct_item_block_t* block = (reduct_item_block_t*)REDUCT_ROUND_UP((reduct_size_t)allocated, REDUCT_ALIGNMENT);
    block->allocated = allocated;
    reduct->blockCount++;

    for (reduct_size_t i = 1; i < REDUCT_ITEM_BLOCK_MAX - 1; i++)
    {
        block->items[i].free = &block->items[i + 1];
    }
    block->items[REDUCT_ITEM_BLOCK_MAX - 1].free = REDUCT_NULL;
    reduct->freeCount += REDUCT_ITEM_BLOCK_MAX - 1;

    reduct->freeList = &block->items[1];
    block->next = reduct->block;
    reduct->block = block;

    item = &block->items[0];
    return item;
}


REDUCT_API void reduct_item_deinit(reduct_t* reduct, reduct_item_t* item)
{
    switch (item->type)
    {
    case REDUCT_ITEM_TYPE_ATOM:
    {
        reduct_atom_t* atom = &item->atom;
        if (reduct->atomMap != REDUCT_NULL && atom->index != REDUCT_ATOM_INDEX_NONE)
        {
            reduct->atomMap[atom->index] = REDUCT_ATOM_TOMBSTONE;
            reduct->atomMapTombstones++;
            reduct->atomMapSize--;
        }
        break;
    }
    case REDUCT_ITEM_TYPE_ATOM_STACK:
    {   
        reduct_atom_stack_t* stack = &item->atomStack;
        if (stack->next != REDUCT_NULL)
        {
            stack->next->prev = stack->prev;
        }
        if (stack->prev != REDUCT_NULL)
        {
            stack->prev->next = stack->next;
        }
        else if (reduct->atomStack == stack)
        {
            reduct->atomStack = stack->next;
        }
        if (stack->data != REDUCT_NULL)
        {
            REDUCT_FREE(stack->data);
        }
        break;
    }
    case REDUCT_ITEM_TYPE_FUNCTION:
    {
        reduct_function_t* func = &item->function;
        if (func->insts != REDUCT_NULL)
        {
            REDUCT_FREE(func->insts);
        }
        if (func->positions != REDUCT_NULL)
        {
            REDUCT_FREE(func->positions);
        }
        if (func->constants != REDUCT_NULL)
        {
            REDUCT_FREE(func->constants);
        }
        break;
    }
    case REDUCT_ITEM_TYPE_CLOSURE:
    {
        reduct_closure_t* closure = &item->closure;
        if (closure->constants != closure->smallConstants)
        {
            REDUCT_FREE(closure->constants);
        }
        break;
    }
    default:
        break;
    }

    reduct_item_init(item);

#ifndef NDEBUG
    REDUCT_MEMSET(&item->atom, 0xFE, sizeof(reduct_item_t) - offsetof(reduct_item_t, atom));
#endif
}

REDUCT_API void reduct_item_free(reduct_t* reduct, reduct_item_t* item)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(item != REDUCT_NULL);

    reduct_item_deinit(reduct, item);

    reduct->freeCount++;
    item->free = reduct->freeList;
    reduct->freeList = item;
}

REDUCT_API const char* reduct_item_type_str(reduct_item_type_t type)
{
    switch (type)
    {
    case REDUCT_ITEM_TYPE_NONE:
        return "none";
    case REDUCT_ITEM_TYPE_ATOM:
        return "atom";
    case REDUCT_ITEM_TYPE_ATOM_STACK:
        return "atom stack";
    case REDUCT_ITEM_TYPE_LIST:
        return "list";
    case REDUCT_ITEM_TYPE_LIST_NODE:
        return "list node";
    case REDUCT_ITEM_TYPE_FUNCTION:
        return "function";
    case REDUCT_ITEM_TYPE_CLOSURE:
        return "closure";
    default:
        return "unknown";
    }
}

#endif
