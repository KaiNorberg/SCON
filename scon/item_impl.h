#ifndef SCON_ITEM_IMPL_H
#define SCON_ITEM_IMPL_H 1

#include "core_api.h"
#include "item_api.h"
#include "item_internal.h"
#include "core_internal.h"

static inline _scon_node_t* _scon_node_new(scon_t* scon)
{
    _scon_node_t* item = SCON_NULL;
    if (scon->freeList != NULL)
    {
        item = scon->freeList;
        scon->freeList = item->free;
        return item;
    
    }
    
    _scon_node_block_t* block;
    if (scon->block == SCON_NULL)
    {
        block = &scon->firstBlock;
    }
    else
    {
        block = SCON_CALLOC(1, sizeof(_scon_node_block_t));
        if (block == SCON_NULL)
        {
            SCON_THROW(scon, "out of memory");
        }
        for (int i = 1; i < _SCON_ITEM_BLOCK_MAX - 1; i++)
        {
            block->items[i].free = &block->items[i + 1];
        }
    }
    block->items[_SCON_ITEM_BLOCK_MAX - 1].free = NULL;
    scon->freeList = &block->items[1];
    block->next = scon->block;
    scon->block = block;
    
    item = &block->items[0];
    item->type = SCON_ITEM_NONE;
    item->length = 0;
    item->flags = 0;
    return item;
}

static inline void _scon_node_free(scon_t* scon, _scon_node_t* item)
{
    if (item->type == SCON_ITEM_ATOM)
    {
        _scon_atom_deinit(scon, &item->atom);
    }
    else if (item->type == SCON_ITEM_LIST)
    {
        _scon_list_deinit(scon, &item->list);
    }

    item->type = SCON_ITEM_NONE;
    item->length = 0;
    item->flags = 0;
    item->free = scon->freeList;
    scon->freeList = item;
}

static inline void _scon_item_error_set(scon_t* scon, _scon_node_t* item, const char* message)
{
    scon_error_set(scon, message, scon->input->buffer, scon->input->length, item->position);
}

static inline void _scon_item_throw(scon_t* scon, _scon_node_t* item, const char* message)
{
    _scon_item_error_set(scon, item, message);
    SCON_LONGJMP(scon->jmp, SCON_FALSE);
}

static void _scon_item_ensure_node(scon_t* scon, scon_item_t* item)
{
    if (_SCON_ITEM_IS_NODE(item))
    {
        return;
    }

    if (_SCON_ITEM_IS_INT(item))
    {
        _scon_atom_t* atom = _scon_atom_lookup_int(scon, _SCON_ITEM_TO_INT(item));
        *item = _SCON_ITEM_FROM_NODE(SCON_CONTAINER_OF(atom, _scon_node_t, atom));
    }
    else if (_SCON_ITEM_IS_FLOAT(item))
    {
        _scon_atom_t* atom = _scon_atom_lookup_float(scon, _SCON_ITEM_TO_FLOAT(item));
        *item = _SCON_ITEM_FROM_NODE(SCON_CONTAINER_OF(atom, _scon_node_t, atom));
    }
    else
    {
        SCON_THROW(scon, "invalid item type");
    }
}

SCON_API scon_size_t scon_item_len(scon_t* scon, scon_item_t* item)
{
    _scon_item_ensure_node(scon, item);
    return _SCON_ITEM_TO_NODE(item)->length;
}

SCON_API scon_item_type_t scon_item_get_type(scon_t* scon, scon_item_t* item)
{
    if (!_SCON_ITEM_IS_NODE(item))
    {
        return SCON_ITEM_ATOM;
    }

    _scon_item_ensure_node(scon, item);
    return _SCON_ITEM_TO_NODE(item)->type;
}

SCON_API void scon_item_get_string(scon_t* scon, scon_item_t* item, char** out, scon_size_t* len)
{
    _scon_item_ensure_node(scon, item);
    _scon_node_t* node = _SCON_ITEM_TO_NODE(item);
    if (node->type != SCON_ITEM_ATOM)
    {
        SCON_THROW(scon, "invalid item type");
    }

    *out = _SCON_ATOM_STRING(&node->atom);
    *len = node->length;
}

SCON_API scon_int64_t scon_item_get_int(scon_t* scon, scon_item_t* item)
{
    if (_SCON_ITEM_IS_INT(item))
    {
        return _SCON_ITEM_TO_INT(item);
    }

    _scon_item_ensure_node(scon, item);
    _scon_node_t* node = _SCON_ITEM_TO_NODE(item);
    if (!(node->flags & _SCON_NODE_FLAG_INT_SHAPED))
    {
        SCON_THROW(scon, "invalid item type");
    }

    return node->atom.integerValue;
}

