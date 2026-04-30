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
 * @todo The Garbage collector really needs to be optimized.
 *
 * @{
 */

#define REDUCT_GC_THRESHOLD_INITIAL 32 ///< Initial blocks allocated threshold for garbage collection.

/**
 * @brief Run the garbage collector.
 *
 * @param reduct The Reduct structure.
 */
REDUCT_API void reduct_gc(reduct_t* reduct);

/**
 * @brief Optionally run the garbage collector if the number of allocated blocks exceeds the threshold.

 * @param reduct The Reduct structure.
 */
static inline REDUCT_ALWAYS_INLINE void reduct_gc_if_needed(reduct_t* reduct)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);

    if (REDUCT_UNLIKELY(reduct->blocksAllocated > reduct->gcThreshold))
    {
        reduct_gc(reduct);
        reduct->blocksAllocated = 0;
        reduct->gcThreshold = reduct->blocksAllocated + REDUCT_GC_THRESHOLD_INITIAL;
    }
}

/** @} */

#endif
