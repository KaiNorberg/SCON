#include "defs.h"
#ifndef SCON_HANDLE_IMPL_H
#define SCON_HANDLE_IMPL_H 1

#include "core.h"
#include "handle.h"
#include "item.h"

static void scon_handle_ensure_item(scon_t* scon, scon_handle_t* handle)
{
    if (SCON_HANDLE_IS_ITEM(handle))
    {
        return;
    }

    if (SCON_HANDLE_IS_INT(handle))
    {
        scon_atom_t* atom = scon_atom_lookup_int(scon, SCON_HANDLE_TO_INT(handle));
        *handle = SCON_HANDLE_FROM_ATOM(atom);
    }
    else if (SCON_HANDLE_IS_FLOAT(handle))
    {
        scon_atom_t* atom = scon_atom_lookup_float(scon, SCON_HANDLE_TO_FLOAT(handle));
        *handle = SCON_HANDLE_FROM_ITEM(SCON_CONTAINER_OF(atom, scon_item_t, atom));
    }
    else
    {
        SCON_THROW(scon, "invalid item type");
    }
}

SCON_API scon_size_t scon_handle_len(scon_t* scon, scon_handle_t* handle)
{
    scon_handle_ensure_item(scon, handle);
    return SCON_HANDLE_TO_ITEM(handle)->length;
}

SCON_API scon_item_type_t scon_handle_get_type(scon_t* scon, scon_handle_t* handle)
{
    if (!SCON_HANDLE_IS_ITEM(handle))
    {
        return SCON_ITEM_TYPE_ATOM;
    }

    scon_handle_ensure_item(scon, handle);
    return SCON_HANDLE_TO_ITEM(handle)->type;
}

SCON_API void scon_handle_get_string(scon_t* scon, scon_handle_t* handle, char** out, scon_size_t* len)
{
    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);
    if (item->type != SCON_ITEM_TYPE_ATOM)
    {
        SCON_THROW(scon, "invalid item type");
    }

    *out = item->atom.string;
    *len = item->length;
}

SCON_API scon_int64_t scon_handle_get_int(scon_t* scon, scon_handle_t* handle)
{
    if (SCON_HANDLE_IS_INT(handle))
    {
        return SCON_HANDLE_TO_INT(handle);
    }

    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);
    if (!(item->flags & SCON_ITEM_FLAG_INT_SHAPED))
    {
        SCON_THROW(scon, "invalid item type");
    }

    return item->atom.integerValue;
}

SCON_API scon_float_t scon_handle_get_float(scon_t* scon, scon_handle_t* handle)
{
    if (SCON_HANDLE_IS_FLOAT(handle))
    {
        return SCON_HANDLE_TO_FLOAT(handle);
    }

    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);
    if (!(item->flags & SCON_ITEM_FLAG_FLOAT_SHAPED))
    {
        SCON_THROW(scon, "invalid item type");
    }

    return item->atom.floatValue;
}

SCON_API scon_bool_t scon_handle_is_number(scon_t* scon, scon_handle_t* handle)
{
    if (SCON_HANDLE_IS_INT(handle) || SCON_HANDLE_IS_FLOAT(handle))
    {
        return SCON_TRUE;
    }

    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);
    return (item->flags & (SCON_ITEM_FLAG_INT_SHAPED | SCON_ITEM_FLAG_FLOAT_SHAPED)) != 0;
}

SCON_API scon_bool_t scon_handle_is_int(scon_t* scon, scon_handle_t* handle)
{
    if (SCON_HANDLE_IS_INT(handle))
    {
        return SCON_TRUE;
    }

    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);
    return (item->flags & SCON_ITEM_FLAG_INT_SHAPED) != 0;
}

SCON_API scon_bool_t scon_handle_is_float(scon_t* scon, scon_handle_t* handle)
{
    if (SCON_HANDLE_IS_FLOAT(handle))
    {
        return SCON_TRUE;
    }

    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);
    return (item->flags & SCON_ITEM_FLAG_FLOAT_SHAPED) != 0;
}

