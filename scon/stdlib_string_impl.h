#ifndef SCON_STDLIB_STRING_IMPL_H
#define SCON_STDLIB_STRING_IMPL_H 1

#include "char.h"
#include "core.h"
#include "handle.h"
#include "stdlib_string.h"

SCON_API scon_handle_t scon_starts_with(scon_t* scon, scon_handle_t* handle, scon_handle_t* prefix)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
    {
        if (item->length == 0)
        {
            return SCON_HANDLE_FALSE();
        }
        scon_handle_t first = item->list.handles[0];
        return (scon_handle_compare(scon, &first, prefix) == 0) ? SCON_HANDLE_TRUE() : SCON_HANDLE_FALSE();
    }
    case SCON_ITEM_TYPE_ATOM:
    {
        char *srcStr, *preStr;
        scon_size_t srcLen, preLen;
        scon_handle_get_string_params(scon, handle, &srcStr, &srcLen);
        scon_handle_get_string_params(scon, prefix, &preStr, &preLen);

        if (preLen > srcLen)
        {
            return SCON_HANDLE_FALSE();
        }
        return (SCON_MEMCMP(srcStr, preStr, preLen) == 0) ? SCON_HANDLE_TRUE() : SCON_HANDLE_FALSE();
    }
    default:
        SCON_ERROR_RUNTIME(scon, "starts-with? expected list or atom, got %s", scon_item_type_str(item->type));
    }
}

SCON_API scon_handle_t scon_ends_with(scon_t* scon, scon_handle_t* handle, scon_handle_t* suffix)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);

    switch (item->type)
    {
    case SCON_ITEM_TYPE_LIST:
    {
        if (item->length == 0)
        {
            return SCON_HANDLE_FALSE();
        }
        scon_handle_t last = item->list.handles[item->length - 1];
        return (scon_handle_compare(scon, &last, suffix) == 0) ? SCON_HANDLE_TRUE() : SCON_HANDLE_FALSE();
    }
    case SCON_ITEM_TYPE_ATOM:
    {
        char *srcStr, *sufStr;
        scon_size_t srcLen, sufLen;
        scon_handle_get_string_params(scon, handle, &srcStr, &srcLen);
        scon_handle_get_string_params(scon, suffix, &sufStr, &sufLen);

        if (sufLen > srcLen)
        {
            return SCON_HANDLE_FALSE();
        }
        return (SCON_MEMCMP(srcStr + srcLen - sufLen, sufStr, sufLen) == 0) ? SCON_HANDLE_TRUE() : SCON_HANDLE_FALSE();
    }
    default:
        SCON_ERROR_RUNTIME(scon, "ends-with expected list or atom, got %s", scon_item_type_str(item->type));
    }
}

SCON_API scon_handle_t scon_join(scon_t* scon, scon_handle_t* listHandle, scon_handle_t* sepHandle)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (!SCON_HANDLE_IS_LIST(listHandle))
    {
        SCON_ERROR_RUNTIME(scon, "join expects a list as the first argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, listHandle)));
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
    for (scon_size_t i = 0; i < list->length; i++)
    {
        scon_handle_t ch = list->list.handles[i];
        scon_handle_ensure_item(scon, &ch);
        totalLen += SCON_HANDLE_TO_ITEM(&ch)->length;
    }
    totalLen += sepLen * (list->length - 1);

    char stackBuffer[SCON_STACK_BUFFER_SIZE];
    char* buffer = (totalLen < SCON_STACK_BUFFER_SIZE) ? stackBuffer : (char*)SCON_MALLOC(totalLen);
    if (buffer == SCON_NULL)
    {
        SCON_ERROR_INTERNAL(scon, "out of memory");
    }

    scon_size_t currentPos = 0;
    for (scon_size_t i = 0; i < list->length; i++)
    {
        char* str;
        scon_size_t len;
        scon_handle_get_string_params(scon, &list->list.handles[i], &str, &len);
        SCON_MEMCPY(buffer + currentPos, str, len);
        currentPos += len;

        if (i < list->length - 1)
        {
            SCON_MEMCPY(buffer + currentPos, sepStr, sepLen);
            currentPos += sepLen;
        }
    }

    scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, totalLen, SCON_ATOM_LOOKUP_NONE));
    if (buffer != stackBuffer)
    {
        SCON_FREE(buffer);
    }
    return result;
}

SCON_API scon_handle_t scon_split(scon_t* scon, scon_handle_t* srcHandle, scon_handle_t* sepHandle)
{
    SCON_ASSERT(scon != SCON_NULL);

    char *srcStr, *sepStr;
    scon_size_t srcLen, sepLen;

    scon_handle_get_string_params(scon, srcHandle, &srcStr, &srcLen);
    scon_handle_get_string_params(scon, sepHandle, &sepStr, &sepLen);

    scon_list_t* resultList = scon_list_new(scon, 0);
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

SCON_API scon_handle_t scon_upper(scon_t* scon, scon_handle_t* srcHandle)
{
    SCON_ASSERT(scon != SCON_NULL);

    char* srcStr;
    scon_size_t srcLen;
    scon_handle_get_string_params(scon, srcHandle, &srcStr, &srcLen);

    if (srcLen == 0)
    {
        return *srcHandle;
    }

    char stackBuffer[SCON_STACK_BUFFER_SIZE];
    char* buffer = (srcLen < SCON_STACK_BUFFER_SIZE) ? stackBuffer : (char*)SCON_MALLOC(srcLen);
    if (buffer == SCON_NULL)
    {
        SCON_ERROR_INTERNAL(scon, "out of memory");
    }

    for (scon_size_t i = 0; i < srcLen; i++)
    {
        buffer[i] = SCON_CHAR_TO_UPPER(srcStr[i]);
    }

    scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, srcLen, SCON_ATOM_LOOKUP_NONE));
    if (buffer != stackBuffer)
    {
        SCON_FREE(buffer);
    }
    return result;
}

SCON_API scon_handle_t scon_lower(scon_t* scon, scon_handle_t* srcHandle)
{
    SCON_ASSERT(scon != SCON_NULL);

    char* srcStr;
    scon_size_t srcLen;
    scon_handle_get_string_params(scon, srcHandle, &srcStr, &srcLen);

    if (srcLen == 0)
    {
        return *srcHandle;
    }

    char stackBuffer[SCON_STACK_BUFFER_SIZE];
    char* buffer = (srcLen < SCON_STACK_BUFFER_SIZE) ? stackBuffer : (char*)SCON_MALLOC(srcLen);
    if (buffer == SCON_NULL)
    {
        SCON_ERROR_INTERNAL(scon, "out of memory");
    }

    for (scon_size_t i = 0; i < srcLen; i++)
    {
        buffer[i] = SCON_CHAR_TO_LOWER(srcStr[i]);
    }

    scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, srcLen, SCON_ATOM_LOOKUP_NONE));
    if (buffer != stackBuffer)
    {
        SCON_FREE(buffer);
    }
    return result;
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

#endif