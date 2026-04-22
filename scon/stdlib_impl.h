#include "atom.h"
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

#define SCON_STACK_BUFFER_SIZE 256

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

static scon_handle_t scon_stdlib_concat(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    scon_bool_t resultIsList = SCON_FALSE;
    scon_size_t totalLen = 0;

    for (scon_size_t i = 0; i < argc; i++)
    {
        scon_item_type_t type = scon_handle_get_type(scon, &argv[i]);
        if (type == SCON_ITEM_TYPE_LIST)
        {
            resultIsList = SCON_TRUE;
        }
        totalLen += scon_handle_len(scon, &argv[i]);
    }

    if (resultIsList)
    {
        scon_list_t* newList = scon_list_new(scon, totalLen);
        for (scon_size_t i = 0; i < argc; i++)
        {
            scon_item_type_t type = scon_handle_get_type(scon, &argv[i]);
            if (type == SCON_ITEM_TYPE_LIST)
            {
                scon_item_t* item = SCON_HANDLE_TO_ITEM(&argv[i]);
                for (scon_size_t j = 0; j < item->list.length; j++)
                {
                    scon_list_append(scon, newList, SCON_LIST_GET_HANDLE(item, j));
                }
            }
            else
            {
                scon_list_append(scon, newList, argv[i]);
            }
        }
        return SCON_HANDLE_FROM_LIST(newList);
    }

    char stackBuffer[SCON_STACK_BUFFER_SIZE];
    char* buffer = (totalLen < SCON_STACK_BUFFER_SIZE) ? stackBuffer : (char*)SCON_MALLOC(totalLen);
    if (buffer == SCON_NULL)
    {
        SCON_ERROR_INTERNAL(scon, "out of memory");
    }

    scon_size_t currentPos = 0;
    for (scon_size_t i = 0; i < argc; i++)
    {            
        char* str;
        scon_size_t len;
        scon_handle_get_string(scon, &argv[i], &str, &len);
        SCON_MEMCPY(buffer + currentPos, str, len);
        currentPos += len;
    }

    scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, totalLen, SCON_ATOM_LOOKUP_NONE));
    if (buffer != stackBuffer)
    {
        SCON_FREE(buffer);
    }
    return result;
}

static scon_handle_t scon_stdlib_first(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "first expects exactly 1 argument, got %zu", argc);
    }

    scon_handle_t handle = argv[0];
    scon_handle_ensure_item(scon, &handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(&handle);

    if (item->length == 0)
    {
        return scon_handle_nil(scon);
    }

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
        return item->list.handles[0];
    case SCON_ITEM_TYPE_ATOM:
        return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, item->atom.string, 1, SCON_ATOM_LOOKUP_NONE));
    default:
        SCON_ERROR_RUNTIME(scon, item, "first expected list or atom, got %s", scon_item_type_str(item->type));
    }
}

static scon_handle_t scon_stdlib_last(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "last expects exactly 1 argument, got %zu", argc);
    }

    scon_handle_t handle = argv[0];
    scon_handle_ensure_item(scon, &handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(&handle);

    if (item->length == 0)
    {
        return scon_handle_nil(scon);
    }

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
        return item->list.handles[item->length - 1];
    case SCON_ITEM_TYPE_ATOM:
        return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, item->atom.string + item->length - 1, 1, SCON_ATOM_LOOKUP_NONE));
    default:
        SCON_ERROR_RUNTIME(scon, item, "last expected list or atom, got %s", scon_item_type_str(item->type));
    }
}

static scon_handle_t scon_stdlib_rest(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "rest expects exactly 1 argument, got %zu", argc);
    }

    scon_handle_t handle = argv[0];
    scon_handle_ensure_item(scon, &handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(&handle);

    if (item->length <= 1)
    {
        return scon_handle_nil(scon);
    }

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
    {
        scon_size_t newLen = item->length - 1;
        scon_list_t* newList = scon_list_new(scon, newLen);
        for (scon_size_t i = 0; i < newLen; i++)
        {
            scon_list_append(scon, newList, item->list.handles[i + 1]);
        }
        return SCON_HANDLE_FROM_LIST(newList);
    }
    case SCON_ITEM_TYPE_ATOM:
    {
        return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, item->atom.string + 1, item->length - 1, SCON_ATOM_LOOKUP_NONE));
    }
    default:
        SCON_ERROR_RUNTIME(scon, item, "rest expected list or atom, got %s", scon_item_type_str(item->type));
    }
}

static scon_handle_t scon_stdlib_init(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "init expects exactly 1 argument, got %zu", argc);
    }

    scon_handle_t handle = argv[0];
    scon_handle_ensure_item(scon, &handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(&handle);

    if (item->length <= 1)
    {
        return scon_handle_nil(scon);
    }

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
    {
        scon_size_t newLen = item->length - 1;
        scon_list_t* newList = scon_list_new(scon, newLen);
        for (scon_size_t i = 0; i < newLen; i++)
        {
            scon_list_append(scon, newList, item->list.handles[i]);
        }
        return SCON_HANDLE_FROM_LIST(newList);
    }
    case SCON_ITEM_TYPE_ATOM:
    {
        return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, item->atom.string, item->length - 1, SCON_ATOM_LOOKUP_NONE));
    }
    default:
        SCON_ERROR_RUNTIME(scon, item, "init expected list or atom, got %s", scon_item_type_str(item->type));
    }
}

