#ifndef SCON_STDLIB_SYSTEM_IMPL_H
#define SCON_STDLIB_SYSTEM_IMPL_H 1

#include "core.h"
#include "handle.h"
#include "stdlib_system.h"

SCON_API scon_handle_t scon_print(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);

    for (scon_size_t i = 0; i < argc; i++)
    {
        char* str;
        scon_size_t len;
        scon_handle_get_string_params(scon, &argv[i], &str, &len);
        SCON_FWRITE(str, 1, len, SCON_STDOUT);
        if (i < argc - 1)
        {
            SCON_FWRITE(" ", 1, 1, SCON_STDOUT);
        }
    }
    return scon_handle_nil(scon);
}

SCON_API scon_handle_t scon_println(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_t handle = scon_print(scon, argc, argv);
    SCON_FWRITE("\n", 1, 1, SCON_STDOUT);
    return handle;
}

#endif