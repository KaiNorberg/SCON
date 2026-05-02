#ifndef REDUCT_LIST_IMPL_H
#define REDUCT_LIST_IMPL_H 1

#include "core.h"
#include "handle.h"
#include "item.h"
#include "list.h"

static reduct_list_node_t* reduct_list_node_new(struct reduct* reduct)
{
    reduct_item_t* item = reduct_item_new(reduct);
    item->type = REDUCT_ITEM_TYPE_LIST_NODE;
    return &item->node;
}

static reduct_list_node_t* reduct_list_node_copy(reduct_t* reduct, reduct_list_node_t* node)
{
    reduct_item_t* item = reduct_item_new(reduct);
    item->type = REDUCT_ITEM_TYPE_LIST_NODE;
    reduct_list_node_t* newNode = &item->node;
    REDUCT_MEMCPY(newNode, node, sizeof(reduct_list_node_t));
    return newNode;
}

static inline void reduct_list_node_init(reduct_list_node_t* node)
{
    for (reduct_uint32_t i = 0; i < REDUCT_LIST_WIDTH; i++)
    {
        node->children[i] = REDUCT_NULL;
    }
}

REDUCT_API reduct_list_t* reduct_list_new(reduct_t* reduct)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);

    reduct_item_t* item = reduct_item_new(reduct);
    item->type = REDUCT_ITEM_TYPE_LIST;
    reduct_list_t* list = &item->list;
    list->length = 0;
    list->shift = 0;
    list->root = REDUCT_NULL;
    reduct_list_node_init(&list->tail);
    return list;
}

static reduct_list_node_t* reduct_list_find_leaf(reduct_list_t* list, reduct_size_t index, reduct_size_t tailOffset)
{
    if (index >= tailOffset || list->root == REDUCT_NULL)
    {
        return &list->tail;
    }

    reduct_list_node_t* node = list->root;
    for (reduct_uint32_t level = list->shift; level > 0; level -= REDUCT_LIST_BITS)
    {
        node = node->children[(index >> level) & REDUCT_LIST_MASK];
    }
    return node;
}

static reduct_list_node_t* reduct_list_assoc_internal(reduct_t* reduct, reduct_uint32_t shift, reduct_list_node_t* node,
    reduct_size_t index, reduct_handle_t val)
{
    reduct_list_node_t* newNode = reduct_list_node_copy(reduct, node);
    if (shift == 0)
    {
        newNode->handles[index & REDUCT_LIST_MASK] = val;
    }
    else
    {
        reduct_uint32_t subIdx = (index >> shift) & REDUCT_LIST_MASK;
        newNode->children[subIdx] =
            reduct_list_assoc_internal(reduct, shift - REDUCT_LIST_BITS, newNode->children[subIdx], index, val);
    }
    return newNode;
}

REDUCT_API reduct_list_t* reduct_list_assoc(struct reduct* reduct, reduct_list_t* list, reduct_size_t index,
    reduct_handle_t val)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(list != REDUCT_NULL);

    if (REDUCT_UNLIKELY(index >= list->length))
    {
        REDUCT_ERROR_RUNTIME(reduct, "index %zu out of bounds for list of length %u", index);
    }

    reduct_list_t* newList = reduct_list_new(reduct);
    newList->length = list->length;
    newList->shift = list->shift;

    reduct_size_t tailOffset = REDUCT_LIST_TAIL_OFFSET(list);

    if (index >= tailOffset)
    {
        newList->root = list->root;
        newList->tail = list->tail;
        newList->tail.handles[index & REDUCT_LIST_MASK] = val;
    }
    else
    {
        newList->root = reduct_list_assoc_internal(reduct, list->shift, list->root, index, val);
        newList->tail = list->tail;
    }

    return newList;
}

REDUCT_API reduct_list_t* reduct_list_dissoc(struct reduct* reduct, reduct_list_t* list, reduct_size_t index)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(list != REDUCT_NULL);

    if (REDUCT_UNLIKELY(index >= list->length))
    {
        return list;
    }

    /// @todo There is definetly a better way to do this

    reduct_list_t* newList = reduct_list_new(reduct);
    reduct_handle_t val;
    REDUCT_LIST_FOR_EACH(&val, list)
    {
        if (_iter.index - 1 != index)
        {
            reduct_list_push(reduct, newList, val);
        }
    }

    return newList;
}

REDUCT_API reduct_list_t* reduct_list_slice(struct reduct* reduct, reduct_list_t* list, reduct_size_t start,
    reduct_size_t end)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(list != REDUCT_NULL);

    if (REDUCT_UNLIKELY(start > end || end > list->length))
    {
        REDUCT_ERROR_RUNTIME(reduct, "slice: invalid range [%zu, %zu) for list of length %u", start, end, list->length);
    }

    reduct_list_t* newList = reduct_list_new(reduct);
    reduct_list_iter_t iter = REDUCT_LIST_ITER_AT(list, start);

    reduct_handle_t val;
    while (iter.index < end)
    {
        if (REDUCT_LIKELY(reduct_list_iter_next(&iter, &val)))
        {
            reduct_list_push(reduct, newList, val);
        }
    }

    return newList;
}

