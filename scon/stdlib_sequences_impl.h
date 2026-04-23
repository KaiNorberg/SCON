#ifndef SCON_STDLIB_SEQUENCES_IMPL_H
#define SCON_STDLIB_SEQUENCES_IMPL_H 1

#include "core.h"
#include "handle.h"
#include "stdlib_sequences.h"
#include "stdlib_type_casting.h"
#include "eval.h"
#include "gc.h"

SCON_API scon_handle_t scon_len(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    scon_size_t total = 0;
    for (scon_size_t i = 0; i < argc; i++)
    {
        scon_handle_ensure_item(scon, &argv[i]);
        total += SCON_HANDLE_TO_ITEM(&argv[i])->length;
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

    scon_list_t* list = scon_list_new(scon, count);
    scon_int64_t current = startVal;
    for (scon_size_t i = 0; i < count; i++)
    {
        list->handles[i] = SCON_HANDLE_FROM_INT(current);
        current += stepVal;
    }
    list->length = (scon_uint32_t)count;
    return SCON_HANDLE_FROM_LIST(list);
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
        scon_handle_ensure_item(scon, &argv[i]);
        totalLen += SCON_HANDLE_TO_ITEM(&argv[i])->length;
    }

    if (resultIsList)
    {
        scon_list_t* newList = scon_list_new(scon, totalLen);
        for (scon_size_t i = 0; i < argc; i++)
        {
            if (SCON_HANDLE_IS_LIST(&argv[i]))
            {
                scon_item_t* item = SCON_HANDLE_TO_ITEM(&argv[i]);
                scon_size_t len = item->list.length;
                SCON_MEMCPY(newList->handles + newList->length, item->list.handles, len * sizeof(scon_handle_t));
                newList->length += (scon_uint32_t)len;
            }
            else
            {
                newList->handles[newList->length++] = argv[i];
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
        scon_handle_get_string_params(scon, &argv[i], &str, &len);
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

SCON_API scon_handle_t scon_first(scon_t* scon, scon_handle_t* handle)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);

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
        SCON_ERROR_RUNTIME(scon, "first expected list or atom, got %s", scon_item_type_str(item->type));
    }
}

SCON_API scon_handle_t scon_last(scon_t* scon, scon_handle_t* handle)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);

    if (item->length == 0)
    {
        return scon_handle_nil(scon);
    }

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
        return item->list.handles[item->length - 1];
    case SCON_ITEM_TYPE_ATOM:
        return SCON_HANDLE_FROM_ATOM(
            scon_atom_lookup(scon, item->atom.string + item->length - 1, 1, SCON_ATOM_LOOKUP_NONE));
    default:
        SCON_ERROR_RUNTIME(scon, "last expected list or atom, got %s", scon_item_type_str(item->type));
    }
}

SCON_API scon_handle_t scon_rest(scon_t* scon, scon_handle_t* handle)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);

    if (item->length <= 1)
    {
        return scon_handle_nil(scon);
    }

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
    {
        scon_size_t newLen = item->length - 1;
        return SCON_HANDLE_FROM_LIST(scon_list_new_from_data(scon, item->list.handles + 1, newLen));
    }
    case SCON_ITEM_TYPE_ATOM:
    {
        return SCON_HANDLE_FROM_ATOM(
            scon_atom_lookup(scon, item->atom.string + 1, item->length - 1, SCON_ATOM_LOOKUP_NONE));
    }
    default:
        SCON_ERROR_RUNTIME(scon, "rest expected list or atom, got %s", scon_item_type_str(item->type));
    }
}

SCON_API scon_handle_t scon_init(scon_t* scon, scon_handle_t* handle)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);

    if (item->length <= 1)
    {
        return scon_handle_nil(scon);
    }

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
    {
        scon_size_t newLen = item->length - 1;
        return SCON_HANDLE_FROM_LIST(scon_list_new_from_data(scon, item->list.handles, newLen));
    }
    case SCON_ITEM_TYPE_ATOM:
    {
        return SCON_HANDLE_FROM_ATOM(
            scon_atom_lookup(scon, item->atom.string, item->length - 1, SCON_ATOM_LOOKUP_NONE));
    }
    default:
        SCON_ERROR_RUNTIME(scon, "init expected list or atom, got %s", scon_item_type_str(item->type));
    }
}

