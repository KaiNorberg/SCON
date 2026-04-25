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
#include "char.h"
#include "stringify.h"
#include "parse.h"
#include "eval.h"
#include "compile.h"

SCON_API scon_handle_t scon_assert(scon_t* scon, scon_handle_t* cond, scon_handle_t* msg)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (!SCON_HANDLE_IS_TRUTHY(cond))
    {
        char* str;
        scon_size_t len;
        scon_handle_get_string_params(scon, msg, &str, &len);
        SCON_ERROR_RUNTIME(scon, "assert failed: %s", str);
    }

    return *cond;
}

SCON_API scon_handle_t scon_throw(scon_t* scon, scon_handle_t* msg)
{
    SCON_ASSERT(scon != SCON_NULL);

    char* str;
    scon_size_t len;
    scon_handle_get_string_params(scon, msg, &str, &len);
    SCON_ERROR_RUNTIME(scon, "%s", str);

    return scon_handle_nil(scon);
}

SCON_API scon_handle_t scon_try(scon_t* scon, scon_handle_t* callable, scon_handle_t* catchFn)
{
    SCON_ASSERT(scon != SCON_NULL);

    SCON_ERROR_CHECK_CALLABLE(scon, callable, "try");
    SCON_ERROR_CHECK_CALLABLE(scon, catchFn, "try");

    scon_error_t* prev = scon->error;

    scon_error_t error = SCON_ERROR();
    if (SCON_ERROR_CATCH(&error))
    {
        scon_handle_t msg = SCON_HANDLE_FROM_ATOM(
            scon_atom_lookup(scon, error.message, SCON_STRLEN(error.message), SCON_ATOM_LOOKUP_NONE));
        scon_handle_t result = scon_eval_call(scon, *catchFn, 1, &msg);
        scon->error = prev;
        return result;
    }

    scon_handle_t result = scon_eval_call(scon, *callable, 0, SCON_NULL);
    scon->error = prev;
    return result;
}

SCON_API scon_handle_t scon_map(scon_t* scon, scon_handle_t* list, scon_handle_t* callable)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ERROR_CHECK_LIST(scon, list, "map");
    SCON_ERROR_CHECK_CALLABLE(scon, callable, "map");

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
    SCON_ERROR_CHECK_LIST(scon, list, "filter");
    SCON_ERROR_CHECK_CALLABLE(scon, callable, "filter");

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
    SCON_ERROR_CHECK_LIST(scon, list, "reduce");
    SCON_ERROR_CHECK_CALLABLE(scon, callable, "reduce");

    scon_item_t* listItem = SCON_HANDLE_TO_ITEM(list);
    scon_handle_t accumulator = (initial != SCON_NULL) ? *initial : SCON_HANDLE_NONE;

    scon_list_iter_t iter = SCON_LIST_ITER(&listItem->list);
    scon_handle_t entry;

    if (accumulator == SCON_HANDLE_NONE)
    {
        if (!scon_list_iter_next(&iter, &accumulator))
        {
            return scon_handle_nil(scon);
        }
    }

    SCON_GC_RETAIN(scon, accumulator);

    while (scon_list_iter_next(&iter, &entry))
    {
        scon_handle_t args[2] = {accumulator, entry};
        scon_handle_t result = scon_eval_call(scon, *callable, 2, args);
        SCON_GC_RETAIN(scon, result);

        SCON_GC_RELEASE(scon, accumulator);
        accumulator = result;
    }

    SCON_GC_RELEASE(scon, accumulator);
    return accumulator;
}

SCON_API scon_handle_t scon_apply(scon_t* scon, scon_handle_t* list, scon_handle_t* callable)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ERROR_CHECK_LIST(scon, list, "apply");
    SCON_ERROR_CHECK_CALLABLE(scon, callable, "apply");

    scon_item_t* listItem = SCON_HANDLE_TO_ITEM(list);
    scon_size_t len = listItem->length;
    if (len == 0)
    {
        return scon_eval_call(scon, *callable, 0, SCON_NULL);
    }

    scon_handle_t stackBuffer[SCON_STACK_BUFFER_SIZE];
    scon_handle_t* argv =
        (len <= SCON_STACK_BUFFER_SIZE) ? stackBuffer : (scon_handle_t*)SCON_MALLOC(len * sizeof(scon_handle_t));

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

static inline scon_handle_t scon_eval_maybe_call(scon_t* scon, scon_handle_t fn, scon_handle_t* arg)
{
    if (fn == SCON_HANDLE_NONE)
    {
        return *arg;
    }
    else
    {
        return scon_eval_call(scon, fn, 1, arg);
    }
}

#define SCON_ANY_ALL_IMPL(_name, _predicate, _default) \
    SCON_API scon_handle_t _name(scon_t* scon, scon_handle_t* list, scon_handle_t* callable) \
    { \
        SCON_ASSERT(scon != SCON_NULL); \
        SCON_ERROR_CHECK_LIST(scon, list, #_name); \
        scon_item_t* listItem = SCON_HANDLE_TO_ITEM(list); \
        scon_handle_t fn = (callable != SCON_NULL) ? *callable : SCON_HANDLE_NONE; \
        scon_handle_t entry; \
        SCON_LIST_FOR_EACH(&entry, &listItem->list) \
        { \
            scon_handle_t result = scon_eval_maybe_call(scon, fn, &entry); \
            SCON_GC_RETAIN(scon, result); \
            if (_predicate) \
            { \
                SCON_GC_RELEASE(scon, result); \
                return SCON_HANDLE_FROM_INT(!(_default)); \
            } \
            SCON_GC_RELEASE(scon, result); \
        } \
        return SCON_HANDLE_FROM_INT(_default); \
    }

SCON_ANY_ALL_IMPL(scon_any, SCON_HANDLE_IS_TRUTHY(&result), SCON_FALSE)
SCON_ANY_ALL_IMPL(scon_all, !SCON_HANDLE_IS_TRUTHY(&result), SCON_TRUE)

static void scon_sort_merge(scon_t* scon, scon_handle_t callable, scon_handle_t* a, scon_size_t left, scon_size_t right,
    scon_size_t end, scon_handle_t* b)
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

    SCON_ERROR_CHECK_LIST(scon, listHandle, "sort");
    scon_item_t* list = SCON_HANDLE_TO_ITEM(listHandle);

    scon_handle_t callable = (callableHandle != SCON_NULL) ? *callableHandle : SCON_HANDLE_NONE;
    if (callable != SCON_HANDLE_NONE)
    {
        SCON_ERROR_CHECK_CALLABLE(scon, &callable, "sort");
    }

    scon_size_t len = list->length;
    if (len <= 1)
    {
        return *listHandle;
    }

    scon_handle_t* a = (scon_handle_t*)SCON_MALLOC(len * sizeof(scon_handle_t));
    if (a == SCON_NULL)
    {
        SCON_ERROR_INTERNAL(scon, "out of memory");
    }

    scon_handle_t* b = (scon_handle_t*)SCON_MALLOC(len * sizeof(scon_handle_t));
    if (b == SCON_NULL)
    {
        SCON_FREE(a);
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
    scon_handle_t resultHandle = SCON_HANDLE_FROM_LIST(resultList);
    SCON_GC_RETAIN(scon, resultHandle);

    for (scon_size_t i = 0; i < len; i++)
    {
        scon_list_append(scon, resultList, src[i]);
    }

    SCON_FREE(a);
    SCON_FREE(b);
    SCON_GC_RELEASE(scon, resultHandle);
    return resultHandle;
}

static inline scon_int64_t scon_handle_normalize_index(scon_t* scon, scon_handle_t* index, scon_size_t length)
{
    scon_handle_t nHandle = scon_get_int(scon, index);
    scon_int64_t n = SCON_HANDLE_TO_INT(&nHandle);
    if (n < 0)
    {
        n = (scon_int64_t)length + n;
    }
    return n;
}

static inline void scon_sequence_normalize_range(scon_t* scon, scon_handle_t* startH, scon_handle_t* endH,
    scon_size_t length, scon_size_t* outStart, scon_size_t* outEnd)
{
    scon_int64_t start = scon_handle_normalize_index(scon, startH, length);
    scon_int64_t end;

    if (endH != SCON_NULL)
    {
        end = scon_handle_normalize_index(scon, endH, length);
    }
    else
    {
        end = (scon_int64_t)length;
    }

    start = SCON_MAX(0, SCON_MIN(start, (scon_int64_t)length));
    end = SCON_MAX(0, SCON_MIN(end, (scon_int64_t)length));

    *outStart = (scon_size_t)start;
    *outEnd = (scon_size_t)end;
}

static inline scon_handle_t scon_list_find_entry(scon_t* scon, scon_item_t* listItem, scon_handle_t* key)
{
    scon_handle_t entryH;
    SCON_LIST_FOR_EACH(&entryH, &listItem->list)
    {
        if (SCON_HANDLE_IS_LIST(&entryH))
        {
            scon_item_t* entry = SCON_HANDLE_TO_ITEM(&entryH);
            if (entry->length >= 1)
            {
                scon_handle_t entryKey = scon_list_nth(scon, &entry->list, 0);
                if (scon_handle_compare(scon, &entryKey, key) == 0)
                {
                    return entryH;
                }
            }
        }
    }
    return SCON_HANDLE_NONE;
}

static inline scon_bool_t scon_list_get_entry(scon_t* scon, scon_handle_t* entryH, scon_handle_t* outKey,
    scon_handle_t* outVal)
{
    if (!SCON_HANDLE_IS_LIST(entryH))
    {
        return SCON_FALSE;
    }
    scon_item_t* entry = SCON_HANDLE_TO_ITEM(entryH);
    if (entry->length < 1)
    {
        return SCON_FALSE;
    }

    if (outKey != SCON_NULL)
    {
        *outKey = scon_list_nth(scon, &entry->list, 0);
    }
    
    if (outVal != SCON_NULL)
    {
        *outVal = (entry->length >= 2) ? scon_list_nth(scon, &entry->list, 1) : scon_handle_nil(scon);
    }
    return SCON_TRUE;
}

SCON_API scon_handle_t scon_len(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    scon_size_t total = 0;
    for (scon_size_t i = 0; i < argc; i++)
    {
        total += scon_handle_item(scon, &argv[i])->length;
    }
    return SCON_HANDLE_FROM_INT(total);
}

SCON_API scon_handle_t scon_range(struct scon* scon, scon_handle_t* start, scon_handle_t* end, scon_handle_t* step)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_t startH = scon_get_int(scon, start);
    scon_handle_t endH = scon_get_int(scon, end);
    scon_handle_t stepH = (step != SCON_NULL) ? scon_get_int(scon, step) : SCON_HANDLE_FROM_INT(1);

    scon_int64_t startVal = SCON_HANDLE_TO_INT(&startH);
    scon_int64_t endVal = SCON_HANDLE_TO_INT(&endH);
    scon_int64_t stepVal = SCON_HANDLE_TO_INT(&stepH);

    if (stepVal == 0)
    {
        SCON_ERROR_RUNTIME(scon, "range step cannot be 0");
    }

    scon_size_t count = 0;
    if (stepVal > 0)
    {
        if (endVal > startVal)
        {
            count = (scon_size_t)((endVal - startVal + stepVal - 1) / stepVal);
        }
    }
    else
    {
        if (startVal > endVal)
        {
            count = (scon_size_t)((startVal - endVal - stepVal - 1) / -stepVal);
        }
    }

    scon_list_t* list = scon_list_new(scon);
    scon_handle_t listHandle = SCON_HANDLE_FROM_LIST(list);
    SCON_GC_RETAIN(scon, listHandle);

    scon_int64_t current = startVal;
    for (scon_size_t i = 0; i < count; i++)
    {
        scon_list_append(scon, list, SCON_HANDLE_FROM_INT(current));
        current += stepVal;
    }

    SCON_GC_RELEASE(scon, listHandle);
    return listHandle;
}

