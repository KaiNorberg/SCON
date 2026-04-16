#ifndef SCON_LIST_IMPL_H
#define SCON_LIST_IMPL_H 1

#include "core_internal.h"
#include "item_internal.h"
#include "list_internal.h"

static inline void _scon_list_init(scon_t* scon, _scon_list_t* list)
{
    list->length = 0;
    list->capacity = _SCON_LIST_SHORT_MAX;
}

static inline void _scon_list_deinit(scon_t* scon, _scon_list_t* list)
{
    if (list->capacity > _SCON_LIST_SHORT_MAX && list->longItems != SCON_NULL)
    {
        SCON_FREE(list->longItems);
    }
}

static inline _scon_list_t* _scon_list_new(scon_t* scon, scon_size_t capacity)
{
    _scon_node_t* node = _scon_node_new(scon);
    node->type = SCON_ITEM_LIST;
    _scon_list_t* list = &node->list;
    list->length = 0;
    if (capacity < _SCON_LIST_SHORT_MAX)
    {
        list->capacity = _SCON_LIST_SHORT_MAX;
        return list;
    }
    list->capacity = capacity;

    list->longItems = SCON_MALLOC(capacity * sizeof(scon_item_t));
    if (list->longItems == SCON_NULL)
    {
        SCON_THROW(scon, "out of memory");
    }
    
    return list;
}

static inline void _scon_list_push_back(scon_t* scon, _scon_list_t* list, scon_item_t item)
{
    if (list->capacity <= _SCON_LIST_SHORT_MAX && list->length < _SCON_LIST_SHORT_MAX)
    {        
        list->shortItems[list->length++] = item;
        return;
    }

    if (list->length >= list->capacity)
    {
        scon_uint32_t newCapacity = list->capacity * _SCON_LIST_GROWTH_FACTOR;

        scon_item_t* newItems;
        if (list->capacity == _SCON_LIST_SHORT_MAX)
        {
            newItems = SCON_MALLOC(newCapacity * sizeof(scon_item_t));
            if (newItems == SCON_NULL)
            {
                SCON_THROW(scon, "out of memory");
            }
            SCON_MEMCPY(newItems, list->shortItems, _SCON_LIST_SHORT_MAX * sizeof(scon_item_t));
        }
        else
        {
            newItems = SCON_REALLOC(list->longItems, newCapacity * sizeof(scon_item_t));
            if (newItems == SCON_NULL)
            {
                SCON_THROW(scon, "out of memory");
            }
        }

        list->longItems = newItems;
        list->capacity = newCapacity;
    }

    list->longItems[list->length++] = item;
}

static inline void _scon_list_remove_unstable(scon_t* scon, _scon_list_t* list, scon_item_t item)
{
    scon_item_t* items = _SCON_LIST_ITEMS(list);
    for (scon_uint32_t i = 0; i < list->length; i++)
    {
        if (items[i] == item)
        {
            items[i] = items[list->length - 1];
            list->length--;
            return;
        }
    }
}

#endif