REDUCT_API reduct_list_t* reduct_list_append(struct reduct* reduct, reduct_list_t* list, reduct_handle_t val)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(list != REDUCT_NULL);

    reduct_list_t* newList = reduct_list_new(reduct);
    newList->length = list->length;
    newList->shift = list->shift;
    newList->root = list->root;
    newList->tail = list->tail;

    reduct_list_push(reduct, newList, val);
    return newList;
}

REDUCT_API reduct_list_t* reduct_list_prepend(struct reduct* reduct, reduct_list_t* list, reduct_handle_t val)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(list != REDUCT_NULL);

    reduct_list_t* newList = reduct_list_new(reduct);
    reduct_list_push(reduct, newList, val);
    reduct_list_push_list(reduct, newList, list);

    return newList;
}

REDUCT_API reduct_handle_t reduct_list_nth(struct reduct* reduct, reduct_list_t* list, reduct_size_t index)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(list != REDUCT_NULL);

    if (REDUCT_UNLIKELY(index >= list->length))
    {
        REDUCT_ERROR_RUNTIME(reduct, "index %zu out of bounds for list of length %u", index);
    }

    reduct_size_t tailOffset = REDUCT_LIST_TAIL_OFFSET(list);
    reduct_list_node_t* node = reduct_list_find_leaf(list, index, tailOffset);
    return node->handles[index & REDUCT_LIST_MASK];
}

REDUCT_API struct reduct_item* reduct_list_nth_item(struct reduct* reduct, reduct_list_t* list, reduct_size_t index)
{
    reduct_handle_t handle = reduct_list_nth(reduct, list, index);
    reduct_handle_ensure_item(reduct, &handle);
    return REDUCT_HANDLE_TO_ITEM(&handle);
}

static reduct_list_node_t* reduct_push_tail(reduct_t* reduct, reduct_uint32_t shift, reduct_size_t index,
    reduct_list_node_t* parent, reduct_list_node_t* tailNode)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(tailNode != REDUCT_NULL);

    if (shift == 0)
    {
        return tailNode;
    }

    reduct_list_node_t* newNode =
        parent != REDUCT_NULL ? reduct_list_node_copy(reduct, parent) : reduct_list_node_new(reduct);
    if (parent == REDUCT_NULL)
    {
        reduct_list_node_init(newNode);
    }
    reduct_uint32_t subIdx = (index >> shift) & REDUCT_LIST_MASK;
    newNode->children[subIdx] =
        reduct_push_tail(reduct, shift - REDUCT_LIST_BITS, index, newNode->children[subIdx], tailNode);
    return newNode;
}

REDUCT_API void reduct_list_push(reduct_t* reduct, reduct_list_t* list, reduct_handle_t val)
{
    REDUCT_ASSERT(list != REDUCT_NULL);

    if (list->length > 0 && (list->length & REDUCT_LIST_MASK) == 0)
    {
        reduct_list_node_t* fullTailNode = reduct_list_node_new(reduct);
        REDUCT_MEMCPY(fullTailNode, &list->tail, sizeof(reduct_list_node_t));

        reduct_list_node_init(&list->tail);
        list->tail.handles[0] = val;

        if (list->root == REDUCT_NULL)
        {
            list->root = fullTailNode;
            list->shift = 0;
        }
        else
        {
            if ((list->length - 1) >> (list->shift + REDUCT_LIST_BITS) > 0)
            {
                reduct_list_node_t* newRoot = reduct_list_node_new(reduct);
                reduct_list_node_init(newRoot);
                newRoot->children[0] = list->root;
                newRoot->children[1] = reduct_push_tail(reduct, list->shift, list->length - 1, REDUCT_NULL, fullTailNode);
                list->root = newRoot;
                list->shift += REDUCT_LIST_BITS;
            }
            else
            {
                list->root = reduct_push_tail(reduct, list->shift, list->length - 1, list->root, fullTailNode);
            }
        }
    }
    else
    {
        list->tail.handles[list->length & REDUCT_LIST_MASK] = val;
    }

    list->length++;
}

REDUCT_API void reduct_list_push_list(reduct_t* reduct, reduct_list_t* list, reduct_list_t* other)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(list != REDUCT_NULL);
    REDUCT_ASSERT(other != REDUCT_NULL);

    reduct_handle_t val;
    REDUCT_LIST_FOR_EACH(&val, other)
    {
        reduct_list_push(reduct, list, val);
    }
}

REDUCT_API reduct_bool_t reduct_list_iter_next(reduct_list_iter_t* iter, reduct_handle_t* out)
{
    if (REDUCT_UNLIKELY(iter->index >= iter->list->length))
    {
        return REDUCT_FALSE;
    }

    if (iter->leaf == REDUCT_NULL || (iter->index & REDUCT_LIST_MASK) == 0)
    {
        iter->leaf = reduct_list_find_leaf(iter->list, iter->index, iter->tailOffset);
    }

    if (out != REDUCT_NULL)
    {
        *out = iter->leaf->handles[iter->index & REDUCT_LIST_MASK];
    }

    iter->index++;
    return REDUCT_TRUE;
}

#endif
