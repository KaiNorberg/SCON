#ifndef SCON_STDLIB_ASSOC_IMPL_H
#define SCON_STDLIB_ASSOC_IMPL_H 1

#include "core.h"
#include "eval.h"
#include "handle.h"
#include "gc.h"
#include "stdlib_assoc.h"

SCON_API scon_handle_t scon_get(scon_t* scon, scon_handle_t* listHandle, scon_handle_t* keyHandle)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (!SCON_HANDLE_IS_LIST(listHandle))
    {
        return SCON_HANDLE_NONE;
    }

    scon_list_t* list = &SCON_HANDLE_TO_ITEM(listHandle)->list;

    for (scon_size_t i = 0; i < list->length; i++)
    {
        scon_handle_t childHandle = list->handles[i];
        if (!SCON_HANDLE_IS_LIST(&childHandle))
        {
            continue;
        }

        scon_item_t* childItem = SCON_HANDLE_TO_ITEM(&childHandle);
        if (childItem->list.length < 2)
        {
            continue;
        }

        scon_handle_t currentKey = childItem->list.handles[0];
        if (SCON_HANDLE_COMPARE_FAST(scon, &currentKey, keyHandle, ==))
        {
            return childItem->list.handles[1];
        }
    }

    return scon_handle_nil(scon);
}

SCON_API scon_handle_t scon_keys(scon_t* scon, scon_handle_t* listHandle)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (!SCON_HANDLE_IS_LIST(listHandle))
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "keys expects a list, got %s",
            scon_item_type_str(scon_handle_get_type(scon, listHandle)));
    }

    scon_item_t* item = SCON_HANDLE_TO_ITEM(listHandle);
    scon_list_t* list = &item->list;
    scon_list_t* keysList = scon_list_new(scon, list->length);
    scon_handle_t keysHandle = SCON_HANDLE_FROM_LIST(keysList);
    SCON_GC_RETAIN(scon, keysHandle);

    for (scon_size_t i = 0; i < list->length; i++)
    {
        scon_handle_t childHandle = list->handles[i];
        if (SCON_HANDLE_IS_LIST(&childHandle))
        {
            scon_item_t* childItem = SCON_HANDLE_TO_ITEM(&childHandle);
            if (childItem->list.length >= 1)
            {
                scon_list_append(scon, keysList, childItem->list.handles[0]);
            }
        }
    }

    SCON_GC_RELEASE(scon, keysHandle);
    return keysHandle;
}

SCON_API scon_handle_t scon_values(scon_t* scon, scon_handle_t* listHandle)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (!SCON_HANDLE_IS_LIST(listHandle))
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "values expects a list, got %s",
            scon_item_type_str(scon_handle_get_type(scon, listHandle)));
    }

    scon_item_t* item = SCON_HANDLE_TO_ITEM(listHandle);
    scon_list_t* list = &item->list;
    scon_list_t* valuesList = scon_list_new(scon, list->length);
    scon_handle_t valuesHandle = SCON_HANDLE_FROM_LIST(valuesList);
    SCON_GC_RETAIN(scon, valuesHandle);

    for (scon_size_t i = 0; i < list->length; i++)
    {
        scon_handle_t childHandle = list->handles[i];
        if (SCON_HANDLE_IS_LIST(&childHandle))
        {
            scon_item_t* childItem = SCON_HANDLE_TO_ITEM(&childHandle);
            if (childItem->list.length >= 2)
            {
                scon_list_append(scon, valuesList, childItem->list.handles[1]);
            }
        }
    }

    SCON_GC_RELEASE(scon, valuesHandle);
    return valuesHandle;
}

