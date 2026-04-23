#ifndef SCON_STDLIB_SEQUENCES_H
#define SCON_STDLIB_SEQUENCES_H 1

#include "defs.h"

struct scon;

SCON_API scon_handle_t scon_concat(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_first(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_last(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_rest(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_init(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_nth(struct scon* scon, scon_handle_t* handle, scon_handle_t* index);
SCON_API scon_handle_t scon_index(struct scon* scon, scon_handle_t* handle, scon_handle_t* target);
SCON_API scon_handle_t scon_reverse(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_slice(struct scon* scon, scon_handle_t* handle, scon_handle_t* start, scon_handle_t* end);

#endif