#ifndef SCON_BUILTIN_IMPL_H
#define SCON_BUILTIN_IMPL_H 1

#include "builtin.h"
#include "core.h"
#include "defs.h"
#include "handle.h"
#include "item.h"
#include "native.h"

static scon_handle_t scon_builtin_assert(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 2)
    {
        SCON_THROW(scon, "assert requires exactly 2 arguments");
    }

    if (!SCON_HANDLE_IS_TRUTHY(scon, &argv[0]))
    {
        char* str;
        scon_size_t len;
        scon_handle_get_string(scon, &argv[1], &str, &len);
        SCON_THROW(scon, str);
    }

    return argv[0];
}

static scon_handle_t scon_builtin_throw(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_THROW(scon, "throw requires exactly 1 argument");
    }

    char* str;
    scon_size_t len;
    scon_handle_get_string(scon, &argv[0], &str, &len);
    SCON_THROW(scon, str);

    return scon_handle_nil(scon); /* Unreachable */
}

static scon_handle_t scon_builtin_print(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    for (scon_size_t i = 0; i < argc; i++)
    {
        char* str;
        scon_size_t len;
        scon_handle_get_string(scon, &argv[i], &str, &len);
        SCON_FWRITE(str, 1, len, SCON_STDOUT);
        if (i < argc - 1)
        {
            SCON_FWRITE(" ", 1, 1, SCON_STDOUT);
        }
    }
    SCON_FWRITE("\n", 1, 1, SCON_STDOUT);
    return scon_handle_nil(scon);
}

static scon_handle_t scon_builtin_println(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    return scon_builtin_print(scon, argc, argv);
}

SCON_API void scon_builtin_register(scon_t* scon, scon_builtins_t builtins)
{
    if (builtins & SCON_BUILTIN_ERROR)
    {
        scon_native_t natives[] = {
            {"assert", scon_builtin_assert},
            {"throw", scon_builtin_throw},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
    if (builtins & SCON_BUILTIN_SYSTEM)
    {
        scon_native_t natives[] = {
            {"print", scon_builtin_print},
            {"println", scon_builtin_println},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
}

#endif