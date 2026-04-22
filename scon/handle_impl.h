#ifndef SCON_HANDLE_IMPL_H
#define SCON_HANDLE_IMPL_H 1

#include "core.h"
#include "defs.h"
#include "handle.h"
#include "item.h"
#include "gc.h"
#include "eval.h"
#include "stringify.h"
#include "char.h"

SCON_API void scon_handle_get_string_params(scon_t* scon, scon_handle_t* handle, char** outStr, scon_size_t* outLen)
{
    scon_handle_ensure_item(scon, handle);
    scon_item_t* item = SCON_HANDLE_TO_ITEM(handle);
    if (item->type != SCON_ITEM_TYPE_ATOM)
    {
        SCON_ERROR_RUNTIME(scon, item, "expected atom, got %s", scon_item_type_str(item->type));
    }
    *outStr = item->atom.string;
    *outLen = item->length;
}

SCON_API void scon_handle_ensure_item(scon_t* scon, scon_handle_t* handle)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(handle != SCON_NULL);

    if (SCON_HANDLE_IS_ITEM(handle))
    {
        return;
    }

    if (SCON_HANDLE_IS_INT(handle))
    {
        scon_atom_t* atom = scon_atom_lookup_int(scon, SCON_HANDLE_TO_INT(handle));
        *handle = SCON_HANDLE_FROM_ATOM(atom);
        return;
    }

    scon_atom_t* atom = scon_atom_lookup_float(scon, SCON_HANDLE_TO_FLOAT(handle));
    *handle = SCON_HANDLE_FROM_ITEM(SCON_CONTAINER_OF(atom, scon_item_t, atom));
}

SCON_API scon_item_type_t scon_handle_get_type(scon_t* scon, scon_handle_t* handle)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(handle != SCON_NULL);

    if (!SCON_HANDLE_IS_ITEM(handle))
    {
        return SCON_ITEM_TYPE_ATOM;
    }

    scon_handle_ensure_item(scon, handle);
    return SCON_HANDLE_TO_ITEM(handle)->type;
}

SCON_API void scon_handle_promote(struct scon* scon, scon_handle_t* a, scon_handle_t* b, scon_promotion_t* out)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(a != SCON_NULL);
    SCON_ASSERT(b != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

    if (SCON_HANDLE_IS_INT(a) && SCON_HANDLE_IS_INT(b))
    {
        out->type = SCON_PROMOTION_TYPE_INT;
        out->a.intVal = SCON_HANDLE_TO_INT(a);
        out->b.intVal = SCON_HANDLE_TO_INT(b);
        return;
    }

    if (SCON_HANDLE_IS_FLOAT(a) && SCON_HANDLE_IS_FLOAT(b))
    {
        out->type = SCON_PROMOTION_TYPE_FLOAT;
        out->a.floatVal = SCON_HANDLE_TO_FLOAT(a);
        out->b.floatVal = SCON_HANDLE_TO_FLOAT(b);
        return;
    }

    scon_handle_ensure_item(scon, a);
    scon_handle_ensure_item(scon, b);

    scon_item_t* itemA = SCON_HANDLE_TO_ITEM(a);
    scon_item_t* itemB = SCON_HANDLE_TO_ITEM(b);

    if ((SCON_HANDLE_GET_FLAGS(a) & SCON_ITEM_FLAG_FLOAT_SHAPED) || (SCON_HANDLE_GET_FLAGS(b) & SCON_ITEM_FLAG_FLOAT_SHAPED))
    {
        out->type = SCON_PROMOTION_TYPE_FLOAT;
        if (SCON_HANDLE_GET_FLAGS(a) & SCON_ITEM_FLAG_FLOAT_SHAPED)
        {
            out->a.floatVal = itemA->atom.floatValue;
        }
        else
        {
            out->a.floatVal = (scon_float_t)itemA->atom.integerValue;
        }

        if (SCON_HANDLE_GET_FLAGS(b) & SCON_ITEM_FLAG_FLOAT_SHAPED)
        {
            out->b.floatVal = itemB->atom.floatValue;
        }
        else
        {
            out->b.floatVal = (scon_float_t)itemB->atom.integerValue;
        }
    }
    else if ((SCON_HANDLE_GET_FLAGS(a) & SCON_ITEM_FLAG_INT_SHAPED) && (SCON_HANDLE_GET_FLAGS(b) & SCON_ITEM_FLAG_INT_SHAPED))
    {
        out->type = SCON_PROMOTION_TYPE_INT;
        out->a.intVal = itemA->atom.integerValue;
        out->b.intVal = itemB->atom.integerValue;
    }
    else
    {
        SCON_ERROR_RUNTIME(scon, itemA, "unsupported operand type %s and %s", scon_item_type_str(itemA->type),
            scon_item_type_str(itemB->type));
    }
}

SCON_API scon_bool_t scon_handle_is_equal(scon_t* scon, scon_handle_t* a, scon_handle_t* b)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(a != SCON_NULL);
    SCON_ASSERT(b != SCON_NULL);

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
    SCON_ASSERT(handle != SCON_NULL);
    SCON_ASSERT(out != SCON_NULL);

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
    else if (SCON_HANDLE_GET_FLAGS(handle) & SCON_ITEM_FLAG_FLOAT_SHAPED)
    {
        out->group = 0;
        out->isFloat = SCON_TRUE;
        out->num.f = out->item->atom.floatValue;
    }
    else if (SCON_HANDLE_GET_FLAGS(handle) & SCON_ITEM_FLAG_INT_SHAPED)
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
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(a != SCON_NULL);
    SCON_ASSERT(b != SCON_NULL);

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
            scon_handle_t ha = listA->handles[i];
            scon_handle_t hb = listB->handles[i];
            scon_int64_t cmp = scon_handle_compare(scon, &ha, &hb);
            if (cmp != 0)
            {
                return cmp;
            }
        }
    }

    return (scon_int64_t)lenA - (scon_int64_t)lenB;
}

SCON_API scon_handle_t scon_handle_nil(scon_t* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    return SCON_HANDLE_FROM_ITEM(scon->nilItem);
}

SCON_API scon_handle_t scon_handle_pi(scon_t* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    return SCON_HANDLE_FROM_ITEM(scon->piItem);
}

SCON_API scon_handle_t scon_handle_e(scon_t* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    return SCON_HANDLE_FROM_ITEM(scon->eItem);
}

#endif
