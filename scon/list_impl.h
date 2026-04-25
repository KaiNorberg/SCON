#ifndef SCON_LIST_IMPL_H
#define SCON_LIST_IMPL_H 1

#include "core.h"
#include "handle.h"
#include "item.h"
#include "list.h"

static inline scon_list_node_t* scon_list_node_new(struct scon* scon)
{
    scon_item_t* item = scon_item_new(scon);
    item->type = SCON_ITEM_TYPE_LIST_NODE;
    return &item->node;
}

static scon_list_node_t* scon_list_node_copy(scon_t* scon, scon_list_node_t* node)
{
    scon_list_node_t* newNode = scon_list_node_new(scon);
    SCON_MEMCPY(newNode, node, sizeof(scon_list_node_t));
    return newNode;
}

SCON_API scon_list_t* scon_list_new(scon_t* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_item_t* item = scon_item_new(scon);
    item->type = SCON_ITEM_TYPE_LIST;
    scon_list_t* list = &item->list;
    list->length = 0;
    list->shift = 0;
    list->root = SCON_NULL;
    list->tail = SCON_NULL;
    return list;
}

static scon_list_node_t* scon_list_find_leaf(scon_list_t* list, scon_size_t index, scon_size_t tailOffset)
{
    if (index >= tailOffset || list->root == SCON_NULL)
    {
        return list->tail;
    }

    scon_list_node_t* node = list->root;
    for (scon_uint32_t level = list->shift; level > 0; level -= SCON_LIST_BITS)
    {
        node = node->children[(index >> level) & SCON_LIST_MASK];
    }
    return node;
}

static scon_list_node_t* scon_list_assoc_internal(scon_t* scon, scon_uint32_t shift, scon_list_node_t* node,
    scon_size_t index, scon_handle_t val)
{
    scon_list_node_t* newNode = scon_list_node_copy(scon, node);
    if (shift == 0)
    {
        newNode->handles[index & SCON_LIST_MASK] = val;
    }
    else
    {
        scon_uint32_t subIdx = (index >> shift) & SCON_LIST_MASK;
        newNode->children[subIdx] =
            scon_list_assoc_internal(scon, shift - SCON_LIST_BITS, newNode->children[subIdx], index, val);
    }
    return newNode;
}

SCON_API scon_list_t* scon_list_assoc(struct scon* scon, scon_list_t* list, scon_size_t index, scon_handle_t val)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);

    if (SCON_UNLIKELY(index >= list->length))
    {
        SCON_ERROR_RUNTIME(scon, "index %zu out of bounds", index);
    }

    scon_list_t* newList = scon_list_new(scon);
    newList->length = list->length;
    newList->shift = list->shift;

    scon_size_t tailOffset = SCON_LIST_TAIL_OFFSET(list);

    if (index >= tailOffset)
    {
        newList->root = list->root;
        newList->tail = scon_list_node_copy(scon, list->tail);
        newList->tail->handles[index & SCON_LIST_MASK] = val;
    }
    else
    {
        newList->root = scon_list_assoc_internal(scon, list->shift, list->root, index, val);
        newList->tail = list->tail;
    }

    return newList;
}

SCON_API scon_list_t* scon_list_dissoc(struct scon* scon, scon_list_t* list, scon_size_t index)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);

    if (SCON_UNLIKELY(index >= list->length))
    {
        return list;
    }

    /// @todo There is definetly a better way to do this

    scon_list_t* newList = scon_list_new(scon);
    scon_list_iter_t iter = SCON_LIST_ITER_AT(list, 0);

    scon_handle_t val;
    scon_size_t i = 0;
    while (scon_list_iter_next(&iter, &val))
    {
        if (i != index)
        {
            scon_list_append(scon, newList, val);
        }
        i++;
    }

    return newList;
}

