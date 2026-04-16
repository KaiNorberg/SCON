#ifndef SCON_GC_API_H
#define SCON_GC_API_H 1

#include "core_api.h"
#include "item_api.h"

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
 * @param item The item to retain.
 */
SCON_API void scon_gc_retain(scon_t* scon, scon_item_t item);

/**
 * @brief Release a previously retained item, allowing it to be collected by the GC.
 *
 * @param scon The SCON structure.
 * @param item The item to release.
 */
SCON_API void scon_gc_release(scon_t* scon, scon_item_t item);

#endif