SCON_API scon_handle_t scon_concat(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    scon_bool_t resultIsList = SCON_FALSE;
    scon_size_t totalLen = 0;

    for (scon_size_t i = 0; i < argc; i++)
    {
        if (SCON_HANDLE_IS_LIST(&argv[i]))
        {
            resultIsList = SCON_TRUE;
        }
        totalLen += scon_handle_item(scon, &argv[i])->length;
    }

    if (resultIsList)
    {
        scon_list_t* newList = scon_list_new(scon);
        scon_handle_t newHandle = SCON_HANDLE_FROM_LIST(newList);
        SCON_GC_RETAIN(scon, newHandle);

        for (scon_size_t i = 0; i < argc; i++)
        {
            if (SCON_HANDLE_IS_LIST(&argv[i]))
            {
                scon_list_append_list(scon, newList, &SCON_HANDLE_TO_ITEM(&argv[i])->list);
            }
            else
            {
                scon_list_append(scon, newList, argv[i]);
            }
        }

        SCON_GC_RELEASE(scon, newHandle);
        return newHandle;
    }

    SCON_SCRATCH_BUFFER(buffer, totalLen);

    scon_size_t currentPos = 0;
    for (scon_size_t i = 0; i < argc; i++)
    {
        char* str;
        scon_size_t len;
        scon_handle_get_string_params(scon, &argv[i], &str, &len);
        SCON_MEMCPY(buffer + currentPos, str, len);
        currentPos += len;
    }

    scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, totalLen, SCON_ATOM_LOOKUP_NONE));
    SCON_SCRATCH_BUFFER_FREE(buffer);
    return result;
}

static inline scon_handle_t scon_sequence_edge(scon_t* scon, scon_handle_t* handle, scon_bool_t first)
{
    scon_item_t* item = scon_handle_item(scon, handle);

    if (item->length == 0)
    {
        return scon_handle_nil(scon);
    }

    scon_size_t index = first ? 0 : item->length - 1;

    if (item->type == SCON_ITEM_TYPE_LIST)
    {
        return scon_list_nth(scon, &item->list, index);
    }
    else
    {
        return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, item->atom.string + index, 1, SCON_ATOM_LOOKUP_NONE));
    }
}

SCON_API scon_handle_t scon_first(scon_t* scon, scon_handle_t* handle)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ERROR_CHECK_SEQUENCE(scon, handle, "first");
    return scon_sequence_edge(scon, handle, SCON_TRUE);
}

SCON_API scon_handle_t scon_last(scon_t* scon, scon_handle_t* handle)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ERROR_CHECK_SEQUENCE(scon, handle, "last");
    return scon_sequence_edge(scon, handle, SCON_FALSE);
}

static inline scon_handle_t scon_sequence_trim(scon_t* scon, scon_handle_t* handle, scon_bool_t rest)
{
    scon_item_t* item = scon_handle_item(scon, handle);

    if (item->length <= 1)
    {
        return scon_handle_nil(scon);
    }

    scon_size_t start = rest ? 1 : 0;
    scon_size_t end = rest ? item->length : item->length - 1;

    if (item->type == SCON_ITEM_TYPE_LIST)
    {
        return SCON_HANDLE_FROM_LIST(scon_list_slice(scon, &item->list, start, end));
    }
    else
    {
        return SCON_HANDLE_FROM_ATOM(
            scon_atom_lookup(scon, item->atom.string + start, end - start, SCON_ATOM_LOOKUP_NONE));
    }
}

SCON_API scon_handle_t scon_rest(scon_t* scon, scon_handle_t* handle)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ERROR_CHECK_SEQUENCE(scon, handle, "rest");
    return scon_sequence_trim(scon, handle, SCON_TRUE);
}

SCON_API scon_handle_t scon_init(scon_t* scon, scon_handle_t* handle)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ERROR_CHECK_SEQUENCE(scon, handle, "init");
    return scon_sequence_trim(scon, handle, SCON_FALSE);
}

SCON_API scon_handle_t scon_nth(scon_t* scon, scon_handle_t* handle, scon_handle_t* index, scon_handle_t* defaultVal)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ERROR_CHECK_SEQUENCE(scon, handle, "nth");

    scon_item_t* item = scon_handle_item(scon, handle);

    scon_int64_t n = scon_handle_normalize_index(scon, index, item->length);

    if (n < 0 || n >= (scon_int64_t)item->length)
    {
        return (defaultVal != SCON_NULL) ? *defaultVal : scon_handle_nil(scon);
    }

    if (item->type == SCON_ITEM_TYPE_LIST)
    {
        return scon_list_nth(scon, &item->list, (scon_size_t)n);
    }
    else
    {
        return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, item->atom.string + n, 1, SCON_ATOM_LOOKUP_NONE));
    }
}

static inline char scon_handle_get_char(scon_t* scon, scon_handle_t* handle, char defaultChar)
{
    char* str;
    scon_size_t len;
    scon_handle_get_string_params(scon, handle, &str, &len);
    return (len > 0) ? str[0] : defaultChar;
}

SCON_API scon_handle_t scon_assoc(scon_t* scon, scon_handle_t* handle, scon_handle_t* index, scon_handle_t* value,
    scon_handle_t* fillVal)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_item_t* item = scon_handle_item(scon, handle);

    scon_int64_t n = scon_handle_normalize_index(scon, index, item->length);
    if (n < 0)
    {
        n = 0;
    }

    scon_size_t targetIndex = (scon_size_t)n;

    if (targetIndex >= item->length && fillVal == SCON_NULL)
    {
        SCON_ERROR_RUNTIME(scon, "assoc index %zu out of bounds", targetIndex);
    }

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
    {
        if (targetIndex < item->length)
        {
            scon_list_t* newList = scon_list_assoc(scon, &item->list, targetIndex, *value);
            return SCON_HANDLE_FROM_LIST(newList);
        }

        scon_list_t* newList = scon_list_new(scon);
        scon_handle_t newListH = SCON_HANDLE_FROM_LIST(newList);
        SCON_GC_RETAIN(scon, newListH);
        scon_list_append_list(scon, newList, &item->list);
        for (scon_size_t i = item->length; i < targetIndex; i++)
        {
            scon_list_append(scon, newList, *fillVal);
        }
        scon_list_append(scon, newList, *value);
        SCON_GC_RELEASE(scon, newListH);
        return newListH;
    }
    case SCON_ITEM_TYPE_ATOM:
    {
        char charToPut = scon_handle_get_char(scon, value, '\0');
        char padChar = (fillVal != SCON_NULL) ? scon_handle_get_char(scon, fillVal, ' ') : ' ';

        scon_size_t newLen = SCON_MAX(item->length, targetIndex + 1);

        SCON_SCRATCH_BUFFER(buffer, newLen);

        scon_size_t prefixLen = SCON_MIN(item->length, targetIndex);
        SCON_MEMCPY(buffer, item->atom.string, prefixLen);
        for (scon_size_t i = item->length; i < targetIndex; i++)
        {
            buffer[i] = padChar;
        }
        buffer[targetIndex] = charToPut;
        if (targetIndex + 1 < item->length)
        {
            scon_size_t suffixLen = item->length - (targetIndex + 1);
            SCON_MEMCPY(buffer + targetIndex + 1, item->atom.string + targetIndex + 1, suffixLen);
        }

        scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, newLen, SCON_ATOM_LOOKUP_NONE));
        SCON_SCRATCH_BUFFER_FREE(buffer);
        return result;
    }
    default:
        SCON_ERROR_RUNTIME(scon, "assoc expected list or atom, got %s", scon_item_type_str(item->type));
    }
}

SCON_API scon_handle_t scon_dissoc(struct scon* scon, scon_handle_t* handle, scon_handle_t* index)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_item_t* item = scon_handle_item(scon, handle);

    scon_int64_t n = scon_handle_normalize_index(scon, index, item->length);

    if (n < 0 || n >= (scon_int64_t)item->length)
    {
        return *handle;
    }

    scon_size_t targetIndex = (scon_size_t)n;
    scon_size_t newLen = item->length - 1;

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
    {
        scon_list_t* newList = scon_list_dissoc(scon, &item->list, targetIndex);
        return SCON_HANDLE_FROM_LIST(newList);
    }
    case SCON_ITEM_TYPE_ATOM:
    {
        SCON_SCRATCH_BUFFER(buffer, newLen);

        SCON_MEMCPY(buffer, item->atom.string, targetIndex);
        SCON_MEMCPY(buffer + targetIndex, item->atom.string + targetIndex + 1, item->length - targetIndex - 1);

        scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, newLen, SCON_ATOM_LOOKUP_NONE));
        SCON_SCRATCH_BUFFER_FREE(buffer);
        return result;
    }
    default:
        SCON_ERROR_RUNTIME(scon, "dissoc expected list or atom, got %s", scon_item_type_str(item->type));
    }
}

SCON_API scon_handle_t scon_update(scon_t* scon, scon_handle_t* handle, scon_handle_t* index, scon_handle_t* callable,
    scon_handle_t* fillVal)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_item_t* item = scon_handle_item(scon, handle);

    scon_int64_t targetIndex = scon_handle_normalize_index(scon, index, item->length);
    if (targetIndex < 0)
    {
        targetIndex = 0;
    }

    if ((scon_size_t)targetIndex >= item->length && fillVal == SCON_NULL)
    {
        SCON_ERROR_RUNTIME(scon, "update index %lld out of bounds", (long long)targetIndex);
    }

    scon_handle_t currentVal = scon_nth(scon, handle, index, fillVal);
    scon_handle_t newVal = scon_eval_call(scon, *callable, 1, &currentVal);

    return scon_assoc(scon, handle, index, &newVal, fillVal);
}

SCON_API scon_handle_t scon_index_of(scon_t* scon, scon_handle_t* handle, scon_handle_t* target)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_item_t* item = scon_handle_item(scon, handle);

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
    {
        scon_handle_t current;
        scon_size_t i = 0;
        SCON_LIST_FOR_EACH(&current, &item->list)
        {
            if (scon_handle_compare(scon, &current, target) == 0)
            {
                return SCON_HANDLE_FROM_INT(i);
            }
            i++;
        }
    }
    break;
    case SCON_ITEM_TYPE_ATOM:
    {
        char* targetStr;
        scon_size_t targetLen;
        scon_handle_get_string_params(scon, target, &targetStr, &targetLen);

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
        SCON_ERROR_RUNTIME(scon, "index-of expected list or atom, got %s", scon_item_type_str(item->type));
    }

    return scon_handle_nil(scon);
}

SCON_API scon_handle_t scon_reverse(scon_t* scon, scon_handle_t* handle)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_item_t* item = scon_handle_item(scon, handle);

    if (item->length <= 1)
    {
        return *handle;
    }

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
    {
        scon_list_t* newList = scon_list_new(scon);
        scon_handle_t newHandle = SCON_HANDLE_FROM_LIST(newList);
        SCON_GC_RETAIN(scon, newHandle);

        scon_size_t i = item->length;
        while (i > 0)
        {
            scon_list_append(scon, newList, scon_list_nth(scon, &item->list, --i));
        }

        SCON_GC_RELEASE(scon, newHandle);
        return newHandle;
    }
    case SCON_ITEM_TYPE_ATOM:
    {
        SCON_SCRATCH_BUFFER(buffer, item->length);

        for (scon_size_t i = 0; i < item->length; i++)
        {
            buffer[i] = item->atom.string[item->length - 1 - i];
        }

        scon_handle_t result =
            SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, item->length, SCON_ATOM_LOOKUP_NONE));
        SCON_SCRATCH_BUFFER_FREE(buffer);
        return result;
    }
    default:
        SCON_ERROR_RUNTIME(scon, "reverse expected list or atom, got %s", scon_item_type_str(item->type));
    }
}

SCON_API scon_handle_t scon_slice(scon_t* scon, scon_handle_t* handle, scon_handle_t* startH, scon_handle_t* endH)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(handle != SCON_NULL);
    SCON_ASSERT(startH != SCON_NULL);

    scon_item_t* item = scon_handle_item(scon, handle);
    scon_size_t start, end;
    scon_sequence_normalize_range(scon, startH, endH, item->length, &start, &end);

    if (start >= end)
    {
        return scon_handle_nil(scon);
    }

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
    {
        return SCON_HANDLE_FROM_LIST(scon_list_slice(scon, &item->list, start, end));
    }
    case SCON_ITEM_TYPE_ATOM:
    {
        return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, item->atom.string + start, end - start, SCON_ATOM_LOOKUP_NONE));
    }
    default:
        SCON_ERROR_RUNTIME(scon, "slice expected list or atom, got %s", scon_item_type_str(item->type));
    }
}