SCON_API scon_handle_t scon_nth(scon_t* scon, scon_handle_t* handle, scon_handle_t* index, scon_handle_t* defaultVal)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_t nHandle = scon_get_int(scon, index);
    scon_int64_t n = SCON_HANDLE_TO_INT(&nHandle);

    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);

    if (n < 0)
    {
        n = (scon_int64_t)item->length + n;
    }

    if (n < 0 || n >= (scon_int64_t)item->length)
    {
        return (defaultVal != SCON_NULL) ? *defaultVal : scon_handle_nil(scon);
    }

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
        return item->list.handles[n];
    case SCON_ITEM_TYPE_ATOM:
        return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, item->atom.string + n, 1, SCON_ATOM_LOOKUP_NONE));
    default:
        SCON_ERROR_RUNTIME(scon, "nth expected list or atom, got %s", scon_item_type_str(item->type));
    }
}

SCON_API scon_handle_t scon_assoc(scon_t* scon, scon_handle_t* handle, scon_handle_t* index, scon_handle_t* value)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_t nHandle = scon_get_int(scon, index);
    scon_int64_t n = SCON_HANDLE_TO_INT(&nHandle);

    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);

    if (n < 0)
    {
        n = (scon_int64_t)item->length + n;
        if (n < 0)
        {
            n = 0;
        }
    }

    scon_size_t targetIndex = (scon_size_t)n;
    scon_size_t newLen = SCON_MAX(item->length, targetIndex + 1);

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
    {
        scon_list_t* newList = scon_list_new(scon, newLen);

        scon_size_t prefixLen = SCON_MIN(item->length, targetIndex);
        if (prefixLen > 0)
        {
            SCON_MEMCPY(newList->handles, item->list.handles, prefixLen * sizeof(scon_handle_t));
        }
        for (scon_size_t i = item->length; i < targetIndex; i++)
        {
            newList->handles[i] = SCON_HANDLE_FROM_INT(0);
        }
        newList->handles[targetIndex] = *value;
        if (targetIndex + 1 < item->length)
        {
            scon_size_t suffixLen = item->length - (targetIndex + 1);
            SCON_MEMCPY(newList->handles + targetIndex + 1, item->list.handles + targetIndex + 1, suffixLen * sizeof(scon_handle_t));
        }
        newList->length = (scon_uint32_t)newLen;
        return SCON_HANDLE_FROM_LIST(newList);
    }
    case SCON_ITEM_TYPE_ATOM:
    {
        char* valStr;
        scon_size_t valLen;
        scon_handle_get_string_params(scon, value, &valStr, &valLen);
        char charToPut = (valLen > 0) ? valStr[0] : ' ';

        char stackBuffer[SCON_STACK_BUFFER_SIZE];
        char* buffer = (newLen < SCON_STACK_BUFFER_SIZE) ? stackBuffer : (char*)SCON_MALLOC(newLen);
        if (buffer == SCON_NULL)
        {
            SCON_ERROR_INTERNAL(scon, "out of memory");
        }

        scon_size_t prefixLen = SCON_MIN(item->length, targetIndex);
        SCON_MEMCPY(buffer, item->atom.string, prefixLen);
        for (scon_size_t i = item->length; i < targetIndex; i++)
        {
            buffer[i] = ' ';
        }
        buffer[targetIndex] = charToPut;
        if (targetIndex + 1 < item->length)
        {
            scon_size_t suffixLen = item->length - (targetIndex + 1);
            SCON_MEMCPY(buffer + targetIndex + 1, item->atom.string + targetIndex + 1, suffixLen);
        }

        scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, newLen, SCON_ATOM_LOOKUP_NONE));
        if (buffer != stackBuffer)
        {
            SCON_FREE(buffer);
        }
        return result;
    }
    default:
        SCON_ERROR_RUNTIME(scon, "assoc expected list or atom, got %s", scon_item_type_str(item->type));
    }
}

