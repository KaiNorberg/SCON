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
        if (!SCON_HANDLE_IS_ATOM(&argv[i]))
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
        if (!SCON_HANDLE_IS_INT_SHAPED(&argv[i]))
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
        if (!SCON_HANDLE_IS_FLOAT_SHAPED(&argv[i]))
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
        if (!SCON_HANDLE_IS_NUMBER(&argv[i]))
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
        if (!SCON_HANDLE_IS_LAMBDA(&argv[i]))
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
        if (!SCON_HANDLE_IS_NATIVE(&argv[i]))
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
        if (!SCON_HANDLE_IS_CALLABLE(&argv[i]))
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
        if (!SCON_HANDLE_IS_LIST(&argv[i]))
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