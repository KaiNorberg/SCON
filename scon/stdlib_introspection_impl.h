#ifndef SCON_STDLIB_INTROSPECTION_IMPL_H
#define SCON_STDLIB_INTROSPECTION_IMPL_H 1

#include "core.h"
#include "handle.h"
#include "stdlib_introspection.h"

SCON_API scon_handle_t scon_len(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    scon_size_t total = 0;
    for (scon_size_t i = 0; i < argc; i++)
    {
        scon_handle_ensure_item(scon, &argv[i]);
        total += SCON_HANDLE_TO_ITEM(&argv[i])->length;
    }
    return SCON_HANDLE_FROM_INT(total);
}

SCON_API scon_handle_t scon_is_atom(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    for (scon_size_t i = 0; i < argc; i++)
    {
        if (scon_handle_get_type(scon, &argv[i]) != SCON_ITEM_TYPE_ATOM)
        {
            return SCON_HANDLE_FALSE();
        }
    }
    return SCON_HANDLE_TRUE();
}

SCON_API scon_handle_t scon_is_int(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    for (scon_size_t i = 0; i < argc; i++)
    {
        scon_bool_t isInt = SCON_FALSE;
        if (SCON_HANDLE_IS_INT(&argv[i]))
        {
            isInt = SCON_TRUE;
        }
        else if (SCON_HANDLE_IS_ITEM(&argv[i]))
        {
            if (SCON_HANDLE_GET_FLAGS(&argv[i]) & SCON_ITEM_FLAG_INT_SHAPED)
            {
                isInt = SCON_TRUE;
            }
        }
        if (!isInt)
        {
            return SCON_HANDLE_FALSE();
        }
    }
    return SCON_HANDLE_TRUE();
}

SCON_API scon_handle_t scon_is_float(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    for (scon_size_t i = 0; i < argc; i++)
    {
        scon_bool_t isFloat = SCON_FALSE;
        if (SCON_HANDLE_IS_FLOAT(&argv[i]))
        {
            isFloat = SCON_TRUE;
        }
        else if (SCON_HANDLE_IS_ITEM(&argv[i]))
        {
            if (SCON_HANDLE_GET_FLAGS(&argv[i]) & SCON_ITEM_FLAG_FLOAT_SHAPED)
            {
                isFloat = SCON_TRUE;
            }
        }
        if (!isFloat)
        {
            return SCON_HANDLE_FALSE();
        }
    }
    return SCON_HANDLE_TRUE();
}

SCON_API scon_handle_t scon_is_number(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    for (scon_size_t i = 0; i < argc; i++)
    {
        scon_bool_t isNum = SCON_FALSE;
        if (SCON_HANDLE_IS_INT(&argv[i]) || SCON_HANDLE_IS_FLOAT(&argv[i]))
        {
            isNum = SCON_TRUE;
        }
        else if (SCON_HANDLE_IS_ITEM(&argv[i]))
        {
            scon_uint8_t flags = SCON_HANDLE_GET_FLAGS(&argv[i]);
            if (flags & (SCON_ITEM_FLAG_INT_SHAPED | SCON_ITEM_FLAG_FLOAT_SHAPED))
            {
                isNum = SCON_TRUE;
            }
        }
        if (!isNum)
        {
            return SCON_HANDLE_FALSE();
        }
    }
    return SCON_HANDLE_TRUE();
}

SCON_API scon_handle_t scon_is_lambda(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    for (scon_size_t i = 0; i < argc; i++)
    {
        scon_item_type_t type = scon_handle_get_type(scon, &argv[i]);
        if (type != SCON_ITEM_TYPE_FUNCTION && type != SCON_ITEM_TYPE_CLOSURE)
        {
            return SCON_HANDLE_FALSE();
        }
    }
    return SCON_HANDLE_TRUE();
}

SCON_API scon_handle_t scon_is_native(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    for (scon_size_t i = 0; i < argc; i++)
    {
        if (!SCON_HANDLE_IS_ITEM(&argv[i]) || !(SCON_HANDLE_GET_FLAGS(&argv[i]) & SCON_ITEM_FLAG_NATIVE))
        {
            return SCON_HANDLE_FALSE();
        }
    }
    return SCON_HANDLE_TRUE();
}

SCON_API scon_handle_t scon_is_callable(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    for (scon_size_t i = 0; i < argc; i++)
    {
        scon_item_type_t type = scon_handle_get_type(scon, &argv[i]);
        scon_bool_t is_callable = (type == SCON_ITEM_TYPE_FUNCTION || type == SCON_ITEM_TYPE_CLOSURE ||
                                   (SCON_HANDLE_IS_ITEM(&argv[i]) && (SCON_HANDLE_GET_FLAGS(&argv[i]) & SCON_ITEM_FLAG_NATIVE)));
        if (!is_callable)
        {
            return SCON_HANDLE_FALSE();
        }
    }
    return SCON_HANDLE_TRUE();
}

SCON_API scon_handle_t scon_is_list(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    for (scon_size_t i = 0; i < argc; i++)
    {
        if (scon_handle_get_type(scon, &argv[i]) != SCON_ITEM_TYPE_LIST)
        {
            return SCON_HANDLE_FALSE();
        }
    }
    return SCON_HANDLE_TRUE();
}

SCON_API scon_handle_t scon_is_empty(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    for (scon_size_t i = 0; i < argc; i++)
    {
        scon_handle_ensure_item(scon, &argv[i]);
        if (SCON_HANDLE_TO_ITEM(&argv[i])->length != 0)
        {
            return SCON_HANDLE_FALSE();
        }
    }
    return SCON_HANDLE_TRUE();
}

#endif