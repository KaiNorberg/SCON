#ifndef REDUCT_NATIVE_IMPL_H
#define REDUCT_NATIVE_IMPL_H 1

#include "atom.h"
#include "core.h"
#include "intrinsic.h"
#include "native.h"

static inline reduct_bool_t reduct_native_map_grow(reduct_t* reduct)
{
    reduct_size_t oldCapacity = reduct->nativeMapCapacity;
    reduct_native_entry_t* oldMap = reduct->nativeMap;

    reduct->nativeMapCapacity = oldCapacity * REDUCT_NATIVE_MAP_GROWTH;
    reduct->nativeMapMask = reduct->nativeMapCapacity - 1;
    reduct->nativeMap = REDUCT_CALLOC(reduct->nativeMapCapacity, sizeof(reduct_native_entry_t));
    if (reduct->nativeMap == REDUCT_NULL)
    {
        reduct->nativeMapCapacity = oldCapacity;
        reduct->nativeMapMask = oldCapacity - 1;
        reduct->nativeMap = oldMap;
        REDUCT_ERROR_INTERNAL(reduct, "out of memory");
    }

    for (reduct_size_t i = 0; i < oldCapacity; i++)
    {
        if (oldMap[i].name == REDUCT_NULL)
        {
            continue;
        }

        reduct_size_t index = oldMap[i].hash & reduct->nativeMapMask;
        reduct_size_t step = 1;
        while (reduct->nativeMap[index].name != REDUCT_NULL)
        {
            index = (index + step) & reduct->nativeMapMask;
            step++;
        }
        reduct->nativeMap[index] = oldMap[i];
    }

    if (oldMap != REDUCT_NULL)
    {
        REDUCT_FREE(oldMap);
    }
    return REDUCT_TRUE;
}

static inline reduct_native_entry_t* reduct_native_map_insert(reduct_t* reduct, reduct_uint32_t hash,
    const char* name, reduct_size_t len)
{
    if (reduct->nativeMap == REDUCT_NULL)
    {
        reduct->nativeMapCapacity = REDUCT_NATIVE_MAP_INITIAL;
        reduct->nativeMapMask = reduct->nativeMapCapacity - 1;
        reduct->nativeMapSize = 0;
        reduct->nativeMap = REDUCT_CALLOC(reduct->nativeMapCapacity, sizeof(reduct_native_entry_t));
        if (reduct->nativeMap == REDUCT_NULL)
        {
            REDUCT_ERROR_INTERNAL(reduct, "out of memory");
        }
    }

    if (reduct->nativeMapSize * 4 > reduct->nativeMapCapacity * 3)
    {
        reduct_native_map_grow(reduct);
    }

    reduct_size_t index = hash & reduct->nativeMapMask;
    reduct_size_t step = 1;
    while (reduct->nativeMap[index].name != REDUCT_NULL)
    {
        index = (index + step) & reduct->nativeMapMask;
        step++;
    }

    reduct_native_entry_t* entry = &reduct->nativeMap[index];
    entry->hash = hash;
    entry->length = len;
    entry->name = REDUCT_MALLOC(len + 1);
    if (entry->name == REDUCT_NULL)
    {
        REDUCT_ERROR_INTERNAL(reduct, "out of memory");
    }
    REDUCT_MEMCPY(entry->name, name, len);
    entry->name[len] = '\0';

    reduct->nativeMapSize++;
    return entry;
}

REDUCT_API reduct_native_entry_t* reduct_native_map_find(reduct_t* reduct, reduct_uint32_t hash,
    const char* str, reduct_size_t len)
{
    if (reduct->nativeMap == REDUCT_NULL)
    {
        return REDUCT_NULL;
    }

    reduct_size_t index = hash & reduct->nativeMapMask;
    reduct_size_t step = 1;

    while (reduct->nativeMap[index].name != REDUCT_NULL)
    {
        reduct_native_entry_t* entry = &reduct->nativeMap[index];
        if (entry->hash == hash && entry->length == len && REDUCT_MEMCMP(entry->name, str, len) == 0)
        {
            return entry;
        }
        index = (index + step) & reduct->nativeMapMask;
        step++;
    }

    return REDUCT_NULL;
}

REDUCT_API void reduct_native_register(reduct_t* reduct, reduct_native_t* array, reduct_size_t count)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(array != REDUCT_NULL || count == 0);

    for (reduct_size_t i = 0; i < count; i++)
    {
        reduct_native_t* native = &array[i];
        reduct_size_t len = REDUCT_STRLEN(native->name);
        reduct_uint32_t hash = reduct_hash(native->name, len);

        reduct_native_entry_t* entry = reduct_native_map_find(reduct, hash, native->name, len);
        if (entry != REDUCT_NULL)
        {
            entry->nativeFn = native->nativeFn;
            entry->intrinsicFn = native->intrinsicFn;
            continue;
        }

        entry = reduct_native_map_insert(reduct, hash, native->name, len);
        entry->nativeFn = native->nativeFn;
        entry->intrinsicFn = native->intrinsicFn;
    }
}

#endif
