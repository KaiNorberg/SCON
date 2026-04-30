#include "defs.h"
#ifndef REDUCT_ITEM_IMPL_H
#define REDUCT_ITEM_IMPL_H 1

#include "closure.h"
#include "core.h"
#include "function.h"
#include "gc.h"
#include "item.h"

static inline void reduct_item_init(reduct_item_t* item)
{
    item->type = REDUCT_ITEM_TYPE_NONE;
    item->flags = 0;
    item->input = REDUCT_NULL;
    item->position = 0;
}

static inline reduct_item_t* reduct_item_pop_free_list(reduct_t* reduct)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);

    reduct_item_t* item = reduct->freeList;
    reduct->freeList = item->free;
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
    reduct_item_block_t* block;
    if (reduct->block == REDUCT_NULL)
    {
        block = &reduct->firstBlock;
    }
    else
    {
        block = REDUCT_CALLOC(1, sizeof(reduct_item_block_t));
        if (block == REDUCT_NULL)
        {
            REDUCT_ERROR_INTERNAL(reduct, "out of memory");
        }
    }
    reduct->blocksAllocated++;

    for (reduct_size_t i = 1; i < REDUCT_ITEM_BLOCK_MAX - 1; i++)
    {
        block->items[i].free = &block->items[i + 1];
    }
    block->items[REDUCT_ITEM_BLOCK_MAX - 1].free = REDUCT_NULL;

    reduct->freeList = &block->items[1];
    block->next = reduct->block;
    reduct->block = block;

    item = &block->items[0];
    return item;
}

REDUCT_API void reduct_item_free(reduct_t* reduct, reduct_item_t* item)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(item != REDUCT_NULL);

    switch (item->type)
    {
    case REDUCT_ITEM_TYPE_ATOM:
    {
        reduct_atom_deinit(reduct, &item->atom);
        break;
    }
    case REDUCT_ITEM_TYPE_FUNCTION:
    {
        reduct_function_deinit(&item->function);
        break;
    }
    case REDUCT_ITEM_TYPE_CLOSURE:
    {
        reduct_closure_deinit(&item->closure);
        break;
    }
    default:
        break;
    }

    reduct_item_init(item);

#ifndef NDEBUG
    REDUCT_MEMSET(&item->atom, 0xFE, sizeof(reduct_item_t) - offsetof(reduct_item_t, atom));
#endif

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
    case REDUCT_ITEM_TYPE_LIST:
        return "list";
    case REDUCT_ITEM_TYPE_FUNCTION:
        return "function";
    case REDUCT_ITEM_TYPE_CLOSURE:
        return "closure";
    case REDUCT_ITEM_TYPE_LIST_NODE:
        return "list node";
    default:
        return "unknown";
    }
}

#endif
