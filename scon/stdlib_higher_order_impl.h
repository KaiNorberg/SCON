#ifndef SCON_STDLIB_HIGHER_ORDER_IMPL_H
#define SCON_STDLIB_HIGHER_ORDER_IMPL_H 1

#include "core.h"
#include "eval.h"
#include "handle.h"
#include "gc.h"
#include "stdlib_higher_order.h"

SCON_API scon_handle_t scon_map(scon_t* scon, scon_handle_t* list, scon_handle_t* callable)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (!SCON_HANDLE_IS_LIST(list))
    {
        SCON_ERROR_RUNTIME(scon,  "map expects a list as the first argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, list)));
    }

    if (!SCON_HANDLE_IS_CALLABLE(callable))
    {
        SCON_ERROR_RUNTIME(scon,  "map expects a callable as the second argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, callable)));
    }

    scon_item_t* listItem = SCON_HANDLE_TO_ITEM(list);

    scon_list_t* mappedList = scon_list_new(scon, listItem->length);
    scon_handle_t mappedHandle = SCON_HANDLE_FROM_LIST(mappedList);
    SCON_GC_RETAIN(scon, mappedHandle);

    for (scon_size_t i = 0; i < listItem->length; i++)
    {
        scon_handle_t arg = SCON_LIST_GET_HANDLE(listItem, i);
        scon_handle_t result = scon_eval_call(scon, *callable, 1, &arg);
        scon_list_append(scon, mappedList, result);
    }

    SCON_GC_RELEASE(scon, mappedHandle);
    return mappedHandle;
}

SCON_API scon_handle_t scon_filter(scon_t* scon, scon_handle_t* list, scon_handle_t* callable)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (!SCON_HANDLE_IS_LIST(list))
    {
        SCON_ERROR_RUNTIME(scon,  "filter expects a list as the first argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, list)));
    }

    if (!SCON_HANDLE_IS_CALLABLE(callable))
    {
        SCON_ERROR_RUNTIME(scon,  "filter expects a callable as the second argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, callable)));
    }

    scon_item_t* listItem = SCON_HANDLE_TO_ITEM(list);

    scon_list_t* filteredList = scon_list_new(scon, 0);
    scon_handle_t filteredHandle = SCON_HANDLE_FROM_LIST(filteredList);
    SCON_GC_RETAIN(scon, filteredHandle);

    for (scon_size_t i = 0; i < listItem->length; i++)
    {
        scon_handle_t arg = SCON_LIST_GET_HANDLE(listItem, i);
        scon_handle_t result = scon_eval_call(scon, *callable, 1, &arg);
        if (SCON_HANDLE_IS_TRUTHY(&result))
        {
            scon_list_append(scon, filteredList, arg);
        }
    }

    SCON_GC_RELEASE(scon, filteredHandle);
    return filteredHandle;
}

SCON_API scon_handle_t scon_reduce(scon_t* scon, scon_handle_t* list, scon_handle_t* initial, scon_handle_t* callable)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (!SCON_HANDLE_IS_LIST(list))
    {
        SCON_ERROR_RUNTIME(scon,  "reduce expects a list as the first argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, list)));
    }

    if (!SCON_HANDLE_IS_CALLABLE(callable))
    {        
        SCON_ERROR_RUNTIME(scon,  "reduce expects a callable as the second/third argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, callable)));
    }

    scon_item_t* listItem = SCON_HANDLE_TO_ITEM(list);
    scon_handle_t accumulator = (initial != SCON_NULL) ? *initial : SCON_HANDLE_NONE;
    scon_size_t startIdx = 0;

    if (accumulator == SCON_HANDLE_NONE)
    {
        if (listItem->length == 0)
        {
            return SCON_HANDLE_NONE;
        }
        accumulator = SCON_LIST_GET_HANDLE(listItem, 0);
        startIdx = 1;
    }

    SCON_GC_RETAIN(scon, accumulator);

    for (scon_size_t i = startIdx; i < listItem->length; i++)
    {
        scon_handle_t args[2] = {accumulator, SCON_LIST_GET_HANDLE(listItem, i)};
        scon_handle_t result = scon_eval_call(scon, *callable, 2, args);
        
        if (accumulator != result)
        {
            SCON_GC_RELEASE(scon, accumulator);
            accumulator = result;
            SCON_GC_RETAIN(scon, accumulator);
        }
    }

    SCON_GC_RELEASE(scon, accumulator);
    return accumulator;
}

SCON_API scon_handle_t scon_apply(scon_t* scon, scon_handle_t* list, scon_handle_t* callable)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (!SCON_HANDLE_IS_LIST(list))
    {
        SCON_ERROR_RUNTIME(scon, "apply expects a list as the first argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, list)));
    }

    if (!SCON_HANDLE_IS_CALLABLE(callable))
    {
        SCON_ERROR_RUNTIME(scon, "apply expects a callable as the second argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, callable)));
    }

    scon_item_t* listItem = SCON_HANDLE_TO_ITEM(list);
    return scon_eval_call(scon, *callable, listItem->length, listItem->list.handles);
}

SCON_API scon_handle_t scon_any(scon_t* scon, scon_handle_t* list, scon_handle_t* callable)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (!SCON_HANDLE_IS_LIST(list))
    {
        SCON_ERROR_RUNTIME(scon, "any? expects a list as the first argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, list)));
    }

    scon_item_t* listItem = SCON_HANDLE_TO_ITEM(list);
    scon_handle_t fn = (callable != SCON_NULL) ? *callable : SCON_HANDLE_NONE;

    for (scon_size_t i = 0; i < listItem->length; i++)
    {
        scon_handle_t arg = SCON_LIST_GET_HANDLE(listItem, i);
        scon_handle_t result;
        if (fn != SCON_HANDLE_NONE)
        {
            result = scon_eval_call(scon, fn, 1, &arg);
        }
        else
        {
            result = arg;
        }

        if (SCON_HANDLE_IS_TRUTHY(&result))
        {
            return SCON_HANDLE_TRUE();
        }
    }

    return SCON_HANDLE_FALSE();
}

SCON_API scon_handle_t scon_all(scon_t* scon, scon_handle_t* list, scon_handle_t* callable)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (!SCON_HANDLE_IS_LIST(list))
    {
        SCON_ERROR_RUNTIME(scon, "all? expects a list as the first argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, list)));
    }

    scon_item_t* listItem = SCON_HANDLE_TO_ITEM(list);
    scon_handle_t fn = (callable != SCON_NULL) ? *callable : SCON_HANDLE_NONE;

    for (scon_size_t i = 0; i < listItem->length; i++)
    {
        scon_handle_t arg = SCON_LIST_GET_HANDLE(listItem, i);
        scon_handle_t result;
        if (fn != SCON_HANDLE_NONE)
        {
            result = scon_eval_call(scon, fn, 1, &arg);
        }
        else
        {
            result = arg;
        }
        SCON_GC_RETAIN(scon, result);

        if (!SCON_HANDLE_IS_TRUTHY(&result))
        {
            SCON_GC_RELEASE(scon, result);
            return SCON_HANDLE_FALSE();
        }
        SCON_GC_RELEASE(scon, result);
    }

    return SCON_HANDLE_TRUE();
}

static void scon_sort_merge(scon_t* scon, scon_handle_t callable, scon_list_t* a, size_t left, size_t right, size_t end, scon_list_t* b)
{
    scon_size_t i = left;
    scon_size_t j = right;

    for (scon_size_t k = left; k < end; k++)
    {
        scon_bool_t useLeft = SCON_FALSE;
        if (i < right)
        {
            if (j >= end)
            {
                useLeft = SCON_TRUE;
            }
            else
            {
                if (callable != SCON_HANDLE_NONE)
                {
                    scon_handle_t args[2] = {a->handles[i], a->handles[j]};
                    scon_handle_t res = scon_eval_call(scon, callable, 2, args);
                    if (SCON_HANDLE_IS_TRUTHY(&res))
                    {
                        useLeft = SCON_TRUE;
                    }
                }
                else
                {
                    if (scon_handle_compare(scon, &a->handles[i], &a->handles[j]) <= 0)
                    {
                        useLeft = SCON_TRUE;
                    }
                }
            }
        }

        if (useLeft)
        {
            b->handles[k] = a->handles[i];
            i++;
        }
        else
        {
            b->handles[k] = a->handles[j];
            j++;
        }
    }
}

SCON_API scon_handle_t scon_sort(scon_t* scon, scon_handle_t* listHandle, scon_handle_t* callableHandle)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(listHandle != SCON_NULL);

    scon_handle_t callable = (callableHandle != SCON_NULL) ? *callableHandle : SCON_HANDLE_NONE;

    if (!SCON_HANDLE_IS_LIST(listHandle))
    {
        SCON_ERROR_RUNTIME(scon,  "sort expects a list as the first argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, listHandle)));
    }

    scon_item_t* list = SCON_HANDLE_TO_ITEM(listHandle);

    if (callable != SCON_HANDLE_NONE && !SCON_HANDLE_IS_CALLABLE(&callable))
    {
        SCON_ERROR_RUNTIME(scon,  "sort expects a callable as the second argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, &callable)));
    }

    scon_list_t* aList = scon_list_new(scon, list->length);
    scon_list_t* bList = scon_list_new(scon, list->length);
    scon_handle_t aHandle = SCON_HANDLE_FROM_LIST(aList);
    scon_handle_t bHandle = SCON_HANDLE_FROM_LIST(bList);
    SCON_GC_RETAIN(scon, aHandle);
    SCON_GC_RETAIN(scon, bHandle);

    SCON_MEMCPY(aList->handles, list->list.handles, list->length * sizeof(scon_handle_t));
    aList->length = list->length;
    bList->length = list->length;

    for (scon_size_t width = 1; width < list->length; width *= 2)
    {
        for (scon_size_t i = 0; i < list->length; i += 2 * width)
        {
            scon_size_t left = i;
            scon_size_t right = SCON_MIN(i + width, list->length);
            scon_size_t end = SCON_MIN(i + 2 * width, list->length);
            scon_sort_merge(scon, callable, aList, left, right, end, bList);
        }
        scon_list_t* temp = aList;
        aList = bList;
        bList = temp;
    }

    SCON_GC_RELEASE(scon, aHandle);
    return bHandle;
}

#endif