SCON_API scon_bool_t scon_item_is_number(scon_t* scon, scon_item_t* item)
{
    if (_SCON_ITEM_IS_INT(item) || _SCON_ITEM_IS_FLOAT(item))
    {
        return SCON_TRUE;
    }

    _scon_item_ensure_node(scon, item);
    _scon_node_t* node = _SCON_ITEM_TO_NODE(item);
    return (node->flags & (_SCON_NODE_FLAG_INT_SHAPED | _SCON_NODE_FLAG_FLOAT_SHAPED)) != 0;
}

SCON_API scon_bool_t scon_item_is_int(scon_t* scon, scon_item_t* item)
{
    if (_SCON_ITEM_IS_INT(item))
    {
        return SCON_TRUE;
    }

    _scon_item_ensure_node(scon, item);
    _scon_node_t* node = _SCON_ITEM_TO_NODE(item);
    return (node->flags & _SCON_NODE_FLAG_INT_SHAPED) != 0;
}

SCON_API scon_bool_t scon_item_is_float(scon_t* scon, scon_item_t* item)
{
    if (_SCON_ITEM_IS_FLOAT(item))
    {
        return SCON_TRUE;
    }

    _scon_item_ensure_node(scon, item);
    _scon_node_t* node = _SCON_ITEM_TO_NODE(item);
    return (node->flags & _SCON_NODE_FLAG_FLOAT_SHAPED) != 0;
}

SCON_API scon_bool_t scon_item_is_truthy(scon_t* scon, scon_item_t* item)
{
    if (_SCON_ITEM_IS_NODE(item))
    {
        _scon_node_t* node = _SCON_ITEM_TO_NODE(item);
        return !(node->flags & _SCON_NODE_FLAG_FALSY);
    }

    if (_SCON_ITEM_IS_INT(item))
    {
        return _SCON_ITEM_TO_INT(item) != 0;
    }

    if (_SCON_ITEM_IS_FLOAT(item))
    {
        return _SCON_ITEM_TO_FLOAT(item) != 0.0;
    }

    return SCON_FALSE;
}

typedef struct {
    int group;
    scon_bool_t isFloat;
    union {
        scon_int64_t i;
        scon_float_t f;
    } num;
    _scon_node_t* node;
} _scon_cmp_val_t;

static inline void _scon_item_unpack(scon_item_t* item, _scon_cmp_val_t* out)
{
    if (_SCON_ITEM_IS_INT(item))
    {
        out->group = 0;
        out->isFloat = SCON_FALSE;
        out->num.i = _SCON_ITEM_TO_INT(item);
        out->node = SCON_NULL;
        return;
    }
    
    if (_SCON_ITEM_IS_FLOAT(item))
    {
        out->group = 0;
        out->isFloat = SCON_TRUE;
        out->num.f = _SCON_ITEM_TO_FLOAT(item);
        out->node = SCON_NULL;
        return;
    }

    out->node = _SCON_ITEM_TO_NODE(item);
    if (out->node == SCON_NULL)
    {
        out->group = 1;
        return;
    }

    if (out->node->type == SCON_ITEM_LIST)
    {
        out->group = 2;
    }
    else if (out->node->flags & _SCON_NODE_FLAG_FLOAT_SHAPED)
    {
        out->group = 0;
        out->isFloat = SCON_TRUE;
        out->num.f = out->node->atom.floatValue;
    }
    else if (out->node->flags & _SCON_NODE_FLAG_INT_SHAPED)
    {
        out->group = 0;
        out->isFloat = SCON_FALSE;
        out->num.i = out->node->atom.integerValue;
    }
    else
    {
        out->group = 1;
    }
}