SCON_API scon_handle_t scon_flatten(struct scon* scon, scon_handle_t* handle, scon_handle_t* depthH)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (!SCON_HANDLE_IS_LIST(handle))
    {
        return *handle;
    }

    scon_int64_t depth = -1;
    if (depthH != SCON_NULL)
    {
        scon_handle_t d = scon_get_int(scon, depthH);
        depth = SCON_HANDLE_TO_INT(&d);
    }

    if (depth == 0)
    {
        return *handle;
    }

    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);
    scon_list_t* newList = scon_list_new(scon);
    scon_handle_t newHandle = SCON_HANDLE_FROM_LIST(newList);
    SCON_GC_RETAIN(scon, newHandle);

    scon_handle_t current;
    SCON_LIST_FOR_EACH(&current, &item->list)
    {
        if (SCON_HANDLE_IS_LIST(&current))
        {
            scon_handle_t nextDepthH = SCON_HANDLE_FROM_INT(depth - 1);
            scon_handle_t flattened = scon_flatten(scon, &current, &nextDepthH);
            SCON_GC_RETAIN(scon, flattened);

            if (SCON_HANDLE_IS_LIST(&flattened))
            {
                scon_list_append_list(scon, newList, &SCON_HANDLE_TO_ITEM(&flattened)->list);
            }
            else if (flattened != scon_handle_nil(scon))
            {
                scon_list_append(scon, newList, flattened);
            }
            SCON_GC_RELEASE(scon, flattened);
        }
        else
        {
            scon_list_append(scon, newList, current);
        }
    }

    SCON_GC_RELEASE(scon, newHandle);
    return newHandle;
}

SCON_API scon_handle_t scon_contains(struct scon* scon, scon_handle_t* handle, scon_handle_t* target)
{
    SCON_ASSERT(scon != SCON_NULL);
    scon_handle_t index = scon_index_of(scon, handle, target);
    return SCON_HANDLE_FROM_BOOL(index != scon_handle_nil(scon));
}

SCON_API scon_handle_t scon_replace(struct scon* scon, scon_handle_t* handle, scon_handle_t* oldVal,
    scon_handle_t* newVal)
{
    SCON_ASSERT(scon != SCON_NULL);
    scon_item_t* item = scon_handle_item(scon, handle);

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
    {
        scon_list_t* newList = scon_list_new(scon);
        scon_handle_t newHandle = SCON_HANDLE_FROM_LIST(newList);
        SCON_GC_RETAIN(scon, newHandle);

        scon_handle_t entry;
        SCON_LIST_FOR_EACH(&entry, &item->list)
        {
            if (scon_handle_compare(scon, &entry, oldVal) == 0)
            {
                scon_list_append(scon, newList, *newVal);
            }
            else
            {
                scon_list_append(scon, newList, entry);
            }
        }

        SCON_GC_RELEASE(scon, newHandle);
        return newHandle;
    }
    case SCON_ITEM_TYPE_ATOM:
    {
        char* oldStr;
        scon_size_t oldLen;
        scon_handle_get_string_params(scon, oldVal, &oldStr, &oldLen);

        char* newStr;
        scon_size_t newLen;
        scon_handle_get_string_params(scon, newVal, &newStr, &newLen);

        if (oldLen == 0)
        {
            return *handle;
        }

        scon_size_t count = 0;
        for (scon_size_t i = 0; i <= item->length - oldLen; i++)
        {
            if (SCON_MEMCMP(item->atom.string + i, oldStr, oldLen) == 0)
            {
                count++;
                i += oldLen - 1;
            }
        }

        if (count == 0)
        {
            return *handle;
        }

        scon_size_t resultLen = item->length + (count * (newLen - oldLen));
        char* buffer = (char*)SCON_MALLOC(resultLen);
        if (buffer == SCON_NULL)
        {
            SCON_ERROR_INTERNAL(scon, "out of memory");
        }

        scon_size_t currentPos = 0;
        for (scon_size_t i = 0; i < item->length;)
        {
            if (i <= item->length - oldLen && SCON_MEMCMP(item->atom.string + i, oldStr, oldLen) == 0)
            {
                SCON_MEMCPY(buffer + currentPos, newStr, newLen);
                currentPos += newLen;
                i += oldLen;
            }
            else
            {
                buffer[currentPos++] = item->atom.string[i++];
            }
        }

        scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, resultLen, SCON_ATOM_LOOKUP_NONE));
        SCON_FREE(buffer);
        return result;
    }
    default:
        SCON_ERROR_RUNTIME(scon, "replace expected list or atom, got %s", scon_item_type_str(item->type));
    }
}

SCON_API scon_handle_t scon_unique(struct scon* scon, scon_handle_t* handle)
{
    SCON_ASSERT(scon != SCON_NULL);
    if (!SCON_HANDLE_IS_LIST(handle))
    {
        return *handle;
    }

    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);
    scon_list_t* newList = scon_list_new(scon);
    scon_handle_t resultHandle = SCON_HANDLE_FROM_LIST(newList);
    SCON_GC_RETAIN(scon, resultHandle);

    scon_handle_t current;
    SCON_LIST_FOR_EACH(&current, &item->list)
    {
        scon_bool_t found = SCON_FALSE;
        scon_handle_t existing;
        SCON_LIST_FOR_EACH(&existing, newList)
        {
            if (scon_handle_compare(scon, &current, &existing) == 0)
            {
                found = SCON_TRUE;
                break;
            }
        }

        if (!found)
        {
            scon_list_append(scon, newList, current);
        }
    }

    SCON_GC_RELEASE(scon, resultHandle);
    return resultHandle;
}

SCON_API scon_handle_t scon_chunk(struct scon* scon, scon_handle_t* handle, scon_handle_t* sizeH)
{
    SCON_ASSERT(scon != SCON_NULL);
    if (!SCON_HANDLE_IS_LIST(handle))
    {
        return *handle;
    }

    scon_handle_t nHandle = scon_get_int(scon, sizeH);
    scon_int64_t n = SCON_HANDLE_TO_INT(&nHandle);

    if (n <= 0)
    {
        SCON_ERROR_RUNTIME(scon, "chunk size must be greater than 0");
    }

    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);
    scon_list_t* resultList = scon_list_new(scon);
    scon_handle_t resultHandle = SCON_HANDLE_FROM_LIST(resultList);
    SCON_GC_RETAIN(scon, resultHandle);

    scon_size_t chunkSize = (scon_size_t)n;
    for (scon_size_t i = 0; i < item->length; i += chunkSize)
    {
        scon_size_t end = SCON_MIN(i + chunkSize, item->length);
        scon_list_t* chunk = scon_list_slice(scon, &item->list, i, end);
        scon_list_append(scon, resultList, SCON_HANDLE_FROM_LIST(chunk));
    }

    SCON_GC_RELEASE(scon, resultHandle);
    return resultHandle;
}

SCON_API scon_handle_t scon_find(struct scon* scon, scon_handle_t* handle, scon_handle_t* callable)
{
    SCON_ASSERT(scon != SCON_NULL);
    if (!SCON_HANDLE_IS_LIST(handle))
    {
        return scon_handle_nil(scon);
    }

    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);
    scon_handle_t current;
    SCON_LIST_FOR_EACH(&current, &item->list)
    {
        scon_handle_t result = scon_eval_call(scon, *callable, 1, &current);
        if (SCON_HANDLE_IS_TRUTHY(&result))
        {
            return current;
        }
    }

    return scon_handle_nil(scon);
}

SCON_API scon_handle_t scon_get_in(scon_t* scon, scon_handle_t* list, scon_handle_t* path, scon_handle_t* defaultVal)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_t current = *list;
    scon_handle_t pathH = *path;

    if (!SCON_HANDLE_IS_LIST(&pathH))
    {
        scon_item_t* item = scon_handle_item(scon, &current);
        if (item->type != SCON_ITEM_TYPE_LIST)
        {
            return (defaultVal != SCON_NULL) ? *defaultVal : scon_handle_nil(scon);
        }

        scon_handle_t entryH = scon_list_find_entry(scon, item, &pathH);
        if (entryH != SCON_HANDLE_NONE)
        {
            scon_item_t* entry = SCON_HANDLE_TO_ITEM(&entryH);
            return (entry->length >= 2) ? scon_list_nth(scon, &entry->list, 1) : scon_handle_nil(scon);
        }
        return (defaultVal != SCON_NULL) ? *defaultVal : scon_handle_nil(scon);
    }
    scon_item_t* pathItem = SCON_HANDLE_TO_ITEM(&pathH);
    scon_handle_t key;
    SCON_LIST_FOR_EACH(&key, &pathItem->list)
    {
        current = scon_get_in(scon, &current, &key, SCON_NULL);
        if (current == scon_handle_nil(scon))
        {
            return (defaultVal != SCON_NULL) ? *defaultVal : scon_handle_nil(scon);
        }
    }

    return current;
}

SCON_API scon_handle_t scon_assoc_in(scon_t* scon, scon_handle_t* list, scon_handle_t* path, scon_handle_t* val)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (!SCON_HANDLE_IS_LIST(path))
    {
        scon_item_t* listItem = scon_handle_item(scon, list);
        scon_list_t* newList = scon_list_new(scon);
        scon_handle_t targetEntry = scon_list_find_entry(scon, listItem, path);

        scon_handle_t entryH;
        SCON_LIST_FOR_EACH(&entryH, &listItem->list)
        {
            if (entryH == targetEntry)
            {
                scon_list_t* newEntry = scon_list_new(scon);
                scon_list_append(scon, newEntry, *path);
                scon_list_append(scon, newEntry, *val);
                scon_list_append(scon, newList, SCON_HANDLE_FROM_LIST(newEntry));
                continue;
            }
            scon_list_append(scon, newList, entryH);
        }

        if (targetEntry == SCON_HANDLE_NONE)
        {
            scon_list_t* newEntry = scon_list_new(scon);
            scon_list_append(scon, newEntry, *path);
            scon_list_append(scon, newEntry, *val);
            scon_list_append(scon, newList, SCON_HANDLE_FROM_LIST(newEntry));
        }

        return SCON_HANDLE_FROM_LIST(newList);
    }

    scon_item_t* pathItem = SCON_HANDLE_TO_ITEM(path);
    if (pathItem->length == 0)
    {
        return *val;
    }

    scon_handle_t firstKey = scon_list_nth(scon, &pathItem->list, 0);
    scon_handle_t restPath;

    if (pathItem->length == 1)
    {
        restPath = firstKey;
        return scon_assoc_in(scon, list, &restPath, val);
    }
    else
    {
        restPath = SCON_HANDLE_FROM_LIST(scon_list_slice(scon, &pathItem->list, 1, pathItem->length));
        SCON_GC_RETAIN(scon, restPath);

        scon_handle_t subList = scon_get_in(scon, list, &firstKey, SCON_NULL);
        if (!SCON_HANDLE_IS_LIST(&subList))
        {
            subList = SCON_HANDLE_FROM_LIST(scon_list_new(scon));
        }

        scon_handle_t updatedSubList = scon_assoc_in(scon, &subList, &restPath, val);
        scon_handle_t result = scon_assoc_in(scon, list, &firstKey, &updatedSubList);

        SCON_GC_RELEASE(scon, restPath);
        return result;
    }
}

SCON_API scon_handle_t scon_dissoc_in(scon_t* scon, scon_handle_t* list, scon_handle_t* path)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (!SCON_HANDLE_IS_LIST(path))
    {
        scon_item_t* listItem = scon_handle_item(scon, list);
        scon_list_t* newList = scon_list_new(scon);
        scon_handle_t targetEntry = scon_list_find_entry(scon, listItem, path);

        scon_handle_t entryH;
        SCON_LIST_FOR_EACH(&entryH, &listItem->list)
        {
            if (entryH == targetEntry)
            {
                continue;
            }
            scon_list_append(scon, newList, entryH);
        }

        return SCON_HANDLE_FROM_LIST(newList);
    }
    scon_item_t* pathItem = SCON_HANDLE_TO_ITEM(path);
    if (pathItem->length == 0)
    {
        return *list;
    }

    scon_handle_t firstKey = scon_list_nth(scon, &pathItem->list, 0);
    if (pathItem->length == 1)
    {
        return scon_dissoc_in(scon, list, &firstKey);
    }
    else
    {
        scon_handle_t restPath = SCON_HANDLE_FROM_LIST(scon_list_slice(scon, &pathItem->list, 1, pathItem->length));
        SCON_GC_RETAIN(scon, restPath);

        scon_handle_t subList = scon_get_in(scon, list, &firstKey, SCON_NULL);
        if (SCON_HANDLE_IS_LIST(&subList))
        {
            scon_handle_t updatedSubList = scon_dissoc_in(scon, &subList, &restPath);
            scon_handle_t result = scon_assoc_in(scon, list, &firstKey, &updatedSubList);
            SCON_GC_RELEASE(scon, restPath);
            return result;
        }

        SCON_GC_RELEASE(scon, restPath);
        return *list;
    }
}

