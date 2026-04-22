#ifndef SCON_STDLIB_TYPE_CASTING_H
#define SCON_STDLIB_TYPE_CASTING_H 1

#include "defs.h"

struct scon;

SCON_API scon_handle_t scon_get_int(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_get_float(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_get_string(struct scon* scon, scon_handle_t* handle);

#endif