SCON_API scon_handle_t scon_dissoc(struct scon* scon, scon_handle_t* handle, scon_handle_t* index)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_t nHandle = scon_get_int(scon, index);
    scon_int64_t n = SCON_HANDLE_TO_INT(&nHandle);

    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);

    if (n < 0)
    {
        n = (scon_int64_t)item->length + n;
    }

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
        scon_list_t* newList = scon_list_new(scon, newLen);
        SCON_MEMCPY(newList->handles, item->list.handles, targetIndex * sizeof(scon_handle_t));
        SCON_MEMCPY(newList->handles + targetIndex, item->list.handles + targetIndex + 1, 
                    (item->length - targetIndex - 1) * sizeof(scon_handle_t));
        newList->length = (scon_uint32_t)newLen;
        return SCON_HANDLE_FROM_LIST(newList);
    }
    case SCON_ITEM_TYPE_ATOM:
    {
        char stackBuffer[SCON_STACK_BUFFER_SIZE];
        char* buffer = (newLen < SCON_STACK_BUFFER_SIZE) ? stackBuffer : (char*)SCON_MALLOC(newLen);
        if (buffer == SCON_NULL)
        {
            SCON_ERROR_INTERNAL(scon, "out of memory");
        }

        SCON_MEMCPY(buffer, item->atom.string, targetIndex);
        SCON_MEMCPY(buffer + targetIndex, item->atom.string + targetIndex + 1, 
                    item->length - targetIndex - 1);

        scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, newLen, SCON_ATOM_LOOKUP_NONE));
        if (buffer != stackBuffer)
        {
            SCON_FREE(buffer);
        }
        return result;
    }
    default:
        SCON_ERROR_RUNTIME(scon, "dissoc expected list or atom, got %s", scon_item_type_str(item->type));
    }
}

SCON_API scon_handle_t scon_update(scon_t* scon, scon_handle_t* handle, scon_handle_t* index, scon_handle_t* callable)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_t currentVal = scon_nth(scon, handle, index, SCON_NULL);
    scon_handle_t newVal = scon_eval_call(scon, *callable, 1, &currentVal);

    return scon_assoc(scon, handle, index, &newVal);
}