SCON_API scon_bool_t scon_handle_is_truthy(scon_t* scon, scon_handle_t* handle)
{
    if (SCON_HANDLE_IS_ITEM(handle))
    {
        scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);
        return !(item->flags & SCON_ITEM_FLAG_FALSY);
    }

    if (SCON_HANDLE_IS_INT(handle))
    {
        return SCON_HANDLE_TO_INT(handle) != 0;
    }

    if (SCON_HANDLE_IS_FLOAT(handle))
    {
        return SCON_HANDLE_TO_FLOAT(handle) != 0.0;
    }

    return SCON_FALSE;
}

SCON_API scon_bool_t scon_handle_is_equal(scon_t* scon, scon_handle_t* a, scon_handle_t* b)
{
    scon_handle_ensure_item(scon, a);
    scon_handle_ensure_item(scon, b);

    return *a == *b;
}

typedef struct
{
    int group;
    scon_bool_t isFloat;
    union {
        scon_int64_t i;
        scon_float_t f;
    } num;
    scon_item_t* item;
} scon_cmp_val_t;

static inline void scon_handle_unpack(scon_handle_t* handle, scon_cmp_val_t* out)
{
    if (SCON_HANDLE_IS_INT(handle))
    {
        out->group = 0;
        out->isFloat = SCON_FALSE;
        out->num.i = SCON_HANDLE_TO_INT(handle);
        out->item = SCON_NULL;
        return;
    }

    if (SCON_HANDLE_IS_FLOAT(handle))
    {
        out->group = 0;
        out->isFloat = SCON_TRUE;
        out->num.f = SCON_HANDLE_TO_FLOAT(handle);
        out->item = SCON_NULL;
        return;
    }

    out->item = SCON_HANDLE_TO_ITEM(handle);
    if (out->item == SCON_NULL)
    {
        out->group = 1;
        return;
    }

    if (out->item->type == SCON_ITEM_TYPE_LIST)
    {
        out->group = 2;
    }
    else if (out->item->flags & SCON_ITEM_FLAG_FLOAT_SHAPED)
    {
        out->group = 0;
        out->isFloat = SCON_TRUE;
        out->num.f = out->item->atom.floatValue;
    }
    else if (out->item->flags & SCON_ITEM_FLAG_INT_SHAPED)
    {
        out->group = 0;
        out->isFloat = SCON_FALSE;
        out->num.i = out->item->atom.integerValue;
    }
    else
    {
        out->group = 1;
    }
}

SCON_API scon_int64_t scon_handle_compare(scon_t* scon, scon_handle_t* a, scon_handle_t* b)
{
    if (a == b || *a == *b)
    {
        return 0;
    }

    scon_cmp_val_t va, vb;
    scon_handle_unpack(a, &va);
    scon_handle_unpack(b, &vb);

    if (va.group != vb.group)
    {
        return va.group - vb.group;
    }

    if (va.group == 0)
    {
        if (!va.isFloat && !vb.isFloat)
        {
            return (va.num.i < vb.num.i) ? -1 : ((va.num.i > vb.num.i) ? 1 : 0);
        }
        else if (va.isFloat && vb.isFloat)
        {
            return (va.num.f < vb.num.f) ? -1 : ((va.num.f > vb.num.f) ? 1 : 0);
        }

        scon_float_t fa = va.isFloat ? va.num.f : (scon_float_t)va.num.i;
        scon_float_t fb = vb.isFloat ? vb.num.f : (scon_float_t)vb.num.i;
        if (fa < fb)
        {
            return -1;
        }
        if (fa > fb)
        {
            return 1;
        }
        return va.isFloat ? 1 : -1;
    }
    else if (va.group == 1)
    {
        scon_atom_t* atomA = va.item ? &va.item->atom : SCON_NULL;
        scon_atom_t* atomB = vb.item ? &vb.item->atom : SCON_NULL;
        scon_size_t lenA = atomA ? atomA->length : 0;
        scon_size_t lenB = atomB ? atomB->length : 0;
        scon_size_t minLen = lenA < lenB ? lenA : lenB;

        if (minLen > 0)
        {
            int cmp = memcmp(atomA->string, atomB->string, minLen);
            if (cmp != 0)
            {
                return cmp;
            }
        }

        return (scon_int64_t)lenA - (scon_int64_t)lenB;
    }

    scon_list_t* listA = va.item ? &va.item->list : SCON_NULL;
    scon_list_t* listB = vb.item ? &vb.item->list : SCON_NULL;
    scon_size_t lenA = listA ? listA->length : 0;
    scon_size_t lenB = listB ? listB->length : 0;
    scon_size_t minLen = lenA < lenB ? lenA : lenB;

    if (minLen > 0)
    {
        for (scon_size_t i = 0; i < minLen; i++)
        {
            scon_handle_t ha = SCON_HANDLE_FROM_ITEM(listA->items[i]);
            scon_handle_t hb = SCON_HANDLE_FROM_ITEM(listB->items[i]);
            scon_int64_t cmp = scon_handle_compare(scon, &ha, &hb);
            if (cmp != 0)
            {
                return cmp;
            }
        }
    }

    return (scon_int64_t)lenA - (scon_int64_t)lenB;
}

