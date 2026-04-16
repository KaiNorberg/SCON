#include "item_api.h"
#ifndef SCON_GC_IMPL_H
#define SCON_GC_IMPL_H 1

#include "gc_internal.h"
#include "core_internal.h"
#include "item_internal.h"
#include "list_internal.h"

static void scon_gc_mark(scon_t* scon, _scon_node_t* node)
{
    if (node->flags & _SCON_NODE_FLAG_GC_MARK)
    {
        return;
    }

    node->flags |= _SCON_NODE_FLAG_GC_MARK;
}

static void scon_gc_mark_list(scon_t* scon, _scon_list_t* list)
{
    scon_item_t* items = _SCON_LIST_ITEMS(list);
    for (scon_uint32_t i = 0; i < list->length; i++)
    {
        scon_item_t item = items[i];
        if (!_SCON_ITEM_IS_NODE(&item))
        {
            continue;
        }

        _scon_node_t* node = _SCON_ITEM_TO_NODE(&item);
        scon_gc_mark(scon, node);
    }
}       

SCON_API void scon_gc_if_needed(scon_t* scon)
{
    if (scon->blocksAllocated >= scon->gcThreshold)
    {
        scon_gc(scon);
        scon->blocksAllocated = 0;
        scon->gcThreshold = scon->blocksAllocated + _SCON_GC_THRESHOLD_INITIAL;
    }
}

SCON_API void scon_gc(scon_t* scon)
{
    _scon_list_t* retained = &scon->retained;
    for (scon_uint32_t i = 0; i < retained->length; i++)
    {
        scon_item_t* items = _SCON_LIST_ITEMS(retained);
        scon_item_t item = items[i];
        if (!_SCON_ITEM_IS_NODE(&item))
        {
            continue;
        }

        _scon_node_t* node = _SCON_ITEM_TO_NODE(&item);
        scon_gc_mark(scon, node);
    }

    _scon_node_block_t* block = scon->block;
    while (block != SCON_NULL)
    {
        for (int i = 0; i < _SCON_ITEM_BLOCK_MAX; i++)
        {
            _scon_node_t* node = &block->items[i];
            if (node->flags & _SCON_NODE_FLAG_GC_MARK)
            {
                node->flags &= ~_SCON_NODE_FLAG_GC_MARK;
            }
            else
            {
                _scon_node_free(scon, node);
            }
        }
        block = block->next;
    }

    scon->block = SCON_NULL;
    scon->freeList = SCON_NULL;
}

SCON_API void scon_gc_retain(scon_t* scon, scon_item_t item)
{
    if (!_SCON_ITEM_IS_NODE(&item))
    {
        SCON_THROW(scon, "invalid item type");
    }

    _scon_list_push_back(scon, &scon->retained, item);
}

SCON_API void scon_gc_release(scon_t* scon, scon_item_t item)
{
    _scon_list_remove_unstable(scon, &scon->retained, item);
}

#endif