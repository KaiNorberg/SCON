#ifndef SCON_FUNCTION_IMPL_H
#define SCON_FUNCTION_IMPL_H 1

#include "core.h"
#include "function.h"
#include "gc.h"
#include "handle.h"
#include "item.h"

SCON_API void scon_function_init(scon_function_t* func)
{
    SCON_ASSERT(func != SCON_NULL);

    func->insts = SCON_NULL;
    func->positions = SCON_NULL;
    func->constants = SCON_NULL;
    func->instCount = 0;
    func->instCapacity = 0;
    func->constantCount = 0;
    func->constantCapacity = 0;
    func->registerCount = 0;
    func->arity = 0;
}

SCON_API void scon_function_deinit(scon_function_t* func)
{
    SCON_ASSERT(func != SCON_NULL);

    if (func->insts != SCON_NULL)
    {
        SCON_FREE(func->insts);
    }
    if (func->positions != SCON_NULL)
    {
        SCON_FREE(func->positions);
    }
    if (func->constants != SCON_NULL)
    {
        SCON_FREE(func->constants);
    }
}

SCON_API scon_function_t* scon_function_new(scon_t* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_item_t* item = scon_item_new(scon);
    item->type = SCON_ITEM_TYPE_FUNCTION;
    scon_function_t* func = &item->function;
    scon_function_init(func);
    return func;
}

SCON_API void scon_function_grow(scon_t* scon, scon_function_t* func)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(func != SCON_NULL);

    scon_size_t newCapacity = func->instCapacity == 0 ? 16 : func->instCapacity * 2;
    scon_inst_t* newInsts = (scon_inst_t*)SCON_REALLOC(func->insts, newCapacity * sizeof(scon_inst_t));
    scon_uint32_t* newPositions = (scon_uint32_t*)SCON_REALLOC(func->positions, newCapacity * sizeof(scon_uint32_t));

    if (newInsts == SCON_NULL || newPositions == SCON_NULL)
    {
        SCON_ERROR_INTERNAL(scon, "out of memory");
    }

    func->insts = newInsts;
    func->positions = newPositions;
    func->instCapacity = newCapacity;
}

SCON_API scon_const_t scon_function_lookup_constant(scon_t* scon, scon_function_t* func, scon_const_slot_t* slot)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(func != SCON_NULL);
    SCON_ASSERT(slot != SCON_NULL);

    for (scon_const_t i = 0; i < func->constantCount; i++)
    {
        if (func->constants[i].type == SCON_CONST_SLOT_CAPTURE && func->constants[i].raw == slot->raw)
        {
            return i;
        }
    }

    if (func->constantCount >= func->constantCapacity)
    {
        scon_uint32_t newCapacity = func->constantCapacity == 0 ? 16 : func->constantCapacity * 2;
        if (newCapacity > SCON_CONSTANT_MAX)
        {
            SCON_ERROR_RUNTIME(scon, "too many constants in function");
        }
        scon_const_slot_t* newConstants = SCON_REALLOC(func->constants, newCapacity * sizeof(scon_const_slot_t));
        if (newConstants == SCON_NULL)
        {
            SCON_ERROR_INTERNAL(scon, "out of memory");
        }
        func->constants = newConstants;
        func->constantCapacity = newCapacity;
    }

    func->constants[func->constantCount] = *slot;
    return func->constantCount++;
}

#endif
