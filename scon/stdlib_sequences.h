#ifndef SCON_STDLIB_SEQUENCES_H
#define SCON_STDLIB_SEQUENCES_H 1

#include "defs.h"

struct scon;

SCON_API scon_handle_t scon_len(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_range(struct scon* scon, scon_handle_t* start, scon_handle_t* end, scon_handle_t* step);
SCON_API scon_handle_t scon_concat(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_first(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_last(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_rest(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_init(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_nth(scon_t* scon, scon_handle_t* handle, scon_handle_t* index, scon_handle_t* defaultVal);
SCON_API scon_handle_t scon_assoc(struct scon* scon, scon_handle_t* handle, scon_handle_t* index, scon_handle_t* value);
SCON_API scon_handle_t scon_dissoc(struct scon* scon, scon_handle_t* handle, scon_handle_t* index);
SCON_API scon_handle_t scon_update(struct scon* scon, scon_handle_t* handle, scon_handle_t* index, scon_handle_t* callable);
SCON_API scon_handle_t scon_index_of(struct scon* scon, scon_handle_t* handle, scon_handle_t* target);
SCON_API scon_handle_t scon_reverse(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_slice(struct scon* scon, scon_handle_t* handle, scon_handle_t* start, scon_handle_t* end);
SCON_API scon_handle_t scon_flatten(struct scon* scon, scon_handle_t* handle, scon_handle_t* depth);
SCON_API scon_handle_t scon_contains(struct scon* scon, scon_handle_t* handle, scon_handle_t* target);
SCON_API scon_handle_t scon_replace(struct scon* scon, scon_handle_t* handle, scon_handle_t* oldVal, scon_handle_t* newVal);
SCON_API scon_handle_t scon_unique(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_chunk(struct scon* scon, scon_handle_t* handle, scon_handle_t* size);
SCON_API scon_handle_t scon_find(struct scon* scon, scon_handle_t* handle, scon_handle_t* callable);
SCON_API scon_handle_t scon_get_in(struct scon* scon, scon_handle_t* list, scon_handle_t* path, scon_handle_t* defaultVal);
SCON_API scon_handle_t scon_assoc_in(struct scon* scon, scon_handle_t* list, scon_handle_t* path, scon_handle_t* val);
SCON_API scon_handle_t scon_dissoc_in(struct scon* scon, scon_handle_t* list, scon_handle_t* path);
SCON_API scon_handle_t scon_update_in(struct scon* scon, scon_handle_t* list, scon_handle_t* path,
    scon_handle_t* callable);
SCON_API scon_handle_t scon_keys(struct scon* scon, scon_handle_t* list);
SCON_API scon_handle_t scon_values(struct scon* scon, scon_handle_t* list);
SCON_API scon_handle_t scon_merge(struct scon* scon, scon_size_t argc, scon_handle_t* argv);

#endif