SCON_API scon_handle_t scon_update_in(scon_t* scon, scon_handle_t* list, scon_handle_t* path, scon_handle_t* callable)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_t currentVal = scon_get_in(scon, list, path, SCON_NULL);
    scon_handle_t newVal = scon_eval_call(scon, *callable, 1, &currentVal);

    return scon_assoc_in(scon, list, path, &newVal);
}

static inline scon_handle_t scon_list_project(scon_t* scon, scon_handle_t* listHandle, scon_size_t index,
    const char* name)
{
    SCON_ERROR_CHECK_LIST(scon, listHandle, name);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(listHandle);
    scon_list_t* resultList = scon_list_new(scon);
    scon_handle_t resultHandle = SCON_HANDLE_FROM_LIST(resultList);
    SCON_GC_RETAIN(scon, resultHandle);

    scon_handle_t childHandle;
    SCON_LIST_FOR_EACH(&childHandle, &item->list)
    {
        if (SCON_HANDLE_IS_LIST(&childHandle))
        {
            scon_item_t* childItem = SCON_HANDLE_TO_ITEM(&childHandle);
            if (childItem->length > index)
            {
                scon_list_append(scon, resultList, scon_list_nth(scon, &childItem->list, index));
            }
        }
    }

    SCON_GC_RELEASE(scon, resultHandle);
    return resultHandle;
}

SCON_API scon_handle_t scon_keys(scon_t* scon, scon_handle_t* listHandle)
{
    SCON_ASSERT(scon != SCON_NULL);
    return scon_list_project(scon, listHandle, 0, "keys");
}

SCON_API scon_handle_t scon_values(scon_t* scon, scon_handle_t* listHandle)
{
    SCON_ASSERT(scon != SCON_NULL);
    return scon_list_project(scon, listHandle, 1, "values");
}

SCON_API scon_handle_t scon_merge(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    scon_list_t* resultList = scon_list_new(scon);
    scon_handle_t result = SCON_HANDLE_FROM_LIST(resultList);
    SCON_GC_RETAIN(scon, result);

    for (scon_size_t i = 0; i < argc; i++)
    {
        if (!SCON_HANDLE_IS_LIST(&argv[i]))
        {
            continue;
        }

        scon_item_t* currentItem = SCON_HANDLE_TO_ITEM(&argv[i]);
        scon_handle_t entryH;
        SCON_LIST_FOR_EACH(&entryH, &currentItem->list)
        {
            scon_handle_t key, val;
            if (scon_list_get_entry(scon, &entryH, &key, &val))
            {
                scon_handle_t next = scon_assoc_in(scon, &result, &key, &val);
                SCON_GC_RETAIN(scon, next);
                SCON_GC_RELEASE(scon, result);
                result = next;
            }
        }
    }

    SCON_GC_RELEASE(scon, result);
    return result;
}

SCON_API scon_handle_t scon_explode(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    scon_size_t totalLen = 0;
    for (scon_size_t i = 0; i < argc; i++)
    {
        char* str;
        scon_size_t len;
        scon_handle_get_string_params(scon, &argv[i], &str, &len);
        totalLen += len;
    }

    scon_list_t* list = scon_list_new(scon);
    scon_handle_t listHandle = SCON_HANDLE_FROM_LIST(list);
    SCON_GC_RETAIN(scon, listHandle);

    for (scon_size_t i = 0; i < argc; i++)
    {
        char* str;
        scon_size_t len;
        scon_handle_get_string_params(scon, &argv[i], &str, &len);
        for (scon_size_t j = 0; j < len; j++)
        {
            scon_list_append(scon, list, SCON_HANDLE_FROM_INT((scon_int64_t)(unsigned char)str[j]));
        }
    }

    SCON_GC_RELEASE(scon, listHandle);
    return listHandle;
}

SCON_API scon_handle_t scon_implode(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    scon_size_t totalLen = 0;
    for (scon_size_t i = 0; i < argc; i++)
    {
        if (!SCON_HANDLE_IS_LIST(&argv[i]))
        {
            continue;
        }
        totalLen += SCON_HANDLE_TO_ITEM(&argv[i])->length;
    }

    SCON_SCRATCH_BUFFER(buffer, totalLen);

    scon_size_t currentPos = 0;
    for (scon_size_t i = 0; i < argc; i++)
    {
        if (!SCON_HANDLE_IS_LIST(&argv[i]))
        {
            continue;
        }
        scon_item_t* list = SCON_HANDLE_TO_ITEM(&argv[i]);
        scon_handle_t valH;
        SCON_LIST_FOR_EACH(&valH, &list->list)
        {
            scon_handle_t charH = scon_get_int(scon, &valH);
            buffer[currentPos++] = (char)SCON_HANDLE_TO_INT(&charH);
        }
    }

    scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, totalLen, SCON_ATOM_LOOKUP_NONE));
    SCON_SCRATCH_BUFFER_FREE(buffer);
    return result;
}

SCON_API scon_handle_t scon_repeat(scon_t* scon, scon_handle_t* handle, scon_handle_t* count)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_t nHandle = scon_get_int(scon, count);
    scon_int64_t n = SCON_HANDLE_TO_INT(&nHandle);

    if (n < 0)
    {
        SCON_ERROR_RUNTIME(scon, "repeat count cannot be negative, got %lld", n);
    }

    scon_size_t repeatCount = (scon_size_t)n;

    if (SCON_HANDLE_IS_LIST(handle))
    {
        scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);

        scon_list_t* newList = scon_list_new(scon);
        scon_handle_t newHandle = SCON_HANDLE_FROM_LIST(newList);
        SCON_GC_RETAIN(scon, newHandle);

        for (scon_size_t i = 0; i < repeatCount; i++)
        {
            scon_list_append_list(scon, newList, &item->list);
        }

        SCON_GC_RELEASE(scon, newHandle);
        return newHandle;
    }

    char* str;
    scon_size_t len;
    scon_handle_get_string_params(scon, handle, &str, &len);

    scon_size_t totalLen = len * repeatCount;
    SCON_SCRATCH_BUFFER(buffer, totalLen);

    for (scon_size_t i = 0; i < repeatCount; i++)
    {
        SCON_MEMCPY(buffer + (i * len), str, len);
    }

    scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, totalLen, SCON_ATOM_LOOKUP_NONE));
    SCON_SCRATCH_BUFFER_FREE(buffer);
    return result;
}

static inline scon_handle_t scon_sequence_check_edge(scon_t* scon, scon_handle_t* handle, scon_handle_t* target,
    scon_bool_t start, const char* name)
{
    SCON_ASSERT(scon != SCON_NULL);

    SCON_ERROR_CHECK_SEQUENCE(scon, handle, name);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);

    if (item->type == SCON_ITEM_TYPE_LIST)
    {
        if (item->length == 0)
        {
            return SCON_HANDLE_FALSE();
        }
        scon_size_t index = start ? 0 : item->length - 1;
        scon_handle_t edge = scon_list_nth(scon, &item->list, index);
        return (scon_handle_compare(scon, &edge, target) == 0) ? SCON_HANDLE_TRUE() : SCON_HANDLE_FALSE();
    }
    else
    {
        char *srcStr, *tgtStr;
        scon_size_t srcLen, tgtLen;
        scon_handle_get_string_params(scon, handle, &srcStr, &srcLen);
        scon_handle_get_string_params(scon, target, &tgtStr, &tgtLen);

        if (tgtLen > srcLen)
        {
            return SCON_HANDLE_FALSE();
        }
        const char* offset = start ? srcStr : srcStr + srcLen - tgtLen;
        return (SCON_MEMCMP(offset, tgtStr, tgtLen) == 0) ? SCON_HANDLE_TRUE() : SCON_HANDLE_FALSE();
    }
    return SCON_HANDLE_FALSE();
}

SCON_API scon_handle_t scon_starts_with(scon_t* scon, scon_handle_t* handle, scon_handle_t* prefix)
{
    return scon_sequence_check_edge(scon, handle, prefix, SCON_TRUE, "starts-with?");
}

SCON_API scon_handle_t scon_ends_with(scon_t* scon, scon_handle_t* handle, scon_handle_t* suffix)
{
    return scon_sequence_check_edge(scon, handle, suffix, SCON_FALSE, "ends-with?");
}

SCON_API scon_handle_t scon_join(scon_t* scon, scon_handle_t* listHandle, scon_handle_t* sepHandle)
{
    SCON_ASSERT(scon != SCON_NULL);
    if (!SCON_HANDLE_IS_LIST(listHandle))
    {
        SCON_ERROR_RUNTIME(scon, "join expects a list as the first argument, got %s",
            scon_item_type_str(SCON_HANDLE_GET_TYPE(listHandle)));
    }

    scon_item_t* list = SCON_HANDLE_TO_ITEM(listHandle);
    if (list->length == 0)
    {
        return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, "", 0, SCON_ATOM_LOOKUP_NONE));
    }

    char* sepStr;
    scon_size_t sepLen;
    scon_handle_get_string_params(scon, sepHandle, &sepStr, &sepLen);

    scon_size_t totalLen = 0;
    scon_handle_t entry;
    SCON_LIST_FOR_EACH(&entry, &list->list)
    {
        totalLen += scon_handle_item(scon, &entry)->length;
    }

    if (list->length > 1)
    {
        totalLen += sepLen * (list->length - 1);
    }

    SCON_SCRATCH_BUFFER(buffer, totalLen);

    scon_size_t currentPos = 0;
    scon_size_t i = 0;
    SCON_LIST_FOR_EACH(&entry, &list->list)
    {
        char* str;
        scon_size_t len;
        scon_handle_get_string_params(scon, &entry, &str, &len);
        SCON_MEMCPY(buffer + currentPos, str, len);
        currentPos += len;

        if (i < list->length - 1)
        {
            SCON_MEMCPY(buffer + currentPos, sepStr, sepLen);
            currentPos += sepLen;
        }
        i++;
    }

    scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, totalLen, SCON_ATOM_LOOKUP_NONE));
    SCON_SCRATCH_BUFFER_FREE(buffer);
    return result;
}

SCON_API scon_handle_t scon_split(scon_t* scon, scon_handle_t* srcHandle, scon_handle_t* sepHandle)
{
    SCON_ASSERT(scon != SCON_NULL);

    char *srcStr, *sepStr;
    scon_size_t srcLen, sepLen;

    scon_handle_get_string_params(scon, srcHandle, &srcStr, &srcLen);
    scon_handle_get_string_params(scon, sepHandle, &sepStr, &sepLen);

    scon_list_t* resultList = scon_list_new(scon);
    scon_handle_t resultHandle = SCON_HANDLE_FROM_LIST(resultList);
    SCON_GC_RETAIN(scon, resultHandle);

    if (sepLen == 0)
    {
        for (scon_size_t i = 0; i < srcLen; i++)
        {
            scon_list_append(scon, resultList,
                SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, srcStr + i, 1, SCON_ATOM_LOOKUP_NONE)));
        }
    }
    else
    {
        scon_size_t lastPos = 0;
        for (scon_size_t i = 0; i <= srcLen - sepLen; i++)
        {
            if (SCON_MEMCMP(srcStr + i, sepStr, sepLen) == 0)
            {
                scon_list_append(scon, resultList,
                    SCON_HANDLE_FROM_ATOM(
                        scon_atom_lookup(scon, srcStr + lastPos, i - lastPos, SCON_ATOM_LOOKUP_NONE)));
                i += sepLen - 1;
                lastPos = i + 1;
            }
        }

        scon_list_append(scon, resultList,
            SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, srcStr + lastPos, srcLen - lastPos, SCON_ATOM_LOOKUP_NONE)));
    }

    SCON_GC_RELEASE(scon, resultHandle);
    return resultHandle;
}

