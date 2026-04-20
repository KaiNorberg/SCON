#ifndef SCON_NATIVE_IMPL_H
#define SCON_NATIVE_IMPL_H 1

#include "atom.h"
#include "core.h"
#include "gc.h"
#include "item.h"
#include "native.h"

SCON_API void scon_native_register(scon_t* scon, scon_native_t* array, scon_size_t count)
{
    for (scon_size_t i = 0; i < count; i++)
    {
        scon_native_t* native = &array[i];

        scon_size_t len = SCON_STRLEN(native->name);
        scon_atom_t* atom = scon_atom_lookup(scon, native->name, len);
        scon_item_t* item = SCON_CONTAINER_OF(atom, scon_item_t, atom);

        item->flags |= SCON_ITEM_FLAG_NATIVE;
        atom->native = native->fn;

        scon_gc_retain_item(scon, item);
    }
}

#endif