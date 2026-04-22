#ifndef SCON_STDLIB_ERROR_IMPL_H
#define SCON_STDLIB_ERROR_IMPL_H 1

#include "core.h"
#include "handle.h"
#include "stdlib_error.h"

SCON_API scon_handle_t scon_assert(scon_t* scon, scon_handle_t* cond, scon_handle_t* msg)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (!SCON_HANDLE_IS_TRUTHY(cond))
    {
        char* str;
        scon_size_t len;
        scon_handle_get_string_params(scon, msg, &str, &len);
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "assert failed: %s", str);
    }

    return *cond;
}

SCON_API scon_handle_t scon_throw(scon_t* scon, scon_handle_t* msg)
{
    SCON_ASSERT(scon != SCON_NULL);

    char* str;
    scon_size_t len;
    scon_handle_get_string_params(scon, msg, &str, &len);
    SCON_ERROR_RUNTIME(scon, SCON_NULL, "throw: %s", str);

    return scon_handle_nil(scon);
}

#endif