SCON_API scon_handle_t scon_assoc(scon_t* scon, scon_handle_t* listHandle, scon_handle_t* keyHandle, scon_handle_t* valHandle)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (!SCON_HANDLE_IS_LIST(listHandle))
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "assoc expects a list as the first argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, listHandle)));
    }

    scon_item_t* oldItem = SCON_HANDLE_TO_ITEM(listHandle);
    scon_list_t* newList = scon_list_new(scon, oldItem->length);
    scon_handle_t newHandle = SCON_HANDLE_FROM_LIST(newList);
    SCON_GC_RETAIN(scon, newHandle);

    scon_bool_t found = SCON_FALSE;
    for (scon_size_t i = 0; i < oldItem->length; i++)
    {
        scon_handle_t childHandle = oldItem->list.handles[i];
        if (!found && SCON_HANDLE_IS_LIST(&childHandle))
        {
            scon_item_t* childItem = SCON_HANDLE_TO_ITEM(&childHandle);
            if (childItem->list.length >= 1 && SCON_HANDLE_COMPARE_FAST(scon, &childItem->list.handles[0], keyHandle, ==))
            {
                scon_list_t* pair = scon_list_new(scon, 2);
                scon_list_append(scon, pair, *keyHandle);
                scon_list_append(scon, pair, *valHandle);
                scon_list_append(scon, newList, SCON_HANDLE_FROM_LIST(pair));
                found = SCON_TRUE;
                continue;
            }
        }
        scon_list_append(scon, newList, childHandle);
    }

    if (!found)
    {
        scon_list_t* pair = scon_list_new(scon, 2);
        scon_list_append(scon, pair, *keyHandle);
        scon_list_append(scon, pair, *valHandle);
        scon_list_append(scon, newList, SCON_HANDLE_FROM_LIST(pair));
    }

    SCON_GC_RELEASE(scon, newHandle);
    return newHandle;
}

SCON_API scon_handle_t scon_dissoc(scon_t* scon, scon_handle_t* listHandle, scon_handle_t* keyHandle)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (!SCON_HANDLE_IS_LIST(listHandle))
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "dissoc expects a list as the first argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, listHandle)));
    }

    scon_item_t* oldItem = SCON_HANDLE_TO_ITEM(listHandle);
    scon_list_t* newList = scon_list_new(scon, 0);
    scon_handle_t newHandle = SCON_HANDLE_FROM_LIST(newList);
    SCON_GC_RETAIN(scon, newHandle);

    for (scon_size_t i = 0; i < oldItem->length; i++)
    {
        scon_handle_t childHandle = oldItem->list.handles[i];
        if (SCON_HANDLE_IS_LIST(&childHandle))
        {
            scon_item_t* childItem = SCON_HANDLE_TO_ITEM(&childHandle);
            if (childItem->list.length >= 1 && SCON_HANDLE_COMPARE_FAST(scon, &childItem->list.handles[0], keyHandle, ==))
            {
                continue;
            }
        }
        scon_list_append(scon, newList, childHandle);
    }

    SCON_GC_RELEASE(scon, newHandle);
    return newHandle;
}

SCON_API scon_handle_t scon_update(scon_t* scon, scon_handle_t* listHandle, scon_handle_t* keyHandle, scon_handle_t* callable)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (!SCON_HANDLE_IS_LIST(listHandle))
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "update expects a list as the first argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, listHandle)));
    }

    scon_item_t* oldItem = SCON_HANDLE_TO_ITEM(listHandle);
    scon_list_t* newList = scon_list_new(scon, oldItem->length);
    scon_handle_t newHandle = SCON_HANDLE_FROM_LIST(newList);
    SCON_GC_RETAIN(scon, newHandle);

    scon_bool_t found = SCON_FALSE;
    for (scon_size_t i = 0; i < oldItem->length; i++)
    {
        scon_handle_t childHandle = oldItem->list.handles[i];
        if (!found && SCON_HANDLE_IS_LIST(&childHandle))
        {
            scon_item_t* childItem = SCON_HANDLE_TO_ITEM(&childHandle);
            if (childItem->list.length >= 2 && SCON_HANDLE_COMPARE_FAST(scon, &childItem->list.handles[0], keyHandle, ==))
            {
                scon_handle_t currentVal = childItem->list.handles[1];
                scon_handle_t newVal = scon_eval_call(scon, *callable, 1, &currentVal);
                scon_list_t* pair = scon_list_new(scon, 2);
                scon_list_append(scon, pair, *keyHandle);
                scon_list_append(scon, pair, newVal);
                scon_list_append(scon, newList, SCON_HANDLE_FROM_LIST(pair));
                found = SCON_TRUE;
                continue;
            }
        }
        scon_list_append(scon, newList, childHandle);
    }

    SCON_GC_RELEASE(scon, newHandle);
    return newHandle;
}

#endif