SCON_API scon_int64_t scon_item_compare(scon_t* scon, scon_item_t* a, scon_item_t* b)
{
    if (a == b || *a == *b)
    {
        return 0;
    }

    _scon_cmp_val_t va, vb;
    _scon_item_unpack(a, &va);
    _scon_item_unpack(b, &vb);

    if (va.group != vb.group)
    {
        return va.group - vb.group;
    }

    if (va.group == 0)
    {
        if (!va.isFloat && !vb.isFloat)
        {
            return (va.num.i < vb.num.i) ? -1 : ((va.num.i > vb.num.i) ? 1 : 0);
        }
        else if (va.isFloat && vb.isFloat)
        {
            return (va.num.f < vb.num.f) ? -1 : ((va.num.f > vb.num.f) ? 1 : 0);
        }
        
        scon_float_t fa = va.isFloat ? va.num.f : (scon_float_t)va.num.i;
        scon_float_t fb = vb.isFloat ? vb.num.f : (scon_float_t)vb.num.i;
        if (fa < fb)
        {
            return -1;
        }
        if (fa > fb)
        {
            return 1;   
        }
        return va.isFloat ? 1 : -1;
    }
    else if (va.group == 1)
    {
        _scon_atom_t* atomA = va.node ? &va.node->atom : SCON_NULL;
        _scon_atom_t* atomB = vb.node ? &vb.node->atom : SCON_NULL;
        scon_size_t lenA = atomA ? atomA->length : 0;
        scon_size_t lenB = atomB ? atomB->length : 0;
        scon_size_t minLen = lenA < lenB ? lenA : lenB;

        if (minLen > 0)
        {
            int cmp = memcmp(_SCON_ATOM_STRING(atomA), _SCON_ATOM_STRING(atomB), minLen);
            if (cmp != 0)
            {
                return cmp;
            }
        }

        return (scon_int64_t)lenA - (scon_int64_t)lenB;
    }
    
    _scon_list_t* listA = va.node ? &va.node->list : SCON_NULL;
    _scon_list_t* listB = vb.node ? &vb.node->list : SCON_NULL;
    scon_size_t lenA = listA ? listA->length : 0;
    scon_size_t lenB = listB ? listB->length : 0;
    scon_size_t minLen = lenA < lenB ? lenA : lenB;

    if (minLen > 0)
    {
        scon_item_t* itemsA = _SCON_LIST_ITEMS(listA);
        scon_item_t* itemsB = _SCON_LIST_ITEMS(listB);

        for (scon_size_t i = 0; i < minLen; i++)
        {
            scon_int64_t cmp = scon_item_compare(scon, &itemsA[i], &itemsB[i]);
            if (cmp != 0)
            {
                return cmp;
            }
        }
    }

    return (scon_int64_t)lenA - (scon_int64_t)lenB;
}

SCON_API scon_bool_t scon_item_is_equal(scon_t* scon, scon_item_t* a, scon_item_t* b)
{
    _scon_item_ensure_node(scon, a);
    _scon_item_ensure_node(scon, b);

    return *a == *b;
}

SCON_API scon_item_t scon_item_get(scon_t* scon, scon_item_t* list, const char* name)
{
    if (!_SCON_ITEM_IS_NODE(list))
    {
        return SCON_NONE;
    }
    
    _scon_node_t* node = _SCON_ITEM_TO_NODE(list);
    if (node->type != SCON_ITEM_LIST)
    {
        return SCON_NONE;
    }

    scon_size_t strLen = SCON_STRLEN(name);
    if (strLen == 0)
    {
        return SCON_NONE;
    }

    _scon_list_t* l = &node->list;
    scon_size_t len = l->length;
    scon_item_t* items = _SCON_LIST_ITEMS(l);
    for (scon_size_t i = 0; i < len; i++)
    {
        if (!_SCON_ITEM_IS_NODE(&items[i]))
        {
            continue;
        }

        _scon_node_t* itemNode = _SCON_ITEM_TO_NODE(&items[i]);
        if (itemNode->type != SCON_ITEM_LIST || itemNode->list.length < 2)
        {
            continue;
        }

        scon_item_t* pair = _SCON_LIST_ITEMS(&itemNode->list);
        if (!_SCON_ITEM_IS_NODE(&pair[0]))
        {
            continue;
        }

        _scon_node_t* keyNode = _SCON_ITEM_TO_NODE(&pair[0]);
        if (keyNode->type == SCON_ITEM_ATOM && keyNode->length == strLen)
        {
            if (memcmp(_SCON_ATOM_STRING(&keyNode->atom), name, strLen) == 0)
            {
                return pair[1];
            }
        }
    }
    
    return SCON_NONE;
}

SCON_API scon_item_t scon_item_nth(scon_t* scon, scon_item_t* list, scon_size_t n)
{
    if (!_SCON_ITEM_IS_NODE(list))
    {
        return SCON_NONE;
    }

    _scon_node_t* node = _SCON_ITEM_TO_NODE(list);
    if (node->type != SCON_ITEM_LIST)
    {
        return SCON_NONE;
    }

    _scon_list_t* l = &node->list;
    if (n >= l->length)
    {
        return SCON_NONE;
    }

    scon_item_t* items = _SCON_LIST_ITEMS(l);
    return items[n];
}

#endif
