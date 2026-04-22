#include "item.h"
#ifndef SCON_STRINGIFY_IMPL_H
#define SCON_STRINGIFY_IMPL_H 1

#include "stringify.h"

SCON_API scon_size_t scon_stringify(scon_t* scon, scon_handle_t* handle, char* buffer, scon_size_t size)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(buffer != SCON_NULL || size == 0);

    if (handle == SCON_NULL)
    {
        return SCON_SNPRINTF(buffer, size, "<null>");
    }

    if (*handle == SCON_HANDLE_NONE)
    {
        return SCON_SNPRINTF(buffer, size, "<none>");
    }

    if (!SCON_HANDLE_IS_ITEM(handle))
    {
        if (SCON_HANDLE_IS_INT(handle))
        {
            return SCON_SNPRINTF(buffer, size, "%lld", (long long)SCON_HANDLE_TO_INT(handle));
        }
        else if (SCON_HANDLE_IS_FLOAT(handle))
        {
            return SCON_SNPRINTF(buffer, size, "%g", (double)SCON_HANDLE_TO_FLOAT(handle));
        }

        return SCON_SNPRINTF(buffer, size, "<unknown>");
    }

    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);
    switch (item->type)
    {
    case SCON_ITEM_TYPE_ATOM:
    {
        if (item->flags & SCON_ITEM_FLAG_INT_SHAPED)
        {
            return SCON_SNPRINTF(buffer, size, "%lld", (long long)item->atom.integerValue);
        }
        else if (item->flags & SCON_ITEM_FLAG_FLOAT_SHAPED)
        {
            return SCON_SNPRINTF(buffer, size, "%g", (double)item->atom.floatValue);
        }

        return SCON_SNPRINTF(buffer, size, "\"%.*s\"", (int)item->atom.length, item->atom.string);
    }
    case SCON_ITEM_TYPE_LIST:
    {
        scon_size_t written = 0;
        scon_size_t res = SCON_SNPRINTF(buffer, size, "(");
        written += res;

        for (scon_size_t i = 0; i < item->length; ++i)
        {
            scon_handle_t child = item->list.handles[i];
            res = scon_stringify(scon, &child, size > written ? buffer + written : SCON_NULL,
                size > written ? size - written : 0);
            written += res;

            if (i < item->length - 1)
            {
                res = SCON_SNPRINTF(size > written ? buffer + written : SCON_NULL, size > written ? size - written : 0,
                    " ");
                written += res;
            }
        }

        res = SCON_SNPRINTF(size > written ? buffer + written : SCON_NULL, size > written ? size - written : 0, ")");
        written += res;

        return written;
    }
    case SCON_ITEM_TYPE_FUNCTION:
        return SCON_SNPRINTF(buffer, size, "<function at %p>", (void*)item);
    case SCON_ITEM_TYPE_CLOSURE:
        return SCON_SNPRINTF(buffer, size, "<closure at %p>", (void*)item);
    default:
        return SCON_SNPRINTF(buffer, size, "<unknown>");
    }
    return 0;
}

#endif