static inline scon_handle_t scon_string_transform(scon_t* scon, scon_handle_t* srcHandle, scon_bool_t upper)
{
    char* srcStr;
    scon_size_t srcLen;
    scon_handle_get_string_params(scon, srcHandle, &srcStr, &srcLen);

    if (srcLen == 0)
    {
        return *srcHandle;
    }

    SCON_SCRATCH_BUFFER(buffer, srcLen);
    for (scon_size_t i = 0; i < srcLen; i++)
    {
        buffer[i] = upper ? SCON_CHAR_TO_UPPER(srcStr[i]) : SCON_CHAR_TO_LOWER(srcStr[i]);
    }

    scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, srcLen, SCON_ATOM_LOOKUP_NONE));
    SCON_SCRATCH_BUFFER_FREE(buffer);
    return result;
}

SCON_API scon_handle_t scon_upper(scon_t* scon, scon_handle_t* srcHandle)
{
    SCON_ASSERT(scon != SCON_NULL);
    return scon_string_transform(scon, srcHandle, SCON_TRUE);
}

SCON_API scon_handle_t scon_lower(scon_t* scon, scon_handle_t* srcHandle)
{
    SCON_ASSERT(scon != SCON_NULL);
    return scon_string_transform(scon, srcHandle, SCON_FALSE);
}

SCON_API scon_handle_t scon_trim(scon_t* scon, scon_handle_t* srcHandle)
{
    SCON_ASSERT(scon != SCON_NULL);

    char* srcStr;
    scon_size_t srcLen;
    scon_handle_get_string_params(scon, srcHandle, &srcStr, &srcLen);

    if (srcLen == 0)
    {
        return *srcHandle;
    }

    scon_size_t start = 0;
    while (start < srcLen && SCON_CHAR_IS_WHITESPACE(srcStr[start]))
    {
        start++;
    }

    if (start == srcLen)
    {
        return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, "", 0, SCON_ATOM_LOOKUP_NONE));
    }

    scon_size_t end = srcLen - 1;
    while (end > start && SCON_CHAR_IS_WHITESPACE(srcStr[end]))
    {
        end--;
    }

    return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, srcStr + start, end - start + 1, SCON_ATOM_LOOKUP_NONE));
}

#define SCON_INTROSPECTION_LOOP(_predicate) \
    do \
    { \
        SCON_ASSERT(scon != SCON_NULL); \
        SCON_ASSERT(argv != SCON_NULL || argc == 0); \
        for (scon_size_t i = 0; i < argc; i++) \
        { \
            if (!(_predicate)) \
            { \
                return SCON_HANDLE_FALSE(); \
            } \
        } \
        return SCON_HANDLE_TRUE(); \
    } while (0)

#define SCON_INTROSPECTION_IMPL(_name, _predicate_macro) \
    SCON_API scon_handle_t _name(scon_t* scon, scon_size_t argc, scon_handle_t* argv) \
    { \
        SCON_INTROSPECTION_LOOP(_predicate_macro(&argv[i])); \
    }

SCON_INTROSPECTION_IMPL(scon_is_atom, SCON_HANDLE_IS_ATOM)
SCON_INTROSPECTION_IMPL(scon_is_int, SCON_HANDLE_IS_INT_SHAPED)
SCON_INTROSPECTION_IMPL(scon_is_float, SCON_HANDLE_IS_FLOAT_SHAPED)
SCON_INTROSPECTION_IMPL(scon_is_number, SCON_HANDLE_IS_NUMBER)
SCON_INTROSPECTION_IMPL(scon_is_lambda, SCON_HANDLE_IS_LAMBDA)
SCON_INTROSPECTION_IMPL(scon_is_native, SCON_HANDLE_IS_NATIVE)
SCON_INTROSPECTION_IMPL(scon_is_callable, SCON_HANDLE_IS_CALLABLE)
SCON_INTROSPECTION_IMPL(scon_is_list, SCON_HANDLE_IS_LIST)

#define SCON_PREDICATE_IS_STRING(_h) (SCON_HANDLE_IS_ATOM(_h) && !SCON_HANDLE_IS_NUMBER(_h))
SCON_INTROSPECTION_IMPL(scon_is_string, SCON_PREDICATE_IS_STRING)

#define SCON_PREDICATE_IS_EMPTY(_h) (scon_handle_item(scon, _h)->length == 0)
SCON_INTROSPECTION_IMPL(scon_is_empty, SCON_PREDICATE_IS_EMPTY)

#define SCON_PREDICATE_IS_NIL(_h) (*(_h) == scon_handle_nil(scon))
SCON_INTROSPECTION_IMPL(scon_is_nil, SCON_PREDICATE_IS_NIL)

static inline scon_item_t* scon_get_numeric_item(scon_t* scon, scon_handle_t* handle)
{
    scon_item_t* item = scon_handle_item(scon, handle);
    if (item->flags & (SCON_ITEM_FLAG_INT_SHAPED | SCON_ITEM_FLAG_FLOAT_SHAPED))
    {
        return item;
    }

    if (item->flags & SCON_ITEM_FLAG_QUOTED)
    {
        scon_atom_t* atom = scon_atom_lookup(scon, item->atom.string, item->length, SCON_ATOM_LOOKUP_NONE);
        if (atom != SCON_NULL)
        {
            scon_item_t* inner = SCON_CONTAINER_OF(atom, scon_item_t, atom);
            if (inner->flags & (SCON_ITEM_FLAG_INT_SHAPED | SCON_ITEM_FLAG_FLOAT_SHAPED))
            {
                return inner;
            }
        }
    }
    return item;
}

#define SCON_GET_NUMERIC_IMPL(_name, _res, _type, _other, _targetUnion, _otherUnion) \
SCON_API scon_handle_t scon_get_##_name(scon_t* scon, scon_handle_t* handle) \
{ \
    SCON_ASSERT(scon != SCON_NULL); \
    if (SCON_HANDLE_IS_##_type(handle)) \
    { \
        return *handle; \
    } \
    if (SCON_HANDLE_IS_##_other(handle)) \
    { \
        return SCON_HANDLE_FROM_##_res((scon_##_name##_t)SCON_HANDLE_TO_##_other(handle)); \
    } \
    scon_item_t* item = scon_get_numeric_item(scon, handle); \
    if (item->flags & SCON_ITEM_FLAG_##_type##_SHAPED) \
    { \
        return SCON_HANDLE_FROM_##_res(item->atom._targetUnion); \
    } \
    if (item->flags & SCON_ITEM_FLAG_##_other##_SHAPED) \
    { \
        return SCON_HANDLE_FROM_##_res((scon_##_name##_t)item->atom._otherUnion); \
    } \
    SCON_ERROR_RUNTIME(scon, "expected " #_name ", got %s", scon_item_type_str(item->type)); \
}

SCON_GET_NUMERIC_IMPL(int, INT, INT, FLOAT, integerValue, floatValue)
SCON_GET_NUMERIC_IMPL(float, FLOAT, FLOAT, INT, floatValue, integerValue)

SCON_API scon_handle_t scon_get_string(scon_t* scon, scon_handle_t* handle)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (SCON_HANDLE_IS_ATOM(handle))
    {
        scon_item_t* item = scon_handle_item(scon, handle);
        return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, item->atom.string, item->length, SCON_ATOM_LOOKUP_QUOTED));
    }

    char stackBuffer[SCON_STACK_BUFFER_SIZE];
    scon_size_t len = scon_stringify(scon, handle, stackBuffer, SCON_STACK_BUFFER_SIZE);
    if (len < SCON_STACK_BUFFER_SIZE)
    {
        return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, stackBuffer, len, SCON_ATOM_LOOKUP_QUOTED));
    }

    SCON_SCRATCH_BUFFER(buffer, len);
    scon_stringify(scon, handle, buffer, len + 1);
    scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, len, SCON_ATOM_LOOKUP_QUOTED));
    SCON_SCRATCH_BUFFER_FREE(buffer);
    return result;
}

static void scon_path_copy(scon_t* scon, char* dest, const char* src, scon_size_t len, scon_size_t max)
{
    if (len >= max)
    {
        SCON_ERROR_RUNTIME(scon, "path too long");
    }
    SCON_MEMCPY(dest, src, len);
    dest[len] = '\0';
}

static void scon_resolve_path(scon_t* scon, const char* path, scon_size_t pathLen, char* outPath, scon_size_t maxLen)
{
    scon_bool_t isAbsolute = SCON_FALSE;
    if (pathLen > 0 && (path[0] == '/' || path[0] == '\\'))
    {
        isAbsolute = SCON_TRUE;
    }
    if (pathLen > 1 && ((path[0] >= 'a' && path[0] <= 'z') || (path[0] >= 'A' && path[0] <= 'Z')) && path[1] == ':')
    {
        isAbsolute = SCON_TRUE;
    }

    if (isAbsolute || scon->evalState == SCON_NULL || scon->evalState->frameCount == 0)
    {
        scon_path_copy(scon, outPath, path, pathLen, maxLen);
        return;
    }

    scon_input_t* input = SCON_NULL;
    for (scon_size_t i = scon->evalState->frameCount; i > 0; i--)
    {
        scon_eval_frame_t* frame = &scon->evalState->frames[i - 1];
        scon_item_t* funcItem = SCON_CONTAINER_OF(frame->closure->function, scon_item_t, function);
        if (funcItem->input != SCON_NULL && funcItem->input->path[0] != '\0')
        {
            input = funcItem->input;
            break;
        }
    }

    if (input == SCON_NULL)
    {
        scon_path_copy(scon, outPath, path, pathLen, maxLen);
        return;
    }

    const char* lastSlash = NULL;
    const char* p = input->path;
    while (*p != '\0')
    {
        if (*p == '/' || *p == '\\')
        {
            lastSlash = p;
        }
        p++;
    }

    if (lastSlash == SCON_NULL)
    {
        scon_path_copy(scon, outPath, path, pathLen, maxLen);
        return;
    }

    scon_size_t dirLen = (scon_size_t)(lastSlash - input->path) + 1;
    if (dirLen + pathLen >= maxLen)
    {
        SCON_ERROR_RUNTIME(scon, "path too long");
    }

    SCON_MEMCPY(outPath, input->path, dirLen);
    SCON_MEMCPY(outPath + dirLen, path, pathLen);
    outPath[dirLen + pathLen] = '\0';
}

static void scon_get_resolved_path(scon_t* scon, scon_handle_t* pathHandle, char* outBuf)
{
    char* pathStr;
    scon_size_t pathLen;
    scon_handle_get_string_params(scon, pathHandle, &pathStr, &pathLen);
    scon_resolve_path(scon, pathStr, pathLen, outBuf, SCON_PATH_MAX);
}

SCON_API scon_handle_t scon_load(struct scon* scon, scon_handle_t* path)
{
    SCON_ASSERT(scon != SCON_NULL);
    char pathBuf[SCON_PATH_MAX];
    scon_get_resolved_path(scon, path, pathBuf);

    scon_handle_t ast = scon_parse_file(scon, pathBuf);
    scon_function_t* function = scon_compile(scon, &ast);
    return scon_eval(scon, function);
}

SCON_API scon_handle_t scon_read_file(struct scon* scon, scon_handle_t* path)
{
    SCON_ASSERT(scon != SCON_NULL);
    char pathBuf[SCON_PATH_MAX];
    scon_get_resolved_path(scon, path, pathBuf);

    scon_file_t file = SCON_FOPEN(pathBuf, "rb");
    if (file == SCON_NULL)
    {
        SCON_ERROR_RUNTIME(scon, "could not open file '%s'", pathBuf);
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)SCON_MALLOC(size);
    if (buffer == SCON_NULL)
    {
        SCON_FCLOSE(file);
        SCON_ERROR_INTERNAL(scon, "out of memory");
    }

    if (SCON_FREAD(buffer, 1, size, file) != (scon_size_t)size)
    {
        SCON_FREE(buffer);
        SCON_FCLOSE(file);
        SCON_ERROR_RUNTIME(scon, "could not read file '%s'", pathBuf);
    }

    SCON_FCLOSE(file);
    scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, size, SCON_ATOM_LOOKUP_QUOTED));
    SCON_FREE(buffer);
    return result;
}

