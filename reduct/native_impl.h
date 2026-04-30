#ifndef REDUCT_NATIVE_IMPL_H
#define REDUCT_NATIVE_IMPL_H 1

#include "atom.h"
#include "core.h"
#include "gc.h"
#include "item.h"
#include "native.h"

REDUCT_API void reduct_native_register(reduct_t* reduct, reduct_native_t* array, reduct_size_t count)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(array != REDUCT_NULL || count == 0);

    for (reduct_size_t i = 0; i < count; i++)
    {
        reduct_native_t* native = &array[i];

        reduct_size_t len = REDUCT_STRLEN(native->name);
        reduct_atom_t* atom = reduct_atom_lookup(reduct, native->name, len, REDUCT_ATOM_LOOKUP_NONE);

        atom->flags |= REDUCT_ATOM_FLAG_NATIVE;
        atom->native = native->fn;
    }
}

#endif