#ifndef SCON_CLOSURE_IMPL_H
#define SCON_CLOSURE_IMPL_H 1

#include "closure.h"
#include "handle.h"
#include "item.h"

SCON_API void scon_closure_deinit(scon_closure_t* closure)
{
    SCON_FREE(closure->constants);
}

SCON_API scon_closure_t* scon_closure_new(struct scon* scon, scon_function_t* function)
{
    scon_item_t* item = scon_item_new(scon);
    item->type = SCON_ITEM_TYPE_CLOSURE;
    scon_closure_t* closure = &item->closure;
    closure->function = function;
    closure->constants = (scon_handle_t*)SCON_CALLOC(function->constantCount, sizeof(scon_handle_t));

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