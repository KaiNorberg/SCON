#ifndef SCON_STDLIB_HIGHER_ORDER_H
#define SCON_STDLIB_HIGHER_ORDER_H 1

#include "defs.h"

struct scon;

SCON_API scon_handle_t scon_map(struct scon* scon, scon_handle_t* callable, scon_handle_t* listHandle);
SCON_API scon_handle_t scon_filter(struct scon* scon, scon_handle_t* callable, scon_handle_t* listHandle);
SCON_API scon_handle_t scon_reduce(struct scon* scon, scon_handle_t* callable, scon_handle_t* accumulator, scon_handle_t* listHandle);
SCON_API scon_handle_t scon_sort(struct scon* scon, scon_size_t argc, scon_handle_t* argv);

#endif