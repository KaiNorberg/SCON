#ifndef SCON_ITEM_IMPL_H
#define SCON_ITEM_IMPL_H 1

#include "core.h"
#include "item.h"

SCON_API scon_item_t* scon_item_new(scon_t* scon)
{
    scon_item_t* item = SCON_NULL;
    if (scon->freeList != NULL)
    {
        item = scon->freeList;
        scon->freeList = item->free;
        item->type = SCON_ITEM_TYPE_NONE;
        item->flags = 0;
        item->length = 0;
        return item;
    }

    scon_item_block_t* block;
    if (scon->block == SCON_NULL)
    {
        block = &scon->firstBlock;
    }
    else
    {
        block = SCON_CALLOC(1, sizeof(scon_item_block_t));
        if (block == SCON_NULL)
        {
            SCON_THROW(scon, "out of memory");
        }
        scon->blocksAllocated++;
    }

    for (scon_size_t i = 1; i < SCON_ITEM_BLOCK_MAX - 1; i++)
    {
        block->items[i].free = &block->items[i + 1];
    }

    block->items[SCON_ITEM_BLOCK_MAX - 1].free = NULL;
    scon->freeList = &block->items[1];
    block->next = scon->block;
    scon->block = block;

    item = &block->items[0];
    item->type = SCON_ITEM_TYPE_NONE;
    item->length = 0;
    item->flags = 0;
    return item;
}

SCON_API void scon_item_free(scon_t* scon, scon_item_t* item)
{
    if (item->type == SCON_ITEM_TYPE_ATOM)
    {
        scon_atom_deinit(scon, &item->atom);
    }
    else if (item->type == SCON_ITEM_TYPE_LIST)
    {
        scon_list_deinit(scon, &item->list);
    }

    item->type = SCON_ITEM_TYPE_NONE;
    item->length = 0;
    item->flags = 0;
    item->free = scon->freeList;
    scon->freeList = item;
}

#endif
