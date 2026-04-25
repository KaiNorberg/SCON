#ifndef SCON_ITEM_IMPL_H
#define SCON_ITEM_IMPL_H 1

#include "closure.h"
#include "core.h"
#include "function.h"
#include "gc.h"
#include "item.h"

static inline void scon_item_init(scon_item_t* item)
{
    item->type = SCON_ITEM_TYPE_NONE;
    item->flags = 0;
    item->retainCount = 0;
    item->length = 0;
    item->input = SCON_NULL;
    item->position = 0;
}

static inline scon_item_t* scon_item_pop_free_list(scon_t* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_item_t* item = scon->freeList;
    scon->freeList = item->free;
    scon_item_init(item);
    return item;
}

SCON_API scon_item_t* scon_item_new(scon_t* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (SCON_UNLIKELY(scon->freeList == SCON_NULL))
    {
        scon_gc_if_needed(scon);
    }

    if (scon->freeList != SCON_NULL)
    {
        return scon_item_pop_free_list(scon);
    }

    scon_item_t* item = SCON_NULL;
    scon_item_block_t* block;
    if (scon->block == SCON_NULL)
    {
        block = &scon->firstBlock;
    }
    else
    {
        block = SCON_MALLOC(sizeof(scon_item_block_t));
        if (block == SCON_NULL)
        {
            SCON_ERROR_INTERNAL(scon, "out of memory");
        }
    }
    scon->blocksAllocated++;

    for (scon_size_t i = 1; i < SCON_ITEM_BLOCK_MAX - 1; i++)
    {
        block->items[i].free = &block->items[i + 1];
    }

    block->items[SCON_ITEM_BLOCK_MAX - 1].free = SCON_NULL;
    scon->freeList = &block->items[1];
    block->next = scon->block;
    scon->block = block;

    item = &block->items[0];
    scon_item_init(item);
    return item;
}

SCON_API void scon_item_free(scon_t* scon, scon_item_t* item)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(item != SCON_NULL);

    switch (item->type)
    {
    case SCON_ITEM_TYPE_ATOM:
    {
        scon_atom_deinit(scon, &item->atom);
        break;
    }
    case SCON_ITEM_TYPE_FUNCTION:
    {
        scon_function_deinit(&item->function);
        break;
    }
    case SCON_ITEM_TYPE_CLOSURE:
    {
        scon_closure_deinit(&item->closure);
        break;
    }
    default:
        break;
    }

    scon_item_init(item);

#ifndef NDEBUG
    SCON_MEMSET(&item->atom, 0xFE, sizeof(scon_item_t) - offsetof(scon_item_t, atom));
#endif

    item->free = scon->freeList;
    scon->freeList = item;
}

SCON_API scon_int64_t scon_item_get_int(scon_item_t* item)
{
    if (item->flags & SCON_ITEM_FLAG_INT_SHAPED)
    {
        return item->atom.integerValue;
    }
    if (item->flags & SCON_ITEM_FLAG_FLOAT_SHAPED)
    {
        return (scon_int64_t)item->atom.floatValue;
    }
    return 0;
}

SCON_API scon_float_t scon_item_get_float(scon_item_t* item)
{
    if (item->flags & SCON_ITEM_FLAG_FLOAT_SHAPED)
    {
        return item->atom.floatValue;
    }
    if (item->flags & SCON_ITEM_FLAG_INT_SHAPED)
    {
        return (scon_float_t)item->atom.integerValue;
    }
    return 0.0;
}

SCON_API const char* scon_item_type_str(scon_item_type_t type)
{
    switch (type)
    {
    case SCON_ITEM_TYPE_NONE:
        return "none";
    case SCON_ITEM_TYPE_ATOM:
        return "atom";
    case SCON_ITEM_TYPE_LIST:
        return "list";
    case SCON_ITEM_TYPE_FUNCTION:
        return "function";
    case SCON_ITEM_TYPE_CLOSURE:
        return "closure";
    case SCON_ITEM_TYPE_LIST_NODE:
        return "list node";
    default:
        return "unknown";
    }
}

#endif