static scon_handle_t scon_stdlib_nth(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "nth expects exactly 2 arguments (list/atom, index), got %zu", argc);
    }

    scon_handle_t handle = argv[0];
    scon_int64_t n = scon_handle_get_int(scon, &argv[1]);

    scon_handle_ensure_item(scon, &handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(&handle);

    if (n < 0)
    {
        n = (scon_int64_t)item->length + n;
    }

    if (n < 0 || n >= (scon_int64_t)item->length)
    {
        return scon_handle_nil(scon);
    }

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
        return item->list.handles[n];
    case SCON_ITEM_TYPE_ATOM:
        return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, item->atom.string + n, 1, SCON_ATOM_LOOKUP_NONE));
    default:
        SCON_ERROR_RUNTIME(scon, item, "nth expected list or atom, got %s", scon_item_type_str(item->type));
    }
}

static scon_handle_t scon_stdlib_index_index(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "index expects exactly 2 arguments (list/atom, value), got %zu", argc);
    }

    scon_handle_t handle = argv[0];
    scon_handle_t target = argv[1];

    scon_handle_ensure_item(scon, &handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(&handle);

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
    {
        for (scon_size_t i = 0; i < item->length; i++)
        {
            scon_handle_t current = item->list.handles[i];
            if (scon_handle_compare(scon, &current, &target) == 0)
            {
                return SCON_HANDLE_FROM_INT(i);
            }
        }
        break;
    }
    case SCON_ITEM_TYPE_ATOM:
    {
        char* targetStr;
        scon_size_t targetLen;
        scon_handle_get_string(scon, &target, &targetStr, &targetLen);

        if (targetLen == 0)
        {
            return SCON_HANDLE_FROM_INT(0);
        }

        if (targetLen <= item->length)
        {
            for (scon_size_t i = 0; i <= item->length - targetLen; i++)
            {
                if (SCON_MEMCMP(item->atom.string + i, targetStr, targetLen) == 0)
                {
                    return SCON_HANDLE_FROM_INT(i);
                }
            }
        }
        break;
    }
    default:
        SCON_ERROR_RUNTIME(scon, item, "index expected list or atom, got %s", scon_item_type_str(item->type));
    }

    return scon_handle_nil(scon);
}

static scon_handle_t scon_stdlib_reverse(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "reverse expects exactly 1 argument, got %zu", argc);
    }

    scon_handle_t handle = argv[0];
    scon_handle_ensure_item(scon, &handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(&handle);

    if (item->length <= 1)
    {
        return handle;
    }

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
    {
        scon_list_t* newList = scon_list_new(scon, item->length);
        for (scon_size_t i = 0; i < item->length; i++)
        {
            scon_list_append(scon, newList, item->list.handles[item->length - 1 - i]);
        }
        return SCON_HANDLE_FROM_LIST(newList);
    }
    case SCON_ITEM_TYPE_ATOM:
    {
        char stackBuffer[SCON_STACK_BUFFER_SIZE];
        char* buffer = (item->length < SCON_STACK_BUFFER_SIZE) ? stackBuffer : (char*)SCON_MALLOC(item->length);
        if (buffer == SCON_NULL)
        {
            SCON_ERROR_INTERNAL(scon, "out of memory");
        }

        for (scon_size_t i = 0; i < item->length; i++)
        {
            buffer[i] = item->atom.string[item->length - 1 - i];
        }

        scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, item->length, SCON_ATOM_LOOKUP_NONE));
        if (buffer != stackBuffer)
        {            
            SCON_FREE(buffer);
        }
        return result;
    }
    default:
        SCON_ERROR_RUNTIME(scon, item, "reverse expected list or atom, got %s", scon_item_type_str(item->type));
    }
}

static scon_handle_t scon_stdlib_slice(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    if (argc < 2 || argc > 3)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "slice expects 2 or 3 arguments (list/atom, start, [end]), got %zu", argc);
    }

    scon_handle_t handle = argv[0];
    scon_int64_t start = scon_handle_get_int(scon, &argv[1]);
    scon_handle_ensure_item(scon, &handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(&handle);

    scon_int64_t end = (argc == 3) ? scon_handle_get_int(scon, &argv[2]) : (scon_int64_t)item->length;

    if (start < 0)
    {
        start = (scon_int64_t)item->length + start;
    }
    if (end < 0)
    {
        end = (scon_int64_t)item->length + end;
    }

    start = SCON_MAX(0, SCON_MIN(start, (scon_int64_t)item->length));
    end = SCON_MAX(0, SCON_MIN(end, (scon_int64_t)item->length));

    if (start >= end)
    {
        return (item->type == SCON_ITEM_TYPE_LIST) ? SCON_HANDLE_FROM_LIST(scon_list_new(scon, 0)) : scon_handle_nil(scon);
    }

    scon_size_t newLen = (scon_size_t)(end - start);

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
    {
        scon_list_t* newList = scon_list_new(scon, newLen);
        for (scon_size_t i = 0; i < newLen; i++)
        {
            scon_list_append(scon, newList, item->list.handles[start + i]);
        }
        return SCON_HANDLE_FROM_LIST(newList);
    }
    case SCON_ITEM_TYPE_ATOM:
    {
        return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, item->atom.string + start, newLen, SCON_ATOM_LOOKUP_NONE));
    }
    default:
        SCON_ERROR_RUNTIME(scon, item, "slice expected list or atom, got %s", scon_item_type_str(item->type));
    }
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
    if (sets & SCON_STDLIB_SEQUENCES)
    {
        scon_native_t natives[] = {
            {"concat", scon_stdlib_concat},
            {"first", scon_stdlib_first},
            {"last", scon_stdlib_last},
            {"rest", scon_stdlib_rest},
            {"init", scon_stdlib_init},
            {"nth", scon_stdlib_nth},
            {"index", scon_stdlib_index_index},
            {"reverse", scon_stdlib_reverse},
            {"slice", scon_stdlib_slice},
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