SCON_API scon_list_t* scon_list_slice(struct scon* scon, scon_list_t* list, scon_size_t start, scon_size_t end)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);

    if (SCON_UNLIKELY(start > end || end > list->length))
    {
        SCON_ERROR_RUNTIME(scon, "invalid slice range [%zu, %zu) for list of length %u", start, end, list->length);
    }

    scon_list_t* newList = scon_list_new(scon);
    scon_size_t count = end - start;
    if (count == 0)
    {
        return newList;
    }

    scon_list_iter_t iter = SCON_LIST_ITER_AT(list, start);

    scon_handle_t val;
    for (scon_size_t i = 0; i < count; i++)
    {
        if (SCON_LIKELY(scon_list_iter_next(&iter, &val)))
        {
            scon_list_append(scon, newList, val);
        }
    }

    return newList;
}

SCON_API scon_handle_t scon_list_nth(struct scon* scon, scon_list_t* list, scon_size_t index)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);

    if (SCON_UNLIKELY(index >= list->length))
    {
        SCON_ERROR_RUNTIME(scon, "index %zu out of bounds", index);
    }

    scon_size_t tailOffset = SCON_LIST_TAIL_OFFSET(list);
    scon_list_node_t* node = scon_list_find_leaf(list, index, tailOffset);
    return node->handles[index & SCON_LIST_MASK];
}

SCON_API struct scon_item* scon_list_nth_item(struct scon* scon, scon_list_t* list, scon_size_t index)
{
    scon_handle_t handle = scon_list_nth(scon, list, index);
    scon_handle_ensure_item(scon, &handle);
    return SCON_HANDLE_TO_ITEM(&handle);
}

static scon_list_node_t* scon_push_tail(scon_t* scon, scon_uint32_t shift, scon_size_t index, scon_list_node_t* parent,
    scon_list_node_t* tailNode)
{
    if (shift == 0)
    {
        return tailNode;
    }

    scon_list_node_t* newNode = parent ? scon_list_node_copy(scon, parent) : scon_list_node_new(scon);
    scon_uint32_t subIdx = (index >> shift) & SCON_LIST_MASK;
    newNode->children[subIdx] =
        scon_push_tail(scon, shift - SCON_LIST_BITS, index, newNode->children[subIdx], tailNode);
    return newNode;
}

SCON_API void scon_list_append(scon_t* scon, scon_list_t* list, scon_handle_t val)
{
    SCON_ASSERT(list != SCON_NULL);

    if (list->length > 0 && (list->length & SCON_LIST_MASK) == 0)
    {
        scon_list_node_t* fullTail = list->tail;
        list->tail = scon_list_node_new(scon);
        list->tail->handles[0] = val;

        if (list->root == SCON_NULL)
        {
            list->root = fullTail;
            list->shift = 0;
        }
        else
        {
            if ((list->length - 1) >> (list->shift + SCON_LIST_BITS) > 0)
            {
                scon_list_node_t* newRoot = scon_list_node_new(scon);
                newRoot->children[0] = list->root;
                newRoot->children[1] = scon_push_tail(scon, list->shift, list->length - 1, SCON_NULL, fullTail);
                list->root = newRoot;
                list->shift += SCON_LIST_BITS;
            }
            else
            {
                list->root = scon_push_tail(scon, list->shift, list->length - 1, list->root, fullTail);
            }
        }
    }
    else
    {
        if (list->tail == SCON_NULL)
        {
            list->tail = scon_list_node_new(scon);
        }

        list->tail->handles[list->length & SCON_LIST_MASK] = val;
    }

    list->length++;
}

SCON_API void scon_list_append_list(scon_t* scon, scon_list_t* list, scon_list_t* other)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);
    SCON_ASSERT(other != SCON_NULL);

    scon_list_iter_t iter = SCON_LIST_ITER(other);
    scon_handle_t val;
    while (scon_list_iter_next(&iter, &val))
    {
        scon_list_append(scon, list, val);
    }
}

SCON_API scon_bool_t scon_list_iter_next(scon_list_iter_t* iter, scon_handle_t* out)
{
    if (SCON_UNLIKELY(iter->index >= iter->list->length))
    {
        return SCON_FALSE;
    }

    if (iter->leaf == SCON_NULL || (iter->index & SCON_LIST_MASK) == 0)
    {
        iter->leaf = scon_list_find_leaf(iter->list, iter->index, iter->tailOffset);
    }

    if (out != SCON_NULL)
    {
        *out = iter->leaf->handles[iter->index & SCON_LIST_MASK];
    }

    iter->index++;
    return SCON_TRUE;
}

#endif
