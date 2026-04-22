#ifndef SCON_STDLIB_ERROR_H
#define SCON_STDLIB_ERROR_H 1

#include "defs.h"

struct scon;

SCON_API scon_handle_t scon_assert(struct scon* scon, scon_handle_t* cond, scon_handle_t* msg);
SCON_API scon_handle_t scon_throw(struct scon* scon, scon_handle_t* msg);

#endif