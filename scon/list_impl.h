#ifndef SCON_LIST_IMPL_H
#define SCON_LIST_IMPL_H 1

#include "core.h"
#include "item.h"
#include "list.h"

SCON_API void scon_list_deinit(scon_t* scon, scon_list_t* list)
{
    if (list->capacity > SCON_LIST_SMALL_MAX)
    {
        SCON_FREE(list->items);
    }
}

SCON_API scon_list_t* scon_list_new(scon_t* scon, scon_size_t capacity)
{
    scon_item_t* item = scon_item_new(scon);
    item->type = SCON_ITEM_TYPE_LIST;
    scon_list_t* list = &item->list;
    list->length = 0;
    if (capacity < SCON_LIST_SMALL_MAX)
    {
        list->items = list->small;
        list->capacity = SCON_LIST_SMALL_MAX;
        return list;
    }
    list->capacity = capacity;

    list->items = SCON_MALLOC(capacity * sizeof(scon_item_t*));
    if (list->items == SCON_NULL)
    {
        SCON_THROW(scon, "out of memory");
    }

    return list;
}

SCON_API void scon_list_append(scon_t* scon, scon_list_t* list, scon_item_t* item)
{
    if (list->length >= list->capacity)
    {
        scon_uint32_t newCapacity = list->capacity * SCON_LIST_GROWTH_FACTOR;

        scon_item_t** newItems;
        if (list->capacity == SCON_LIST_SMALL_MAX)
        {
            newItems = SCON_MALLOC(newCapacity * sizeof(scon_item_t*));
            if (newItems == SCON_NULL)
            {
                SCON_THROW(scon, "out of memory");
            }
            SCON_MEMCPY(newItems, list->items, SCON_LIST_SMALL_MAX * sizeof(scon_item_t*));
        }
        else
        {
            newItems = SCON_REALLOC(list->items, newCapacity * sizeof(scon_item_t*));
            if (newItems == SCON_NULL)
            {
                SCON_THROW(scon, "out of memory");
            }
        }

        list->items = newItems;
        list->capacity = newCapacity;
    }

    list->items[list->length++] = item;
}

SCON_API void scon_list_remove_unstable(scon_t* scon, scon_list_t* list, scon_item_t* item)
{
    for (scon_uint32_t i = 0; i < list->length; i++)
    {
        if (list->items[i] == item)
        {
            list->items[i] = list->items[list->length - 1];
            list->length--;
            return;
        }
    }
}

#endif
