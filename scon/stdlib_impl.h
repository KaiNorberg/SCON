#ifndef SCON_STDLIB_IMPL_H
#define SCON_STDLIB_IMPL_H 1

#include "core.h"
#include "defs.h"
#include "eval.h"
#include "gc.h"
#include "handle.h"
#include "item.h"
#include "native.h"
#include "stdlib.h"

static scon_handle_t scon_stdlib_assert(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    if (argc < 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "assert expects exactly 2 arguments, got %zu", argc);
    }

    if (!SCON_HANDLE_IS_TRUTHY(&argv[0]))
    {
        char* str;
        scon_size_t len;
        scon_handle_get_string(scon, &argv[1], &str, &len);
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "assert failed: %s", str);
    }

    return argv[0];
}

static scon_handle_t scon_stdlib_throw(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "throw expects exactly 1 argument, got %zu", argc);
    }

    char* str;
    scon_size_t len;
    scon_handle_get_string(scon, &argv[0], &str, &len);
    SCON_ERROR_RUNTIME(scon, SCON_NULL, "throw: %s", str);

    return scon_handle_nil(scon); /* Unreachable */
}

static scon_handle_t scon_stdlib_map(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "map expects exactly 2 arguments, got %zu", argc);
    }

    scon_handle_t callable = argv[0];
    scon_handle_t listHandle = argv[1];

    if (!SCON_HANDLE_IS_ITEM(&listHandle))
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "map expects a list as the second argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, &listHandle)));
    }

    scon_item_t* list = SCON_HANDLE_TO_ITEM(&listHandle);
    if (list->type != SCON_ITEM_TYPE_LIST)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "map expects a list as the second argument, got %s",
            scon_item_type_str(list->type));
    }

    scon_list_t* mappedList = scon_list_new(scon, list->length);
    scon_handle_t mappedHandle = SCON_HANDLE_FROM_LIST(mappedList);
    SCON_GC_RETAIN(scon, mappedHandle);

    for (scon_size_t i = 0; i < list->length; i++)
    {
        scon_handle_t arg = SCON_LIST_GET_HANDLE(list, i);
        scon_handle_t result = scon_eval_call(scon, callable, 1, &arg);
        scon_list_append(scon, mappedList, result);
    }

    SCON_GC_RELEASE(scon, mappedHandle);
    return mappedHandle;
}

static scon_handle_t scon_stdlib_filter(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "filter expects exactly 2 arguments, got %zu", argc);
    }

    scon_handle_t callable = argv[0];
    scon_handle_t listHandle = argv[1];

    if (!SCON_HANDLE_IS_ITEM(&listHandle))
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "filter expects a list as the second argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, &listHandle)));
    }

    scon_item_t* list = SCON_HANDLE_TO_ITEM(&listHandle);
    if (list->type != SCON_ITEM_TYPE_LIST)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "filter expects a list as the second argument, got %s",
            scon_item_type_str(list->type));
    }

    scon_list_t* filteredList = scon_list_new(scon, 0);
    scon_handle_t filteredHandle = SCON_HANDLE_FROM_LIST(filteredList);
    SCON_GC_RETAIN(scon, filteredHandle);

    for (scon_size_t i = 0; i < list->length; i++)
    {
        scon_handle_t arg = SCON_LIST_GET_HANDLE(list, i);
        scon_handle_t result = scon_eval_call(scon, callable, 1, &arg);
        if (SCON_HANDLE_IS_TRUTHY(&result))
        {
            scon_list_append(scon, filteredList, arg);
        }
    }

    SCON_GC_RELEASE(scon, filteredHandle);
    return filteredHandle;
}

static scon_handle_t scon_stdlib_reduce(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    if (argc != 3)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "reduce expects exactly 3 arguments (fn, initial, list), got %zu", argc);
    }

    scon_handle_t callable = argv[0];
    scon_handle_t accumulator = argv[1];
    scon_handle_t listHandle = argv[2];

    if (!SCON_HANDLE_IS_ITEM(&listHandle))
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "reduce expects a list as the third argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, &listHandle)));
    }

    scon_item_t* list = SCON_HANDLE_TO_ITEM(&listHandle);
    if (list->type != SCON_ITEM_TYPE_LIST)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "reduce expects a list as the third argument, got %s",
            scon_item_type_str(list->type));
    }

    scon_handle_t result = accumulator;
    SCON_GC_RETAIN(scon, result);

    for (scon_size_t i = 0; i < list->length; i++)
    {
        scon_handle_t args[2];
        args[0] = result;
        args[1] = SCON_LIST_GET_HANDLE(list, i);
        scon_handle_t next = scon_eval_call(scon, callable, 2, args);
        SCON_GC_RETAIN(scon, next);
        SCON_GC_RELEASE(scon, result);
        result = next;
    }

    SCON_GC_RELEASE(scon, result);
    return result;
}

static void scon_stdlib_sort_merge(scon_t* scon, scon_handle_t callable, scon_list_t* a, size_t left, size_t right, size_t end, scon_list_t* b)
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

static scon_handle_t scon_stdlib_sort(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    // https://en.wikipedia.org/wiki/Merge_sort

    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    if (argc != 1 && argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "sort expects 1 or 2 arguments, got %zu", argc);
    }

    scon_handle_t listHandle = argv[0];
    scon_handle_t callable = (argc == 2) ? argv[1] : SCON_HANDLE_NONE;

    if (!SCON_HANDLE_IS_ITEM(&listHandle))
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "sort expects a list as the first argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, &listHandle)));
    }

    scon_item_t* list = SCON_HANDLE_TO_ITEM(&listHandle);
    if (list->type != SCON_ITEM_TYPE_LIST)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "sort expects a list as the first argument, got %s",
            scon_item_type_str(list->type));
    }

    if (callable != SCON_HANDLE_NONE && !SCON_HANDLE_IS_ITEM(&callable))
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "sort expects a callable as the second argument, got %s",
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
            scon_stdlib_sort_merge(scon, callable, aList, left, right, end, bList);
        }
        scon_list_t* temp = aList;
        aList = bList;
        bList = temp;
    }

    SCON_GC_RELEASE(scon, aHandle);
    return bHandle;
}

static scon_handle_t scon_stdlib_print(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    for (scon_size_t i = 0; i < argc; i++)
    {
        char* str;
        scon_size_t len;
        scon_handle_get_string(scon, &argv[i], &str, &len);
        SCON_FWRITE(str, 1, len, SCON_STDOUT);
        if (i < argc - 1)
        {
            SCON_FWRITE(" ", 1, 1, SCON_STDOUT);
        }
    }
    return scon_handle_nil(scon);
}

static scon_handle_t scon_stdlib_println(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    scon_handle_t handle = scon_stdlib_print(scon, argc, argv);
    SCON_FWRITE("\n", 1, 1, SCON_STDOUT);
    return handle;
}

SCON_API void scon_stdlib_register(scon_t* scon, scon_stdlib_sets_t sets)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (sets & SCON_STDLIB_ERROR)
    {
        scon_native_t natives[] = {
            {"assert!", scon_stdlib_assert},
            {"throw!", scon_stdlib_throw},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
    if (sets & SCON_STDLIB_HIGHER_ORDER)
    {
        scon_native_t natives[] = {
            {"map", scon_stdlib_map},
            {"filter", scon_stdlib_filter},
            {"reduce", scon_stdlib_reduce},
            {"sort", scon_stdlib_sort},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
    if (sets & SCON_STDLIB_SYSTEM)
    {
        scon_native_t natives[] = {
            {"print!", scon_stdlib_print},
            {"println!", scon_stdlib_println},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
}

#endif
