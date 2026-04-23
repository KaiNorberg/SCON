#ifndef SCON_STDLIB_TYPE_CASTING_IMPL_H
#define SCON_STDLIB_TYPE_CASTING_IMPL_H 1

#include "core.h"
#include "handle.h"
#include "stdlib_type_casting.h"
#include "stringify.h"

SCON_API scon_handle_t scon_get_int(scon_t* scon, scon_handle_t* handle)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (SCON_HANDLE_IS_INT(handle))
    {
        return *handle;
    }
    if (SCON_HANDLE_IS_FLOAT(handle))
    {
        return SCON_HANDLE_FROM_INT((scon_int64_t)SCON_HANDLE_TO_FLOAT(handle));
    }

    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);
    if (item->flags & SCON_ITEM_FLAG_INT_SHAPED)
    {
        return SCON_HANDLE_FROM_INT(item->atom.integerValue);
    }
    if (item->flags & SCON_ITEM_FLAG_FLOAT_SHAPED)
    {
        return SCON_HANDLE_FROM_INT((scon_int64_t)item->atom.floatValue);
    }

    if (item->flags & SCON_ITEM_FLAG_QUOTED)
    {
        scon_atom_t* atom = scon_atom_lookup(scon, item->atom.string, item->length, SCON_ATOM_LOOKUP_NONE);
        if (atom == SCON_NULL)
        {
            SCON_ERROR_RUNTIME(scon, "expected int, got %s", scon_item_type_str(item->type));
        }
        if (SCON_CONTAINER_OF(atom, scon_item_t, atom)->flags & SCON_ITEM_FLAG_INT_SHAPED)
        {
            return SCON_HANDLE_FROM_INT(atom->integerValue);
        }
        if (SCON_CONTAINER_OF(atom, scon_item_t, atom)->flags & SCON_ITEM_FLAG_FLOAT_SHAPED)
        {
            return SCON_HANDLE_FROM_INT((scon_int64_t)atom->floatValue);
        }
    }

    SCON_ERROR_RUNTIME(scon, "expected int, got %s", scon_item_type_str(item->type));
}

SCON_API scon_handle_t scon_get_float(scon_t* scon, scon_handle_t* handle)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (SCON_HANDLE_IS_FLOAT(handle))
    {
        return *handle;
    }
    if (SCON_HANDLE_IS_INT(handle))
    {
        return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_HANDLE_TO_INT(handle));
    }

    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);
    if (item->flags & SCON_ITEM_FLAG_FLOAT_SHAPED)
    {
        return SCON_HANDLE_FROM_FLOAT(item->atom.floatValue);
    }
    if (item->flags & SCON_ITEM_FLAG_INT_SHAPED)
    {
        return SCON_HANDLE_FROM_FLOAT((scon_float_t)item->atom.integerValue);
    }

    if (item->flags & SCON_ITEM_FLAG_QUOTED)
    {
        scon_atom_t* atom = scon_atom_lookup(scon, item->atom.string, item->length, SCON_ATOM_LOOKUP_NONE);
        if (atom == SCON_NULL)
        {
            SCON_ERROR_RUNTIME(scon, "expected float, got %s", scon_item_type_str(item->type));
        }
        if (SCON_CONTAINER_OF(atom, scon_item_t, atom)->flags & SCON_ITEM_FLAG_FLOAT_SHAPED)
        {
            return SCON_HANDLE_FROM_FLOAT(atom->floatValue);
        }
        if (SCON_CONTAINER_OF(atom, scon_item_t, atom)->flags & SCON_ITEM_FLAG_INT_SHAPED)
        {
            return SCON_HANDLE_FROM_FLOAT((scon_float_t)atom->integerValue);
        }
    }

    SCON_ERROR_RUNTIME(scon, "expected float, got %s", scon_item_type_str(item->type));
}

SCON_API scon_handle_t scon_get_string(scon_t* scon, scon_handle_t* handle)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);

    if (item->type == SCON_ITEM_TYPE_ATOM)
    {
        return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, item->atom.string, item->length, SCON_ATOM_LOOKUP_QUOTED));
    }

    char stackBuffer[SCON_STACK_BUFFER_SIZE];
    scon_size_t len = scon_stringify(scon, handle, stackBuffer, SCON_STACK_BUFFER_SIZE);

    if (len < SCON_STACK_BUFFER_SIZE)
    {
        return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, stackBuffer, len, SCON_ATOM_LOOKUP_QUOTED));
    }

    char* buffer = (char*)SCON_MALLOC(len + 1);
    if (buffer == SCON_NULL)
    {
        SCON_ERROR_INTERNAL(scon, "out of memory");
    }

    scon_stringify(scon, handle, buffer, len + 1);
    scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, len, SCON_ATOM_LOOKUP_QUOTED));
    SCON_FREE(buffer);
    return result;
}

#endif