SCON_API scon_handle_t scon_write_file(struct scon* scon, scon_handle_t* path, scon_handle_t* content)
{
    SCON_ASSERT(scon != SCON_NULL);

    char pathBuf[SCON_PATH_MAX];
    scon_get_resolved_path(scon, path, pathBuf);

    char* contentStr;
    scon_size_t contentLen;
    scon_handle_get_string_params(scon, content, &contentStr, &contentLen);

    scon_file_t file = SCON_FOPEN(pathBuf, "wb");
    if (file == SCON_NULL)
    {
        SCON_ERROR_RUNTIME(scon, "could not open file '%s' for writing", pathBuf);
    }

    if (SCON_FWRITE(contentStr, 1, contentLen, file) != contentLen)
    {
        SCON_FCLOSE(file);
        SCON_ERROR_RUNTIME(scon, "could not write to file '%s'", pathBuf);
    }

    SCON_FCLOSE(file);
    return *content;
}

SCON_API scon_handle_t scon_read_char(struct scon* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    int c = SCON_FGETC(SCON_STDIN);
    if (c == EOF)
    {
        return scon_handle_nil(scon);
    }

    char ch = (char)c;
    return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, &ch, 1, SCON_ATOM_LOOKUP_NONE));
}

SCON_API scon_handle_t scon_read_line(struct scon* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    char stackBuffer[SCON_STACK_BUFFER_SIZE];
    scon_size_t capacity = SCON_STACK_BUFFER_SIZE;
    char* buffer = stackBuffer;
    scon_size_t length = 0;

    while (SCON_TRUE)
    {
        int c = SCON_FGETC(SCON_STDIN);
        if (c == EOF || c == '\n')
        {
            if (c == EOF && length == 0)
            {
                if (buffer != stackBuffer)
                {
                    SCON_FREE(buffer);
                }
                return scon_handle_nil(scon);
            }
            break;
        }

        if (length + 1 >= capacity)
        {
            scon_size_t newCapacity = capacity * 2;
            char* newBuffer = (char*)SCON_MALLOC(newCapacity);
            if (newBuffer == SCON_NULL)
            {
                if (buffer != stackBuffer)
                {
                    SCON_FREE(buffer);
                }
                SCON_ERROR_INTERNAL(scon, "out of memory");
            }
            SCON_MEMCPY(newBuffer, buffer, length);
            if (buffer != stackBuffer)
            {
                SCON_FREE(buffer);
            }
            buffer = newBuffer;
            capacity = newCapacity;
        }
        buffer[length++] = (char)c;
    }

    scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, length, SCON_ATOM_LOOKUP_QUOTED));
    if (buffer != stackBuffer)
    {
        SCON_FREE(buffer);
    }
    return result;
}

SCON_API scon_handle_t scon_print(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);

    for (scon_size_t i = 0; i < argc; i++)
    {
        char* str;
        scon_size_t len;
        scon_handle_get_string_params(scon, &argv[i], &str, &len);
        SCON_FWRITE(str, 1, len, SCON_STDOUT);
        if (i < argc - 1)
        {
            SCON_FWRITE(" ", 1, 1, SCON_STDOUT);
        }
    }
    return scon_handle_nil(scon);
}

SCON_API scon_handle_t scon_println(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_t handle = scon_print(scon, argc, argv);
    SCON_FWRITE("\n", 1, 1, SCON_STDOUT);
    return handle;
}

SCON_API scon_handle_t scon_ord(struct scon* scon, scon_handle_t* handle)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(handle != SCON_NULL);

    char* str;
    scon_size_t len;
    scon_handle_get_string_params(scon, handle, &str, &len);

    if (len == 0)
    {
        SCON_ERROR_RUNTIME(scon, "ord expects a non-empty atom");
    }

    return SCON_HANDLE_FROM_INT((scon_int64_t)(scon_uint8_t)str[0]);
}

SCON_API scon_handle_t scon_chr(struct scon* scon, scon_handle_t* handle)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_t iVal = scon_get_int(scon, handle);
    scon_int64_t val = SCON_HANDLE_TO_INT(&iVal);

    if (val < 0 || val > 255)
    {
        SCON_ERROR_RUNTIME(scon, "chr expects an integer in range 0-255, got %lld", (long long)val);
    }

    char c = (char)(scon_uint8_t)val;
    return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, &c, 1, SCON_ATOM_LOOKUP_NONE));
}

SCON_API scon_handle_t scon_format(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (argc == 0)
    {
        return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, "", 0, SCON_ATOM_LOOKUP_NONE));
    }

    char* fmtStr;
    scon_size_t fmtLen;
    scon_handle_get_string_params(scon, &argv[0], &fmtStr, &fmtLen);

    scon_size_t totalLen = 0;
    scon_size_t argIndex = 1;

    for (scon_size_t i = 0; i < fmtLen; i++)
    {
        if (fmtStr[i] == '{')
        {
            if (i + 1 < fmtLen && fmtStr[i + 1] == '{')
            {
                totalLen++;
                i++;
                continue;
            }

            scon_size_t j = i + 1;
            scon_int64_t explicitIndex = -1;
            if (j < fmtLen && fmtStr[j] >= '0' && fmtStr[j] <= '9')
            {
                explicitIndex = 0;
                while (j < fmtLen && fmtStr[j] >= '0' && fmtStr[j] <= '9')
                {
                    explicitIndex = explicitIndex * 10 + (fmtStr[j] - '0');
                    j++;
                }
            }

            if (j < fmtLen && fmtStr[j] == '}')
            {
                scon_size_t idx = (explicitIndex != -1) ? (scon_size_t)explicitIndex + 1 : argIndex++;
                if (idx >= argc)
                {
                    SCON_ERROR_RUNTIME(scon, "format index out of range");
                }
                totalLen += scon_stringify(scon, &argv[idx], SCON_NULL, 0);
                i = j;
                continue;
            }

            SCON_ERROR_RUNTIME(scon, "invalid format string");
        }
        else if (fmtStr[i] == '}')
        {
            if (i + 1 < fmtLen && fmtStr[i + 1] == '}')
            {
                totalLen++;
                i++;
                continue;
            }

            SCON_ERROR_RUNTIME(scon, "single '}' encountered in format string");
        }
        totalLen++;
    }

    SCON_SCRATCH_BUFFER(buffer, totalLen);

    scon_size_t currentPos = 0;
    argIndex = 1;

    for (scon_size_t i = 0; i < fmtLen; i++)
    {
        if (fmtStr[i] == '{')
        {
            if (i + 1 < fmtLen && fmtStr[i + 1] == '{')
            {
                buffer[currentPos++] = '{';
                i++;
                continue;
            }

            scon_size_t j = i + 1;
            scon_int64_t explicitIndex = -1;
            if (j < fmtLen && fmtStr[j] >= '0' && fmtStr[j] <= '9')
            {
                explicitIndex = 0;
                while (j < fmtLen && fmtStr[j] >= '0' && fmtStr[j] <= '9')
                {
                    explicitIndex = explicitIndex * 10 + (fmtStr[j] - '0');
                    j++;
                }
            }

            if (j < fmtLen && fmtStr[j] == '}')
            {
                scon_size_t idx = (explicitIndex != -1) ? (scon_size_t)explicitIndex + 1 : argIndex++;
                currentPos += scon_stringify(scon, &argv[idx], buffer + currentPos, totalLen - currentPos + 1);
                i = j;
                continue;
            }
        }
        else if (fmtStr[i] == '}')
        {
            if (i + 1 < fmtLen && fmtStr[i + 1] == '}')
            {
                buffer[currentPos++] = '}';
                i++;
                continue;
            }
        }
        buffer[currentPos++] = fmtStr[i];
    }

    scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, totalLen, SCON_ATOM_LOOKUP_NONE));
    SCON_SCRATCH_BUFFER_FREE(buffer);
    return result;
}

SCON_API scon_handle_t scon_now(scon_t* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_TIME());
}

SCON_API scon_handle_t scon_uptime(scon_t* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_CLOCK());
}

SCON_API scon_handle_t scon_env(struct scon* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    extern char** environ;
    scon_size_t count = 0;
    while (environ[count] != SCON_NULL)
    {
        count++;
    }

    scon_list_t* list = scon_list_new(scon);
    for (scon_size_t i = 0; i < count; i++)
    {
        char* env = environ[i];
        char* eq = strchr(env, '=');
        if (eq != SCON_NULL)
        {
            scon_list_t* pair = scon_list_new(scon);
            scon_list_append(scon, pair,
                SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, env, (scon_size_t)(eq - env), SCON_ATOM_LOOKUP_NONE)));
            scon_list_append(scon, pair,
                SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, eq + 1, SCON_STRLEN(eq + 1), SCON_ATOM_LOOKUP_QUOTED)));

            scon_list_append(scon, list, SCON_HANDLE_FROM_LIST(pair));
        }
    }

    return SCON_HANDLE_FROM_LIST(list);
}

SCON_API scon_handle_t scon_args(struct scon* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (scon->argc == 0)
    {
        return scon_handle_nil(scon);
    }

    scon_list_t* list = scon_list_new(scon);
    for (scon_size_t i = 0; i < scon->argc; i++)
    {
        scon_list_append(scon, list,
            SCON_HANDLE_FROM_ATOM(
                scon_atom_lookup(scon, scon->argv[i], SCON_STRLEN(scon->argv[i]), SCON_ATOM_LOOKUP_QUOTED)));
    }

    return SCON_HANDLE_FROM_LIST(list);
}

#define SCON_MATH_MIN_MAX_IMPL(_name, _op) \
    SCON_API scon_handle_t _name(scon_t* scon, scon_size_t argc, scon_handle_t* argv) \
    { \
        SCON_ASSERT(scon != SCON_NULL); \
        if (argc == 0) \
        { \
            return scon_handle_nil(scon); \
        } \
        scon_handle_t current = argv[0]; \
        for (scon_size_t i = 1; i < argc; i++) \
        { \
            scon_promotion_t prom; \
            scon_handle_promote(scon, &current, &argv[i], &prom); \
            if (prom.type == SCON_PROMOTION_TYPE_INT) \
            { \
                current = SCON_HANDLE_FROM_INT(prom.a.intVal _op prom.b.intVal ? prom.a.intVal : prom.b.intVal); \
            } \
            else \
            { \
                current = \
                    SCON_HANDLE_FROM_FLOAT(prom.a.floatVal _op prom.b.floatVal ? prom.a.floatVal : prom.b.floatVal); \
            } \
        } \
        return current; \
    }

SCON_MATH_MIN_MAX_IMPL(scon_min, <)
SCON_MATH_MIN_MAX_IMPL(scon_max, >)

SCON_API scon_handle_t scon_clamp(scon_t* scon, scon_handle_t* val, scon_handle_t* minVal, scon_handle_t* maxVal)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_t current = *val;
    scon_promotion_t prom;

    scon_handle_promote(scon, &current, minVal, &prom);
    if (prom.type == SCON_PROMOTION_TYPE_INT)
    {
        current = SCON_HANDLE_FROM_INT(prom.a.intVal < prom.b.intVal ? prom.b.intVal : prom.a.intVal);
    }
    else
    {
        current = SCON_HANDLE_FROM_FLOAT(prom.a.floatVal < prom.b.floatVal ? prom.b.floatVal : prom.a.floatVal);
    }

    scon_handle_promote(scon, &current, maxVal, &prom);
    if (prom.type == SCON_PROMOTION_TYPE_INT)
    {
        current = SCON_HANDLE_FROM_INT(prom.a.intVal > prom.b.intVal ? prom.b.intVal : prom.a.intVal);
    }
    else
    {
        current = SCON_HANDLE_FROM_FLOAT(prom.a.floatVal > prom.b.floatVal ? prom.b.floatVal : prom.a.floatVal);
    }

    return current;
}

#define SCON_MATH_UNARY_IMPL(_name, _int_func, _float_func) \
    SCON_API scon_handle_t _name(scon_t* scon, scon_handle_t* val) \
    { \
        SCON_ASSERT(scon != SCON_NULL); \
        if (SCON_HANDLE_IS_INT_SHAPED(val)) \
        { \
            scon_handle_t iVal = scon_get_int(scon, val); \
            scon_int64_t i = SCON_HANDLE_TO_INT(&iVal); \
            return SCON_HANDLE_FROM_INT((scon_int64_t)_int_func(i)); \
        } \
        scon_handle_t floatVal = scon_get_float(scon, val); \
        scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal); \
        return SCON_HANDLE_FROM_FLOAT((scon_float_t)_float_func(f)); \
    }

