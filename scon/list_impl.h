#ifndef SCON_LIST_IMPL_H
#define SCON_LIST_IMPL_H 1

#include "core.h"
#include "item.h"
#include "list.h"

SCON_API void scon_list_deinit(scon_t* scon, scon_list_t* list)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);

    if (list->capacity > SCON_LIST_SMALL_MAX)
    {
        SCON_FREE(list->handles);
    }
}

SCON_API scon_list_t* scon_list_new(scon_t* scon, scon_size_t capacity)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_item_t* item = scon_item_new(scon);
    item->type = SCON_ITEM_TYPE_LIST;
    scon_list_t* list = &item->list;
    list->length = 0;
    if (capacity < SCON_LIST_SMALL_MAX)
    {
        list->handles = list->small;
        list->capacity = SCON_LIST_SMALL_MAX;
        return list;
    }
    list->capacity = capacity;

    list->handles = (scon_handle_t*)SCON_MALLOC(capacity * sizeof(scon_handle_t));
    if (list->handles == SCON_NULL)
    {
        SCON_ERROR_INTERNAL(scon, "out of memory");
    }

    return list;
}

SCON_API void scon_list_append(scon_t* scon, scon_list_t* list, scon_handle_t handle)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);

    if (list->length >= list->capacity)
    {
        scon_uint32_t newCapacity = list->capacity * SCON_LIST_GROWTH_FACTOR;

        scon_handle_t* newHandles;
        if (list->capacity == SCON_LIST_SMALL_MAX)
        {
            newHandles = (scon_handle_t*)SCON_MALLOC(newCapacity * sizeof(scon_handle_t));
            if (newHandles == SCON_NULL)
            {
                SCON_ERROR_INTERNAL(scon, "out of memory");
            }
            SCON_MEMCPY(newHandles, list->handles, SCON_LIST_SMALL_MAX * sizeof(scon_handle_t));
        }
        else
        {
            newHandles = (scon_handle_t*)SCON_REALLOC(list->handles, newCapacity * sizeof(scon_handle_t));
            if (newHandles == SCON_NULL)
            {
                SCON_ERROR_INTERNAL(scon, "out of memory");
            }
        }

        list->handles = newHandles;
        list->capacity = newCapacity;
    }

    list->handles[list->length++] = handle;
}

SCON_API void scon_list_remove_unstable(scon_t* scon, scon_list_t* list, scon_handle_t handle)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(list != SCON_NULL);

    for (scon_uint32_t i = 0; i < list->length; i++)
    {
        if (list->handles[i] == handle)
        {
            list->handles[i] = list->handles[list->length - 1];
            list->length--;
            return;
        }
    }
}

#endif
