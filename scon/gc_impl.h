#include "item.h"
#ifndef SCON_GC_IMPL_H
#define SCON_GC_IMPL_H 1

#include "core.h"
#include "eval.h"
#include "gc.h"
#include "item.h"
#include "list.h"

static void scon_gc_mark(scon_t* scon, scon_item_t* item);

static void scon_gc_mark_node(scon_t* scon, scon_uint32_t shift, scon_list_node_t* node)
{
    if (node == SCON_NULL)
    {
        return;
    }

    scon_item_t* item = SCON_CONTAINER_OF(node, scon_item_t, node);
    if (item->flags & SCON_ITEM_FLAG_GC_MARK)
    {
        return;
    }
    item->flags |= SCON_ITEM_FLAG_GC_MARK;

    if (shift == 0)
    {
        for (int i = 0; i < SCON_LIST_WIDTH; i++)
        {
            scon_handle_t h = node->handles[i];
            if (SCON_HANDLE_IS_ITEM(&h))
            {
                scon_gc_mark(scon, SCON_HANDLE_TO_ITEM(&h));
            }
        }
    }
    else
    {
        for (int i = 0; i < SCON_LIST_WIDTH; i++)
        {
            scon_gc_mark_node(scon, shift - SCON_LIST_BITS, node->children[i]);
        }
    }
}

static void scon_gc_mark_list(scon_t* scon, scon_list_t* list)
{
    scon_gc_mark_node(scon, list->shift, list->root);
    scon_gc_mark_node(scon, 0, list->tail);
}

static void scon_gc_mark(scon_t* scon, scon_item_t* item)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (item == SCON_NULL || (item->flags & SCON_ITEM_FLAG_GC_MARK))
    {
        return;
    }

    item->flags |= SCON_ITEM_FLAG_GC_MARK;

    if (item->type == SCON_ITEM_TYPE_LIST)
    {
        scon_gc_mark_list(scon, &item->list);
    }
    else if (item->type == SCON_ITEM_TYPE_FUNCTION)
    {
        for (scon_uint16_t i = 0; i < item->function.constantCount; i++)
        {
            if (item->function.constants[i].type == SCON_CONST_SLOT_ITEM)
            {
                scon_gc_mark(scon, item->function.constants[i].item);
            }
            else if (item->function.constants[i].type == SCON_CONST_SLOT_CAPTURE)
            {
                scon_gc_mark(scon, SCON_CONTAINER_OF(item->function.constants[i].capture, scon_item_t, atom));
            }
        }
    }
    else if (item->type == SCON_ITEM_TYPE_CLOSURE)
    {
        scon_gc_mark(scon, SCON_CONTAINER_OF(item->closure.function, scon_item_t, function));
        for (scon_uint16_t i = 0; i < item->closure.function->constantCount; i++)
        {
            scon_handle_t handle = item->closure.constants[i];
            if (SCON_HANDLE_IS_ITEM(&handle))
            {
                scon_gc_mark(scon, SCON_HANDLE_TO_ITEM(&handle));
            }
        }
    }
}

SCON_API void scon_gc_if_needed(scon_t* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (scon->blocksAllocated >= scon->gcThreshold)
    {
        scon_gc(scon);
        scon->blocksAllocated = 0;
        scon->gcThreshold = scon->blocksAllocated + SCON_GC_THRESHOLD_INITIAL;
    }
}

SCON_API void scon_gc(scon_t* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_item_block_t* rootBlock = scon->block;
    while (rootBlock != SCON_NULL)
    {
        for (int i = 0; i < SCON_ITEM_BLOCK_MAX; i++)
        {
            scon_item_t* item = &rootBlock->items[i];
            if (item->type != SCON_ITEM_TYPE_NONE && item->retainCount > 0)
            {
                scon_gc_mark(scon, item);
            }
        }
        rootBlock = rootBlock->next;
    }

    if (scon->evalState != SCON_NULL)
    {
        for (scon_uint32_t i = 0; i < scon->evalState->regCount; i++)
        {
            scon_handle_t child = scon->evalState->regs[i];
            if (SCON_HANDLE_IS_ITEM(&child))
            {
                scon_gc_mark(scon, SCON_HANDLE_TO_ITEM(&child));
            }
        }
        for (scon_uint32_t i = 0; i < scon->evalState->frameCount; i++)
        {
            scon_closure_t* closure = scon->evalState->frames[i].closure;
            scon_gc_mark(scon, SCON_CONTAINER_OF(closure, scon_item_t, closure));
        }
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

#endif
