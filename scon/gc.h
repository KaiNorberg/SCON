#ifndef SCON_GC_H
#define SCON_GC_H 1

#include "core.h"
#include "handle.h"
#include "item.h"

/**
 * @brief Garbage collection
 * @defgroup gc Garbage Collection
 * @file gc.h
 *
 * @todo The Garbage collector really needs to be optimized.
 *
 * @{
 */

/**
 * @brief Run the garbage collector.
 *
 * @param scon The SCON structure.
 */
SCON_API void scon_gc(scon_t* scon);

/**
 * @brief Optionally run the garbage collector if the number of allocated blocks exceeds the threshold.

 * @param scon The SCON structure.
 */
SCON_API void scon_gc_if_needed(scon_t* scon);

/**
 * @brief Retain an item, preventing it from being collected by the GC.
 *
 * @param scon The SCON structure.
 * @param item The item to retain, must be a item.
 */
#define SCON_GC_RETAIN(_scon, _handle) \
    do \
    { \
        (void)(_scon); \
        scon_handle_t __handle = (_handle); \
        if (SCON_HANDLE_IS_ITEM(&__handle)) \
        { \
            scon_item_t* __item = SCON_HANDLE_TO_ITEM(&__handle); \
            __item->retainCount++; \
        } \
    } while (0)

/**
 * @brief Retain a item, preventing it from being collected by the GC.
 *
 * @param scon The SCON structure.
 * @param item The item to retain.
 */
#define SCON_GC_RETAIN_ITEM(_scon, _item) \
    do \
    { \
        (void)(_scon); \
        scon_item_t* __item = (_item); \
        __item->retainCount++; \
    } while (0)

/**
 * @brief Release a previously retained item, allowing it to be collected by the GC.
 *
 * @param scon The SCON structure.
 * @param item The item to release.
 */
#define SCON_GC_RELEASE(_scon, _handle) \
    do \
    { \
        (void)(_scon); \
        scon_handle_t __handle = (_handle); \
        if (SCON_HANDLE_IS_ITEM(&__handle)) \
        { \
            scon_item_t* __item = SCON_HANDLE_TO_ITEM(&__handle); \
            __item->retainCount--; \
        } \
    } while (0)

/** @} */

#endif
