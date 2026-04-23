#ifndef SCON_STDLIB_SEQUENCES_IMPL_H
#define SCON_STDLIB_SEQUENCES_IMPL_H 1

#include "core.h"
#include "handle.h"
#include "stdlib_sequences.h"
#include "stdlib_type_casting.h"

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
        SCON_ERROR_RUNTIME(scon, item, "first expected list or atom, got %s", scon_item_type_str(item->type));
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
        return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, item->atom.string + item->length - 1, 1, SCON_ATOM_LOOKUP_NONE));
    default:
        SCON_ERROR_RUNTIME(scon, item, "last expected list or atom, got %s", scon_item_type_str(item->type));
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

SCON_API scon_handle_t scon_nth(scon_t* scon, scon_handle_t* handle, scon_handle_t* index)
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

SCON_API scon_handle_t scon_index(scon_t* scon, scon_handle_t* handle, scon_handle_t* target)
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
        SCON_ERROR_RUNTIME(scon, item, "index expected list or atom, got %s", scon_item_type_str(item->type));
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

#endif