SCON_API scon_handle_t scon_index_of(scon_t* scon, scon_handle_t* handle, scon_handle_t* target)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
    {
        for (scon_size_t i = 0; i < item->length; i++)
        {
            scon_handle_t current = item->list.handles[i];
            if (scon_handle_compare(scon, &current, target) == 0)
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

    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);

    if (item->length <= 1)
    {
        return *handle;
    }

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
    {
        scon_list_t* newList = scon_list_new(scon, item->length);
        for (scon_size_t i = 0; i < item->length; i++)
        {
            newList->handles[i] = item->list.handles[item->length - 1 - i];
        }
        newList->length = item->length;
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

        scon_handle_t result =
            SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, item->length, SCON_ATOM_LOOKUP_NONE));
        if (buffer != stackBuffer)
        {
            SCON_FREE(buffer);
        }
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

    scon_handle_t startHVal = scon_get_int(scon, startH);
    scon_int64_t start = SCON_HANDLE_TO_INT(&startHVal);

    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);

    scon_int64_t end;
    if (endH != SCON_NULL)
    {
        scon_handle_t endHVal = scon_get_int(scon, endH);
        end = SCON_HANDLE_TO_INT(&endHVal);
    }
    else
    {
        end = (scon_int64_t)item->length;
    }

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
        return (item->type == SCON_ITEM_TYPE_LIST) ? SCON_HANDLE_FROM_LIST(scon_list_new(scon, 0))
                                                   : scon_handle_nil(scon);
    }

    scon_size_t newLen = (scon_size_t)(end - start);

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
    {
        return SCON_HANDLE_FROM_LIST(scon_list_new_from_data(scon, item->list.handles + start, newLen));
    }
    case SCON_ITEM_TYPE_ATOM:
    {
        return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, item->atom.string + start, newLen, SCON_ATOM_LOOKUP_NONE));
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
    scon_list_t* newList = scon_list_new(scon, 0);
    scon_handle_t newHandle = SCON_HANDLE_FROM_LIST(newList);
    SCON_GC_RETAIN(scon, newHandle);

    for (scon_size_t i = 0; i < item->length; i++)
    {
        scon_handle_t current = item->list.handles[i];
        if (SCON_HANDLE_IS_LIST(&current))
        {
            scon_handle_t nextDepthH = SCON_HANDLE_FROM_INT(depth - 1);
            scon_handle_t flattened = scon_flatten(scon, &current, &nextDepthH);
            SCON_GC_RETAIN(scon, flattened);

            scon_item_t* flattenedItem = SCON_HANDLE_TO_ITEM(&flattened);
            for (scon_size_t j = 0; j < flattenedItem->length; j++)
            {
                scon_list_append(scon, newList, flattenedItem->list.handles[j]);
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
    return index == scon_handle_nil(scon) ? SCON_HANDLE_FALSE() : SCON_HANDLE_TRUE();
}

SCON_API scon_handle_t scon_replace(struct scon* scon, scon_handle_t* handle, scon_handle_t* oldVal, scon_handle_t* newVal)
{
    SCON_ASSERT(scon != SCON_NULL);
    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);

    if (item->type == SCON_ITEM_TYPE_LIST)
    {
        scon_list_t* newList = scon_list_new(scon, item->length);
        for (scon_size_t i = 0; i < item->length; i++)
        {
            scon_handle_t current = item->list.handles[i];
            if (scon_handle_compare(scon, &current, oldVal) == 0)
            {
                scon_list_append(scon, newList, *newVal);
            }
            else
            {
                scon_list_append(scon, newList, current);
            }
        }
        return SCON_HANDLE_FROM_LIST(newList);
    }
    else if (item->type == SCON_ITEM_TYPE_ATOM)
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
        for (scon_size_t i = 0; i < item->length; )
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
    else
    {
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
    scon_list_t* newList = scon_list_new(scon, 0);
    scon_handle_t newHandle = SCON_HANDLE_FROM_LIST(newList);
    SCON_GC_RETAIN(scon, newHandle);

    for (scon_size_t i = 0; i < item->length; i++)
    {
        scon_handle_t current = item->list.handles[i];
        scon_bool_t found = SCON_FALSE;
        for (scon_size_t j = 0; j < newList->length; j++)
        {
            if (scon_handle_compare(scon, &current, &newList->handles[j]) == 0)
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

    SCON_GC_RELEASE(scon, newHandle);
    return newHandle;
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
    scon_size_t chunkSize = (scon_size_t)n;
    scon_size_t numChunks = (item->length + chunkSize - 1) / chunkSize;

    scon_list_t* resultList = scon_list_new(scon, numChunks);
    scon_handle_t resultHandle = SCON_HANDLE_FROM_LIST(resultList);
    SCON_GC_RETAIN(scon, resultHandle);

    for (scon_size_t i = 0; i < item->length; i += chunkSize)
    {
        scon_size_t currentChunkSize = SCON_MIN(chunkSize, item->length - i);
        scon_list_t* chunk = scon_list_new(scon, currentChunkSize);
        for (scon_size_t j = 0; j < currentChunkSize; j++)
        {
            scon_list_append(scon, chunk, item->list.handles[i + j]);
        }
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
    for (scon_size_t i = 0; i < item->length; i++)
    {
        scon_handle_t arg = item->list.handles[i];
        scon_handle_t res = scon_eval_call(scon, *callable, 1, &arg);
        if (SCON_HANDLE_IS_TRUTHY(&res))
        {
            return arg;
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
        scon_handle_ensure_item(scon, list);
        scon_item_t* listItem = SCON_HANDLE_TO_ITEM(list);
        for (scon_size_t i = 0; i < listItem->length; i++)
        {
            scon_handle_t entryH = listItem->list.handles[i];
            if (SCON_HANDLE_IS_LIST(&entryH))
            {
                scon_item_t* entry = SCON_HANDLE_TO_ITEM(&entryH);
                if (entry->length >= 1 && scon_handle_compare(scon, &entry->list.handles[0], path) == 0)
                {
                    return entry->length >= 2 ? entry->list.handles[1] : scon_handle_nil(scon);
                }
            }
        }
        return (defaultVal != SCON_NULL) ? *defaultVal : scon_handle_nil(scon);
    }

    scon_item_t* pathItem = SCON_HANDLE_TO_ITEM(&pathH);
    for (scon_size_t i = 0; i < pathItem->length; i++)
    {
        scon_handle_t key = pathItem->list.handles[i];
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
        scon_handle_ensure_item(scon, list);
        scon_item_t* listItem = SCON_HANDLE_TO_ITEM(list);
        scon_list_t* newList = scon_list_new(scon, listItem->length);
        scon_bool_t found = SCON_FALSE;

        for (scon_size_t i = 0; i < listItem->length; i++)
        {
            scon_handle_t entryH = listItem->list.handles[i];
            if (SCON_HANDLE_IS_LIST(&entryH))
            {
                scon_item_t* entry = SCON_HANDLE_TO_ITEM(&entryH);
                if (entry->length >= 1 && scon_handle_compare(scon, &entry->list.handles[0], path) == 0)
                {
                    scon_list_t* newEntry = scon_list_new(scon, 2);
                    scon_list_append(scon, newEntry, *path);
                    scon_list_append(scon, newEntry, *val);
                    scon_list_append(scon, newList, SCON_HANDLE_FROM_LIST(newEntry));
                    found = SCON_TRUE;
                    continue;
                }
            }
            scon_list_append(scon, newList, entryH);
        }

        if (!found)
        {
            scon_list_t* newEntry = scon_list_new(scon, 2);
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

    scon_handle_t firstKey = pathItem->list.handles[0];
    scon_handle_t restPath;

    if (pathItem->length == 1)
    {
        restPath = firstKey;
        return scon_assoc_in(scon, list, &restPath, val);
    }
    else
    {
        scon_list_t* restPathList = scon_list_new(scon, pathItem->length - 1);
        for (scon_size_t i = 1; i < pathItem->length; i++)
        {
            scon_list_append(scon, restPathList, pathItem->list.handles[i]);
        }
        restPath = SCON_HANDLE_FROM_LIST(restPathList);
        SCON_GC_RETAIN(scon, restPath);

        scon_handle_t subList = scon_get_in(scon, list, &firstKey, SCON_NULL);
        if (!SCON_HANDLE_IS_LIST(&subList))
        {
            subList = SCON_HANDLE_FROM_LIST(scon_list_new(scon, 0));
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
        scon_handle_ensure_item(scon, list);
        scon_item_t* listItem = SCON_HANDLE_TO_ITEM(list);
        scon_list_t* newList = scon_list_new(scon, listItem->length);

        for (scon_size_t i = 0; i < listItem->length; i++)
        {
            scon_handle_t entryH = listItem->list.handles[i];
            if (SCON_HANDLE_IS_LIST(&entryH))
            {
                scon_item_t* entry = SCON_HANDLE_TO_ITEM(&entryH);
                if (entry->length >= 1 && scon_handle_compare(scon, &entry->list.handles[0], path) == 0)
                {
                    continue;
                }
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

    scon_handle_t firstKey = pathItem->list.handles[0];
    if (pathItem->length == 1)
    {
        return scon_dissoc_in(scon, list, &firstKey);
    }
    else
    {
        scon_list_t* restPathList = scon_list_new(scon, pathItem->length - 1);
        for (scon_size_t i = 1; i < pathItem->length; i++)
        {
            scon_list_append(scon, restPathList, pathItem->list.handles[i]);
        }
        scon_handle_t restPath = SCON_HANDLE_FROM_LIST(restPathList);
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

SCON_API scon_handle_t scon_keys(scon_t* scon, scon_handle_t* listHandle)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (!SCON_HANDLE_IS_LIST(listHandle))
    {
        SCON_ERROR_RUNTIME(scon, "keys expects a list, got %s",
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
        SCON_ERROR_RUNTIME(scon, "values expects a list, got %s",
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

/** **`(merge <list> {list}) -> <list>`**

Returns a new list containing all unique keys from all provided association lists, with values from later lists overwriting those from earlier ones.

**`(zip <list> {list}) -> <list>`**

Returns a new list of sub-lists, where each sub-list contains the i-th element from each of the provided lists. The length of the resulting list is determined by the shortest input list. */

SCON_API scon_handle_t scon_merge(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    scon_handle_t result = SCON_HANDLE_FROM_LIST(scon_list_new(scon, 0));
    SCON_GC_RETAIN(scon, result);

    for (scon_size_t i = 0; i < argc; i++)
    {
        if (!SCON_HANDLE_IS_LIST(&argv[i]))
        {
            continue;
        }

        scon_item_t* currentList = SCON_HANDLE_TO_ITEM(&argv[i]);
        for (scon_size_t j = 0; j < currentList->length; j++)
        {
            scon_handle_t entryH = currentList->list.handles[j];
            if (SCON_HANDLE_IS_LIST(&entryH))
            {
                scon_item_t* entry = SCON_HANDLE_TO_ITEM(&entryH);
                if (entry->length >= 1)
                {
                    scon_handle_t key = entry->list.handles[0];
                    scon_handle_t val = entry->length >= 2 ? entry->list.handles[1] : scon_handle_nil(scon);
                    scon_handle_t next = scon_assoc_in(scon, &result, &key, &val);
                    SCON_GC_RETAIN(scon, next);
                    SCON_GC_RELEASE(scon, result);
                    result = next;
                }
            }
        }
    }

    SCON_GC_RELEASE(scon, result);
    return result;
}


#endif