SCON_API scon_handle_t scon_handle_get(scon_t* scon, scon_handle_t* list, const char* name)
{
    if (!SCON_HANDLE_IS_ITEM(list))
    {
        return SCON_HANDLE_NONE;
    }

    scon_item_t* item = SCON_HANDLE_TO_ITEM(list);
    if (item->type != SCON_ITEM_TYPE_LIST)
    {
        return SCON_HANDLE_NONE;
    }

    scon_size_t strLen = SCON_STRLEN(name);
    if (strLen == 0)
    {
        return SCON_HANDLE_NONE;
    }

    scon_list_t* l = &item->list;
    scon_size_t len = l->length;
    for (scon_size_t i = 0; i < len; i++)
    {
        scon_item_t* item = l->items[i];
        if (item->type != SCON_ITEM_TYPE_LIST || item->list.length < 2)
        {
            continue;
        }

        scon_item_t* key = item->list.items[0];
        if (key->type == SCON_ITEM_TYPE_ATOM && key->length == strLen)
        {
            if (SCON_MEMCMP(key->atom.string, name, strLen) == 0)
            {
                return SCON_HANDLE_FROM_ITEM(key->list.items[1]);
            }
        }
    }

    return SCON_HANDLE_NONE;
}

SCON_API scon_handle_t scon_handle_nth(scon_t* scon, scon_handle_t* list, scon_size_t n)
{
    if (!SCON_HANDLE_IS_ITEM(list))
    {
        return SCON_HANDLE_NONE;
    }

    scon_item_t* item = SCON_HANDLE_TO_ITEM(list);
    if (item->type != SCON_ITEM_TYPE_LIST)
    {
        return SCON_HANDLE_NONE;
    }

    if (n >= item->list.length)
    {
        return SCON_HANDLE_NONE;
    }

    return SCON_HANDLE_FROM_ITEM(item->list.items[n]);
}

SCON_API scon_handle_t scon_handle_true(scon_t* scon)
{
    return SCON_HANDLE_FROM_ITEM(scon->trueItem);
}

SCON_API scon_handle_t scon_handle_false(scon_t* scon)
{
    return SCON_HANDLE_FROM_ITEM(scon->falseItem);
}

SCON_API scon_handle_t scon_handle_nil(scon_t* scon)
{
    return SCON_HANDLE_FROM_ITEM(scon->nilItem);
}

SCON_API scon_handle_t scon_handle_pi(scon_t* scon)
{
    return SCON_HANDLE_FROM_ITEM(scon->piItem);
}

SCON_API scon_handle_t scon_handle_e(scon_t* scon)
{
    return SCON_HANDLE_FROM_ITEM(scon->eItem);
}

#endif
