#ifndef SCON_CLOSURE_IMPL_H
#define SCON_CLOSURE_IMPL_H 1

#include "closure.h"
#include "handle.h"
#include "item.h"

SCON_API void scon_closure_deinit(scon_closure_t* closure)
{
    SCON_ASSERT(closure != SCON_NULL);
    if (closure->constants != closure->smallConstants)
    {
        SCON_FREE(closure->constants);
    }
}

SCON_API scon_closure_t* scon_closure_new(struct scon* scon, scon_function_t* function)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(function != SCON_NULL);

    scon_item_t* item = scon_item_new(scon);
    item->type = SCON_ITEM_TYPE_CLOSURE;
    scon_closure_t* closure = &item->closure;
    closure->function = function;
    if (function->constantCount <= SCON_CLOSURE_SMALL_MAX)
    {
        closure->constants = closure->smallConstants;
    }
    else
    {
        closure->constants = (scon_handle_t*)SCON_MALLOC(sizeof(scon_handle_t) * function->constantCount);
    }

    for (scon_uint16_t i = 0; i < function->constantCount; i++)
    {
        if (function->constants[i].type != SCON_CONST_SLOT_ITEM)
        {
            closure->constants[i] = SCON_HANDLE_NONE;
            continue;
        }

        scon_item_t* item = function->constants[i].item;
        if (item->flags & SCON_ITEM_FLAG_INT_SHAPED)
        {
            closure->constants[i] = SCON_HANDLE_FROM_INT(item->atom.integerValue);
        }
        else if (item->flags & SCON_ITEM_FLAG_FLOAT_SHAPED)
        {
            closure->constants[i] = SCON_HANDLE_FROM_FLOAT(item->atom.floatValue);
        }
        else
        {
            closure->constants[i] = SCON_HANDLE_FROM_ITEM(item);
        }
    }

    return closure;
}

#endif