#define SCON_INT_ABS(_x) ((_x) < 0 ? -(_x) : (_x))
SCON_MATH_UNARY_IMPL(scon_abs, SCON_INT_ABS, SCON_FABS)
SCON_MATH_UNARY_IMPL(scon_exp, SCON_EXP, SCON_EXP)
SCON_MATH_UNARY_IMPL(scon_sqrt, SCON_SQRT, SCON_SQRT)

#define SCON_MATH_UNARY_TO_INT_IMPL(_name, _float_func) \
    SCON_API scon_handle_t _name(struct scon* scon, scon_handle_t* val) \
    { \
        SCON_ASSERT(scon != SCON_NULL); \
        if (SCON_HANDLE_IS_INT_SHAPED(val)) \
        { \
            return scon_get_int(scon, val); \
        } \
        scon_handle_t floatVal = scon_get_float(scon, val); \
        scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal); \
        return SCON_HANDLE_FROM_INT((scon_int64_t)_float_func(f)); \
    }

SCON_MATH_UNARY_TO_INT_IMPL(scon_floor, SCON_FLOOR)
SCON_MATH_UNARY_TO_INT_IMPL(scon_ceil, SCON_CEIL)
SCON_MATH_UNARY_TO_INT_IMPL(scon_round, SCON_ROUND)

SCON_API scon_handle_t scon_pow(scon_t* scon, scon_handle_t* base, scon_handle_t* exp)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_promotion_t prom;
    scon_handle_promote(scon, base, exp, &prom);

    if (prom.type == SCON_PROMOTION_TYPE_INT)
    {
        return SCON_HANDLE_FROM_INT((scon_int64_t)SCON_POW((scon_float_t)prom.a.intVal, (scon_float_t)prom.b.intVal));
    }
    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_POW(prom.a.floatVal, prom.b.floatVal));
}

SCON_API scon_handle_t scon_log(struct scon* scon, scon_handle_t* val, scon_handle_t* base)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (base == SCON_NULL)
    {
        if (SCON_HANDLE_IS_INT_SHAPED(val))
        {
            scon_handle_t iVal = scon_get_int(scon, val);
            scon_int64_t i = SCON_HANDLE_TO_INT(&iVal);
            return SCON_HANDLE_FROM_INT((scon_int64_t)SCON_LOG(i));
        }

        scon_handle_t floatVal = scon_get_float(scon, val);
        scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal);
        return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_LOG(f));
    }

    scon_promotion_t prom;
    scon_handle_promote(scon, val, base, &prom);

    if (prom.type == SCON_PROMOTION_TYPE_INT)
    {
        scon_float_t res = SCON_LOG((scon_float_t)prom.a.intVal) / SCON_LOG((scon_float_t)prom.b.intVal);
        return SCON_HANDLE_FROM_INT((scon_int64_t)res);
    }
    scon_float_t res = SCON_LOG(prom.a.floatVal) / SCON_LOG(prom.b.floatVal);
    return SCON_HANDLE_FROM_FLOAT((scon_float_t)res);
}

#define SCON_MATH_UNARY_FLOAT_IMPL(_name, _func) \
    SCON_API scon_handle_t _name(struct scon* scon, scon_handle_t* val) \
    { \
        SCON_ASSERT(scon != SCON_NULL); \
        scon_handle_t fv = scon_get_float(scon, val); \
        return SCON_HANDLE_FROM_FLOAT((scon_float_t)_func(SCON_HANDLE_TO_FLOAT(&fv))); \
    }

SCON_MATH_UNARY_FLOAT_IMPL(scon_sin, SCON_SIN)
SCON_MATH_UNARY_FLOAT_IMPL(scon_cos, SCON_COS)
SCON_MATH_UNARY_FLOAT_IMPL(scon_tan, SCON_TAN)
SCON_MATH_UNARY_FLOAT_IMPL(scon_asin, SCON_ASIN)
SCON_MATH_UNARY_FLOAT_IMPL(scon_acos, SCON_ACOS)
SCON_MATH_UNARY_FLOAT_IMPL(scon_atan, SCON_ATAN)
SCON_MATH_UNARY_FLOAT_IMPL(scon_sinh, SCON_SINH)
SCON_MATH_UNARY_FLOAT_IMPL(scon_cosh, SCON_COSH)
SCON_MATH_UNARY_FLOAT_IMPL(scon_tanh, SCON_TANH)
SCON_MATH_UNARY_FLOAT_IMPL(scon_asinh, SCON_ASINH)
SCON_MATH_UNARY_FLOAT_IMPL(scon_acosh, SCON_ACOSH)
SCON_MATH_UNARY_FLOAT_IMPL(scon_atanh, SCON_ATANH)

SCON_API scon_handle_t scon_atan2(struct scon* scon, scon_handle_t* y, scon_handle_t* x)
{
    SCON_ASSERT(scon != SCON_NULL);
    scon_handle_t yFloatVal = scon_get_float(scon, y);
    scon_handle_t xFloatVal = scon_get_float(scon, x);
    scon_float_t yf = SCON_HANDLE_TO_FLOAT(&yFloatVal);
    scon_float_t xf = SCON_HANDLE_TO_FLOAT(&xFloatVal);
    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_ATAN2(yf, xf));
}

SCON_API scon_handle_t scon_rand(struct scon* scon, scon_handle_t* minVal, scon_handle_t* maxVal)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_promotion_t prom;
    scon_handle_promote(scon, minVal, maxVal, &prom);

    scon_float_t r = (scon_float_t)SCON_RAND() / (scon_float_t)SCON_RAND_MAX;

    if (prom.type == SCON_PROMOTION_TYPE_INT)
    {
        scon_int64_t res = prom.a.intVal + (scon_int64_t)(r * (prom.b.intVal - prom.a.intVal));
        return SCON_HANDLE_FROM_INT(res);
    }
    scon_float_t res = prom.a.floatVal + (r * (prom.b.floatVal - prom.a.floatVal));
    return SCON_HANDLE_FROM_FLOAT(res);
}

SCON_API scon_handle_t scon_seed(struct scon* scon, scon_handle_t* val)
{
    SCON_ASSERT(scon != SCON_NULL);
    scon_handle_t iVal = scon_get_int(scon, val);
    scon_int64_t i = SCON_HANDLE_TO_INT(&iVal);
    SCON_SRAND((unsigned int)i);
    return scon_handle_nil(scon);
}

