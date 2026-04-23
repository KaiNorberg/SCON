#ifndef SCON_STDLIB_HIGHER_ORDER_H
#define SCON_STDLIB_HIGHER_ORDER_H 1

#include "defs.h"

struct scon;

SCON_API scon_handle_t scon_map(struct scon* scon, scon_handle_t* list, scon_handle_t* callable);
SCON_API scon_handle_t scon_filter(struct scon* scon, scon_handle_t* list, scon_handle_t* callable);
SCON_API scon_handle_t scon_reduce(struct scon* scon, scon_handle_t* list, scon_handle_t* initial,
    scon_handle_t* callable);
SCON_API scon_handle_t scon_apply(struct scon* scon, scon_handle_t* list, scon_handle_t* callable);
SCON_API scon_handle_t scon_any(struct scon* scon, scon_handle_t* list, scon_handle_t* callable);
SCON_API scon_handle_t scon_all(struct scon* scon, scon_handle_t* list, scon_handle_t* callable);
SCON_API scon_handle_t scon_sort(struct scon* scon, scon_handle_t* list, scon_handle_t* callable);

#endif