#ifndef SCON_FUNCTION_IMPL_H
#define SCON_FUNCTION_IMPL_H 1

#include "core.h"
#include "function.h"
#include "gc.h"
#include "handle.h"
#include "item.h"

SCON_API void scon_function_init(scon_function_t* func)
{
    func->insts = func->small;
    func->constants = SCON_NULL;
    func->instCount = 0;
    func->instCapacity = SCON_FUNCTION_SMALL_MAX;
    func->constantCount = 0;
    func->constantCapacity = 0;
    func->arity = 0;
}

SCON_API void scon_function_deinit(scon_function_t* func)
{
    if (func->insts != func->small)
    {
        SCON_FREE(func->insts);
    }
    if (func->constants != SCON_NULL)
    {
        SCON_FREE(func->constants);
    }
}

SCON_API scon_function_t* scon_function_new(scon_t* scon)
{
    scon_item_t* item = scon_item_new(scon);
    item->type = SCON_ITEM_TYPE_FUNCTION;
    scon_function_t* func = &item->function;
    scon_function_init(func);
    return func;
}

SCON_API void scon_function_grow(scon_t* scon, scon_function_t* func)
{
    scon_size_t newCapacity = func->instCapacity * 2;
    scon_inst_t* newInsts;

    if (func->insts == func->small)
    {
        newInsts = (scon_inst_t*)SCON_MALLOC(newCapacity * sizeof(scon_inst_t));
        if (newInsts != SCON_NULL)
        {
            SCON_MEMCPY(newInsts, func->small, func->instCount * sizeof(scon_inst_t));
        }
    }
    else
    {
        newInsts = (scon_inst_t*)SCON_REALLOC(func->insts, newCapacity * sizeof(scon_inst_t));
    }

    if (newInsts == SCON_NULL)
    {
        SCON_THROW(scon, "out of memory");
    }

    func->insts = newInsts;
    func->instCapacity = newCapacity;
}

SCON_API scon_const_t scon_function_lookup_constant(scon_t* scon, scon_function_t* func, scon_const_slot_t* slot)
{
    for (scon_const_t i = 0; i < func->constantCount; i++)
    {
        if (func->constants[i].type == SCON_CONST_SLOT_CAPTURE && func->constants[i].raw == slot->raw)
        {
            return i;
        }
    }

    if (func->constantCount >= func->constantCapacity)
    {
        scon_uint32_t newCap = func->constantCapacity == 0 ? 16 : func->constantCapacity * 2;
        if (newCap > SCON_CONSTANT_MAX)
        {
            SCON_THROW(scon, "too many constants in function");
        }
        scon_const_slot_t* newConsts = SCON_REALLOC(func->constants, newCap * sizeof(scon_const_slot_t*));
        if (newConsts == SCON_NULL)
        {
            SCON_THROW(scon, "out of memory");
        }
        func->constants = newConsts;
        func->constantCapacity = newCap;
    }

    func->constants[func->constantCount] = *slot;
    return func->constantCount++;
}

SCON_API scon_const_t scon_const_true(struct scon* scon, scon_function_t* func)
{
    scon_const_slot_t slot = SCON_CONST_SLOT_ITEM(scon->trueItem);
    return scon_function_lookup_constant(scon, func, &slot);
}

SCON_API scon_const_t scon_const_false(struct scon* scon, scon_function_t* func)
{
    scon_const_slot_t slot = SCON_CONST_SLOT_ITEM(scon->falseItem);
    return scon_function_lookup_constant(scon, func, &slot);
}

SCON_API scon_const_t scon_const_nil(struct scon* scon, scon_function_t* func)
{
    scon_const_slot_t slot = SCON_CONST_SLOT_ITEM(scon->nilItem);
    return scon_function_lookup_constant(scon, func, &slot);
}

SCON_API scon_const_t scon_const_pi(struct scon* scon, scon_function_t* func)
{
    scon_const_slot_t slot = SCON_CONST_SLOT_ITEM(scon->piItem);
    return scon_function_lookup_constant(scon, func, &slot);
}

SCON_API scon_const_t scon_const_e(struct scon* scon, scon_function_t* func)
{
    scon_const_slot_t slot = SCON_CONST_SLOT_ITEM(scon->eItem);
    return scon_function_lookup_constant(scon, func, &slot);
}

#endif
