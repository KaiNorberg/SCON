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

    scon_list_t* mappedList = scon_list_new(scon);
    scon_handle_t mappedHandle = SCON_HANDLE_FROM_LIST(mappedList);
    SCON_GC_RETAIN(scon, mappedHandle);

    scon_handle_t entry;
    SCON_LIST_FOR_EACH(&entry, &listItem->list)
    {
        scon_handle_t result = scon_eval_call(scon, *callable, 1, &entry);
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

    scon_list_t* filteredList = scon_list_new(scon);
    scon_handle_t filteredHandle = SCON_HANDLE_FROM_LIST(filteredList);
    SCON_GC_RETAIN(scon, filteredHandle);

    scon_handle_t entry;
    SCON_LIST_FOR_EACH(&entry, &listItem->list)
    {
        scon_handle_t result = scon_eval_call(scon, *callable, 1, &entry);
        if (SCON_HANDLE_IS_TRUTHY(&result))
        {
            scon_list_append(scon, filteredList, entry);
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

    scon_list_iter_t iter = SCON_LIST_ITER(&listItem->list);
    scon_handle_t entry;

    if (accumulator == SCON_HANDLE_NONE)
    {
        if (!scon_list_iter_next(&iter, &accumulator))
        {
            return SCON_HANDLE_NONE;
        }
    }

    SCON_GC_RETAIN(scon, accumulator);

    while (scon_list_iter_next(&iter, &entry))
    {
        scon_handle_t args[2] = {accumulator, entry};
        scon_handle_t result = scon_eval_call(scon, *callable, 2, args);
        SCON_GC_RETAIN(scon, result);
        
        if (result != accumulator)
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
    scon_size_t len = listItem->length;
    if (len == 0)
    {
        return scon_eval_call(scon, *callable, 0, SCON_NULL);
    }

    scon_handle_t stackBuffer[SCON_STACK_BUFFER_SIZE];
    scon_handle_t* argv = (len <= SCON_STACK_BUFFER_SIZE) ? stackBuffer : (scon_handle_t*)SCON_MALLOC(len * sizeof(scon_handle_t));
    
    if (argv == SCON_NULL)
    {
        SCON_ERROR_INTERNAL(scon, "out of memory");
    }
    
    scon_size_t i = 0;
    scon_handle_t entry;
    SCON_LIST_FOR_EACH(&entry, &listItem->list)
    {
        argv[i++] = entry;
    }

    scon_handle_t result = scon_eval_call(scon, *callable, len, argv);

    if (argv != stackBuffer)
    {
        SCON_FREE(argv);
    }

    return result;
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

    scon_handle_t entry;
    SCON_LIST_FOR_EACH(&entry, &listItem->list)
    {
        scon_handle_t result;
        if (fn != SCON_HANDLE_NONE)
        {
            result = scon_eval_call(scon, fn, 1, &entry);
        }
        else
        {
            result = entry;
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

    scon_handle_t entry;
    SCON_LIST_FOR_EACH(&entry, &listItem->list)
    {
        scon_handle_t result;
        if (fn != SCON_HANDLE_NONE)
        {
            result = scon_eval_call(scon, fn, 1, &entry);
        }
        else
        {
            result = entry;
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

static void scon_sort_merge(scon_t* scon, scon_handle_t callable, scon_handle_t* a, size_t left, size_t right, size_t end, scon_handle_t* b)
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
                    scon_handle_t args[2] = {a[i], a[j]};
                    scon_handle_t res = scon_eval_call(scon, callable, 2, args);
                    if (SCON_HANDLE_IS_TRUTHY(&res))
                    {
                        useLeft = SCON_TRUE;
                    }
                }
                else
                {
                    if (scon_handle_compare(scon, &a[i], &a[j]) <= 0)
                    {
                        useLeft = SCON_TRUE;
                    }
                }
            }
        }

        if (useLeft)
        {
            b[k] = a[i];
            i++;
        }
        else
        {
            b[k] = a[j];
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

    scon_size_t len = list->length;
    if (len <= 1)
    {
        return *listHandle;
    }

    scon_handle_t* a = (scon_handle_t*)SCON_MALLOC(len * sizeof(scon_handle_t));
    scon_handle_t* b = (scon_handle_t*)SCON_MALLOC(len * sizeof(scon_handle_t));
    if (a == SCON_NULL || b == SCON_NULL)
    {
        SCON_ERROR_INTERNAL(scon, "out of memory");
    }

    scon_size_t idx = 0;
    scon_handle_t entry;
    SCON_LIST_FOR_EACH(&entry, &list->list)
    {
        a[idx++] = entry;
    }

    scon_handle_t* src = a;
    scon_handle_t* dst = b;
    for (scon_size_t width = 1; width < list->length; width *= 2)
    {
        for (scon_size_t i = 0; i < list->length; i += 2 * width)
        {
            scon_size_t left = i;
            scon_size_t right = SCON_MIN(i + width, len);
            scon_size_t end = SCON_MIN(i + 2 * width, len);
            scon_sort_merge(scon, callable, src, left, right, end, dst);
        }
        scon_handle_t* temp = src;
        src = dst;
        dst = temp;
    }

    scon_list_t* resultList = scon_list_new(scon);
    for (scon_size_t i = 0; i < len; i++)
    {
        scon_list_append(scon, resultList, src[i]);
    }
    
    SCON_FREE(a);
    SCON_FREE(b);
    return SCON_HANDLE_FROM_LIST(resultList);
}

#endif
