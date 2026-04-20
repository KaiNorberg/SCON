#include "item.h"
#ifndef SCON_GC_IMPL_H
#define SCON_GC_IMPL_H 1

#include "core.h"
#include "gc.h"
#include "item.h"
#include "list.h"

static void scon_gc_mark(scon_t* scon, scon_item_t* item)
{
    if (item->flags & SCON_ITEM_FLAG_GC_MARK)
    {
        return;
    }

    item->flags |= SCON_ITEM_FLAG_GC_MARK;
}

static void scon_gc_mark_list(scon_t* scon, scon_list_t* list)
{
    for (scon_uint32_t i = 0; i < list->length; i++)
    {
        scon_gc_mark(scon, list->items[i]);
    }
}

SCON_API void scon_gc_if_needed(scon_t* scon)
{
    if (scon->blocksAllocated >= scon->gcThreshold)
    {
        scon_gc(scon);
        scon->blocksAllocated = 0;
        scon->gcThreshold = scon->blocksAllocated + SCON_GC_THRESHOLD_INITIAL;
    }
}

SCON_API void scon_gc(scon_t* scon)
{
    scon_list_t* retained = &scon->retained;
    for (scon_uint32_t i = 0; i < retained->length; i++)
    {
        scon_gc_mark(scon, retained->items[i]);
    }

    scon->freeList = SCON_NULL;

    scon_item_block_t* block = scon->block;
    while (block != SCON_NULL)
    {
        for (int i = 0; i < SCON_ITEM_BLOCK_MAX; i++)
        {
            scon_item_t* item = &block->items[i];
            if (item->flags & SCON_ITEM_FLAG_GC_MARK)
            {
                item->flags &= ~SCON_ITEM_FLAG_GC_MARK;
            }
            else
            {
                scon_item_free(scon, item);
            }
        }
        block = block->next;
    }
}

SCON_API void scon_gc_retain(scon_t* scon, scon_handle_t handle)
{
    if (!SCON_HANDLE_IS_ITEM(&handle))
    {
        SCON_THROW(scon, "invalid item type");
    }

    scon_list_append(scon, &scon->retained, SCON_HANDLE_TO_ITEM(&handle));
}

SCON_API void scon_gc_release(scon_t* scon, scon_handle_t item)
{
    if (!SCON_HANDLE_IS_ITEM(&item))
    {
        SCON_THROW(scon, "invalid item type");
    }

    scon_list_remove_unstable(scon, &scon->retained, SCON_HANDLE_TO_ITEM(&item));
}

#endif