#define SCON_STDLIB_WRAPPER_0(_name, _impl) \
    static scon_handle_t scon_stdlib_##_name(scon_t* scon, scon_size_t argc, scon_handle_t* argv) \
    { \
        scon_error_check_arity(scon, argc, 0, #_name); \
        (void)argv; \
        return _impl(scon); \
    }

#define SCON_STDLIB_WRAPPER_1(_name, _impl) \
    static scon_handle_t scon_stdlib_##_name(scon_t* scon, scon_size_t argc, scon_handle_t* argv) \
    { \
        scon_error_check_arity(scon, argc, 1, #_name); \
        return _impl(scon, &argv[0]); \
    }

#define SCON_STDLIB_WRAPPER_2(_name, _impl) \
    static scon_handle_t scon_stdlib_##_name(scon_t* scon, scon_size_t argc, scon_handle_t* argv) \
    { \
        scon_error_check_arity(scon, argc, 2, #_name); \
        return _impl(scon, &argv[0], &argv[1]); \
    }

#define SCON_STDLIB_WRAPPER_3(_name, _impl) \
    static scon_handle_t scon_stdlib_##_name(scon_t* scon, scon_size_t argc, scon_handle_t* argv) \
    { \
        scon_error_check_arity(scon, argc, 3, #_name); \
        return _impl(scon, &argv[0], &argv[1], &argv[2]); \
    }

#define SCON_STDLIB_WRAPPER_R12(_name, _impl) \
    static scon_handle_t scon_stdlib_##_name(scon_t* scon, scon_size_t argc, scon_handle_t* argv) \
    { \
        scon_error_check_arity_range(scon, argc, 1, 2, #_name); \
        return _impl(scon, &argv[0], argc == 2 ? &argv[1] : SCON_NULL); \
    }

#define SCON_STDLIB_WRAPPER_R23(_name, _impl) \
    static scon_handle_t scon_stdlib_##_name(scon_t* scon, scon_size_t argc, scon_handle_t* argv) \
    { \
        scon_error_check_arity_range(scon, argc, 2, 3, #_name); \
        return _impl(scon, &argv[0], &argv[1], argc == 3 ? &argv[2] : SCON_NULL); \
    }

#define SCON_STDLIB_WRAPPER_R34(_name, _impl) \
    static scon_handle_t scon_stdlib_##_name(scon_t* scon, scon_size_t argc, scon_handle_t* argv) \
    { \
        scon_error_check_arity_range(scon, argc, 3, 4, #_name); \
        return _impl(scon, &argv[0], &argv[1], &argv[2], argc == 4 ? &argv[3] : SCON_NULL); \
    }

#define SCON_STDLIB_WRAPPER_ARG2(_name, _impl) \
    static scon_handle_t scon_stdlib_##_name(scon_t* scon, scon_size_t argc, scon_handle_t* argv) \
    { \
        scon_error_check_arity(scon, argc, 2, #_name); \
        return _impl(scon, &argv[0], &argv[1]); \
    }

#define SCON_STDLIB_WRAPPER_V1(_name, _impl) \
    static scon_handle_t scon_stdlib_##_name(scon_t* scon, scon_size_t argc, scon_handle_t* argv) \
    { \
        scon_error_check_min_arity(scon, argc, 1, #_name); \
        return _impl(scon, argc, argv); \
    }

SCON_STDLIB_WRAPPER_2(assert, scon_assert)
SCON_STDLIB_WRAPPER_1(throw, scon_throw)
SCON_STDLIB_WRAPPER_2(try, scon_try)
SCON_STDLIB_WRAPPER_2(map, scon_map)
SCON_STDLIB_WRAPPER_2(filter, scon_filter)
SCON_STDLIB_WRAPPER_2(apply, scon_apply)
SCON_STDLIB_WRAPPER_V1(len, scon_len)
SCON_STDLIB_WRAPPER_V1(is_atom, scon_is_atom)
SCON_STDLIB_WRAPPER_V1(is_int, scon_is_int)
SCON_STDLIB_WRAPPER_V1(is_float, scon_is_float)
SCON_STDLIB_WRAPPER_V1(is_number, scon_is_number)
SCON_STDLIB_WRAPPER_V1(is_string, scon_is_string)
SCON_STDLIB_WRAPPER_V1(is_lambda, scon_is_lambda)
SCON_STDLIB_WRAPPER_V1(is_native, scon_is_native)
SCON_STDLIB_WRAPPER_V1(is_callable, scon_is_callable)
SCON_STDLIB_WRAPPER_V1(is_list, scon_is_list)
SCON_STDLIB_WRAPPER_V1(is_empty, scon_is_empty)
SCON_STDLIB_WRAPPER_V1(is_nil, scon_is_nil)

static scon_handle_t scon_stdlib_reduce(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    scon_error_check_arity_range(scon, argc, 2, 3, "reduce");
    return scon_reduce(scon, &argv[0], argc == 3 ? &argv[1] : SCON_NULL, argc == 3 ? &argv[2] : &argv[1]);
}

SCON_STDLIB_WRAPPER_R12(any, scon_any)
SCON_STDLIB_WRAPPER_R12(all, scon_all)
SCON_STDLIB_WRAPPER_R12(sort, scon_sort)
SCON_STDLIB_WRAPPER_R12(flatten, scon_flatten)
SCON_STDLIB_WRAPPER_R12(log, scon_log)

static scon_handle_t scon_stdlib_range(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    scon_error_check_arity_range(scon, argc, 1, 3, "range");
    scon_handle_t* start = (argc >= 2) ? &argv[0] : SCON_NULL;
    scon_handle_t* end = (argc == 1) ? &argv[0] : (argc >= 2 ? &argv[1] : SCON_NULL);
    scon_handle_t* step = (argc == 3) ? &argv[2] : SCON_NULL;

    if (argc == 1)
    {
        scon_handle_t zero = SCON_HANDLE_FROM_INT(0);
        return scon_range(scon, &zero, end, SCON_NULL);
    }

    return scon_range(scon, start, end, step);
}

static scon_handle_t scon_stdlib_concat(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    return scon_concat(scon, argc, argv);
}

SCON_STDLIB_WRAPPER_1(first, scon_first)
SCON_STDLIB_WRAPPER_1(last, scon_last)
SCON_STDLIB_WRAPPER_1(rest, scon_rest)
SCON_STDLIB_WRAPPER_1(init, scon_init)
SCON_STDLIB_WRAPPER_1(reverse, scon_reverse)
SCON_STDLIB_WRAPPER_1(unique, scon_unique)
SCON_STDLIB_WRAPPER_1(keys, scon_keys)
SCON_STDLIB_WRAPPER_1(values, scon_values)
SCON_STDLIB_WRAPPER_R23(nth, scon_nth)
SCON_STDLIB_WRAPPER_R23(slice, scon_slice)
SCON_STDLIB_WRAPPER_R23(get_in, scon_get_in)
SCON_STDLIB_WRAPPER_R34(assoc, scon_assoc)
SCON_STDLIB_WRAPPER_ARG2(dissoc, scon_dissoc)
SCON_STDLIB_WRAPPER_R34(update, scon_update)
SCON_STDLIB_WRAPPER_ARG2(index_of, scon_index_of)
SCON_STDLIB_WRAPPER_ARG2(contains, scon_contains)
SCON_STDLIB_WRAPPER_3(replace, scon_replace)
SCON_STDLIB_WRAPPER_ARG2(chunk, scon_chunk)
SCON_STDLIB_WRAPPER_ARG2(find, scon_find)
SCON_STDLIB_WRAPPER_3(assoc_in, scon_assoc_in)
SCON_STDLIB_WRAPPER_ARG2(dissoc_in, scon_dissoc_in)
SCON_STDLIB_WRAPPER_3(update_in, scon_update_in)
SCON_STDLIB_WRAPPER_V1(merge, scon_merge)
SCON_STDLIB_WRAPPER_V1(explode, scon_explode)
SCON_STDLIB_WRAPPER_V1(implode, scon_implode)

static scon_handle_t scon_stdlib_repeat(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    scon_error_check_arity(scon, argc, 2, "repeat");
    return scon_repeat(scon, &argv[0], &argv[1]);
}

SCON_STDLIB_WRAPPER_ARG2(starts_with, scon_starts_with)
SCON_STDLIB_WRAPPER_ARG2(ends_with, scon_ends_with)
SCON_STDLIB_WRAPPER_ARG2(join, scon_join)
SCON_STDLIB_WRAPPER_ARG2(split, scon_split)

SCON_STDLIB_WRAPPER_1(upper, scon_upper)
SCON_STDLIB_WRAPPER_1(lower, scon_lower)
SCON_STDLIB_WRAPPER_1(trim, scon_trim)
SCON_STDLIB_WRAPPER_1(int, scon_get_int)
SCON_STDLIB_WRAPPER_1(float, scon_get_float)
SCON_STDLIB_WRAPPER_1(string, scon_get_string)

static scon_handle_t scon_stdlib_eval_impl(scon_t* scon, scon_handle_t* arg)
{
    return scon_eval(scon, scon_compile(scon, arg));
}
SCON_STDLIB_WRAPPER_1(eval, scon_stdlib_eval_impl)

static scon_handle_t scon_stdlib_parse_impl(scon_t* scon, scon_handle_t* arg)
{
    char* str;
    scon_size_t len;
    scon_handle_get_string_params(scon, arg, &str, &len);
    return scon_parse(scon, str, len, "<parse>");
}
SCON_STDLIB_WRAPPER_1(parse, scon_stdlib_parse_impl)

SCON_STDLIB_WRAPPER_1(load, scon_load)
SCON_STDLIB_WRAPPER_1(read_file, scon_read_file)
SCON_STDLIB_WRAPPER_2(write_file, scon_write_file)
SCON_STDLIB_WRAPPER_0(read_char, scon_read_char)
SCON_STDLIB_WRAPPER_0(read_line, scon_read_line)

static scon_handle_t scon_stdlib_print(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    return scon_print(scon, argc, argv);
}

static scon_handle_t scon_stdlib_println(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    return scon_println(scon, argc, argv);
}

SCON_STDLIB_WRAPPER_1(ord, scon_ord)
SCON_STDLIB_WRAPPER_1(chr, scon_chr)

static scon_handle_t scon_stdlib_format(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    return scon_format(scon, argc, argv);
}

SCON_STDLIB_WRAPPER_0(now, scon_now)
SCON_STDLIB_WRAPPER_0(uptime, scon_uptime)
SCON_STDLIB_WRAPPER_0(env, scon_env)
SCON_STDLIB_WRAPPER_0(args, scon_args)
SCON_STDLIB_WRAPPER_V1(min, scon_min)
SCON_STDLIB_WRAPPER_V1(max, scon_max)
SCON_STDLIB_WRAPPER_3(clamp, scon_clamp)

SCON_STDLIB_WRAPPER_1(abs, scon_abs)
SCON_STDLIB_WRAPPER_1(floor, scon_floor)
SCON_STDLIB_WRAPPER_1(ceil, scon_ceil)
SCON_STDLIB_WRAPPER_1(round, scon_round)
SCON_STDLIB_WRAPPER_1(exp, scon_exp)
SCON_STDLIB_WRAPPER_1(sqrt, scon_sqrt)
SCON_STDLIB_WRAPPER_1(sin, scon_sin)
SCON_STDLIB_WRAPPER_2(pow, scon_pow)

SCON_STDLIB_WRAPPER_1(cos, scon_cos)
SCON_STDLIB_WRAPPER_1(tan, scon_tan)
SCON_STDLIB_WRAPPER_1(asin, scon_asin)
SCON_STDLIB_WRAPPER_1(acos, scon_acos)
SCON_STDLIB_WRAPPER_1(atan, scon_atan)
SCON_STDLIB_WRAPPER_1(sinh, scon_sinh)
SCON_STDLIB_WRAPPER_1(cosh, scon_cosh)
SCON_STDLIB_WRAPPER_1(tanh, scon_tanh)
SCON_STDLIB_WRAPPER_1(asinh, scon_asinh)
SCON_STDLIB_WRAPPER_1(acosh, scon_acosh)
SCON_STDLIB_WRAPPER_1(atanh, scon_atanh)
SCON_STDLIB_WRAPPER_2(atan2, scon_atan2)
SCON_STDLIB_WRAPPER_2(rand, scon_rand)
SCON_STDLIB_WRAPPER_1(seed, scon_seed)

SCON_API void scon_stdlib_register(scon_t* scon, scon_stdlib_sets_t sets)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (sets & SCON_STDLIB_ERROR)
    {
        scon_native_t natives[] = {
            {"assert!", scon_stdlib_assert},
            {"throw!", scon_stdlib_throw},
            {"try", scon_stdlib_try},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
    if (sets & SCON_STDLIB_HIGHER_ORDER)
    {
        scon_native_t natives[] = {
            {"map", scon_stdlib_map},
            {"filter", scon_stdlib_filter},
            {"reduce", scon_stdlib_reduce},
            {"apply", scon_stdlib_apply},
            {"any?", scon_stdlib_any},
            {"all?", scon_stdlib_all},
            {"sort", scon_stdlib_sort},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
    if (sets & SCON_STDLIB_SEQUENCES)
    {
        scon_native_t natives[] = {
            {"len", scon_stdlib_len},
            {"range", scon_stdlib_range},
            {"concat", scon_stdlib_concat},
            {"first", scon_stdlib_first},
            {"last", scon_stdlib_last},
            {"rest", scon_stdlib_rest},
            {"init", scon_stdlib_init},
            {"nth", scon_stdlib_nth},
            {"assoc", scon_stdlib_assoc},
            {"dissoc", scon_stdlib_dissoc},
            {"update", scon_stdlib_update},
            {"index-of", scon_stdlib_index_of},
            {"reverse", scon_stdlib_reverse},
            {"slice", scon_stdlib_slice},
            {"flatten", scon_stdlib_flatten},
            {"contains?", scon_stdlib_contains},
            {"replace", scon_stdlib_replace},
            {"unique", scon_stdlib_unique},
            {"chunk", scon_stdlib_chunk},
            {"find", scon_stdlib_find},
            {"get-in", scon_stdlib_get_in},
            {"assoc-in", scon_stdlib_assoc_in},
            {"dissoc-in", scon_stdlib_dissoc_in},
            {"update-in", scon_stdlib_update_in},
            {"keys", scon_stdlib_keys},
            {"values", scon_stdlib_values},
            {"merge", scon_stdlib_merge},
            {"explode", scon_stdlib_explode},
            {"implode", scon_stdlib_implode},
            {"repeat", scon_stdlib_repeat},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
    if (sets & SCON_STDLIB_STRING)
    {
        scon_native_t natives[] = {
            {"starts-with?", scon_stdlib_starts_with},
            {"ends-with?", scon_stdlib_ends_with},
            {"contains?", scon_stdlib_contains},
            {"replace", scon_stdlib_replace},
            {"join", scon_stdlib_join},
            {"split", scon_stdlib_split},
            {"upper", scon_stdlib_upper},
            {"lower", scon_stdlib_lower},
            {"trim", scon_stdlib_trim},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
    if (sets & SCON_STDLIB_INTROSPECTION)
    {
        scon_native_t natives[] = {
            {"atom?", scon_stdlib_is_atom},
            {"int?", scon_stdlib_is_int},
            {"float?", scon_stdlib_is_float},
            {"number?", scon_stdlib_is_number},
            {"string?", scon_stdlib_is_string},
            {"lambda?", scon_stdlib_is_lambda},
            {"native?", scon_stdlib_is_native},
            {"callable?", scon_stdlib_is_callable},
            {"list?", scon_stdlib_is_list},
            {"empty?", scon_stdlib_is_empty},
            {"nil?", scon_stdlib_is_nil},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
    if (sets & SCON_STDLIB_TYPE_CASTING)
    {
        scon_native_t natives[] = {
            {"int", scon_stdlib_int},
            {"float", scon_stdlib_float},
            {"string", scon_stdlib_string},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
    if (sets & SCON_STDLIB_SYSTEM)
    {
        scon_native_t natives[] = {
            {"eval", scon_stdlib_eval},
            {"parse", scon_stdlib_parse},
            {"load!", scon_stdlib_load},
            {"read-file!", scon_stdlib_read_file},
            {"write-file!", scon_stdlib_write_file},
            {"read-char!", scon_stdlib_read_char},
            {"read-line!", scon_stdlib_read_line},
            {"print!", scon_stdlib_print},
            {"println!", scon_stdlib_println},
            {"ord", scon_stdlib_ord},
            {"chr", scon_stdlib_chr},
            {"format", scon_stdlib_format},
            {"now!", scon_stdlib_now},
            {"uptime!", scon_stdlib_uptime},
            {"env!", scon_stdlib_env},
            {"args!", scon_stdlib_args},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
    if (sets & SCON_STDLIB_MATH)
    {
        scon_native_t natives[] = {
            {"min", scon_stdlib_min},
            {"max", scon_stdlib_max},
            {"clamp", scon_stdlib_clamp},
            {"abs", scon_stdlib_abs},
            {"floor", scon_stdlib_floor},
            {"ceil", scon_stdlib_ceil},
            {"round", scon_stdlib_round},
            {"pow", scon_stdlib_pow},
            {"exp", scon_stdlib_exp},
            {"log", scon_stdlib_log},
            {"sqrt", scon_stdlib_sqrt},
            {"sin", scon_stdlib_sin},
            {"cos", scon_stdlib_cos},
            {"tan", scon_stdlib_tan},
            {"asin", scon_stdlib_asin},
            {"acos", scon_stdlib_acos},
            {"atan", scon_stdlib_atan},
            {"atan2", scon_stdlib_atan2},
            {"sinh", scon_stdlib_sinh},
            {"cosh", scon_stdlib_cosh},
            {"tanh", scon_stdlib_tanh},
            {"asinh", scon_stdlib_asinh},
            {"acosh", scon_stdlib_acosh},
            {"atanh", scon_stdlib_atanh},
            {"rand", scon_stdlib_rand},
            {"seed!", scon_stdlib_seed},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
}

#endif
