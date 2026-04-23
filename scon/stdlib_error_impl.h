#include "atom.h"
#ifndef SCON_STDLIB_ERROR_IMPL_H
#define SCON_STDLIB_ERROR_IMPL_H 1

#include "core.h"
#include "error.h"
#include "eval.h"
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
        SCON_ERROR_RUNTIME(scon, "assert failed: %s", str);
    }

    return *cond;
}

SCON_API scon_handle_t scon_throw(scon_t* scon, scon_handle_t* msg)
{
    SCON_ASSERT(scon != SCON_NULL);

    char* str;
    scon_size_t len;
    scon_handle_get_string_params(scon, msg, &str, &len);
    SCON_ERROR_RUNTIME(scon, "%s", str);

    return scon_handle_nil(scon);
}

SCON_API scon_handle_t scon_try(scon_t* scon, scon_handle_t* callable, scon_handle_t* catchFn)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (!SCON_HANDLE_IS_CALLABLE(callable))
    {
        SCON_ERROR_RUNTIME(scon, "try expects a callable as the first argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, callable)));
    }

    if (!SCON_HANDLE_IS_CALLABLE(catchFn))
    {
        SCON_ERROR_RUNTIME(scon, "try expects a callable as the second argument, got %s",
            scon_item_type_str(scon_handle_get_type(scon, catchFn)));
    }

    scon_error_t* prev = scon->error;

    scon_error_t error = SCON_ERROR();
    if (SCON_ERROR_CATCH(&error))
    {
        scon_handle_t msg = SCON_HANDLE_FROM_ATOM(
            scon_atom_lookup(scon, error.message, SCON_STRLEN(error.message), SCON_ATOM_LOOKUP_NONE));
        scon_handle_t result = scon_eval_call(scon, *catchFn, 1, &msg);
        scon->error = prev;
        return result;
    }

    scon_handle_t result = scon_eval_call(scon, *callable, 0, SCON_NULL);
    scon->error = prev;
    return result;
}

#endif