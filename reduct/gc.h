#include "defs.h"
#ifndef REDUCT_GC_H
#define REDUCT_GC_H 1

#include "core.h"
#include "handle.h"
#include "item.h"

/**
 * @file gc.h
 * @brief Garbage collection
 * @defgroup gc Garbage Collection
 *
 * @{
 */

/**
 * @brief Run the garbage collector.
 *
 * @param reduct The Reduct structure.
 */
REDUCT_API void reduct_gc(reduct_t* reduct);

/**
 * @brief Optionally run the garbage collector if the free list is low.
 *
 * @param reduct The Reduct structure.
 */
static inline REDUCT_ALWAYS_INLINE void reduct_gc_if_needed(reduct_t* reduct)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);

    if (REDUCT_UNLIKELY(reduct->blockCount * REDUCT_ITEM_BLOCK_MAX * 3 > reduct->freeCount * 4 && reduct->blockCount > reduct->prevBlockCount))
    {
        reduct_gc(reduct);
    }
}

/** @} */

#endif
