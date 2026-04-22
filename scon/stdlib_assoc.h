#ifndef SCON_STDLIB_ASSOC_H
#define SCON_STDLIB_ASSOC_H 1

#include "defs.h"

struct scon;

SCON_API scon_handle_t scon_get(struct scon* scon, scon_handle_t* list, scon_handle_t* key);
SCON_API scon_handle_t scon_keys(struct scon* scon, scon_handle_t* list);
SCON_API scon_handle_t scon_values(struct scon* scon, scon_handle_t* list);
SCON_API scon_handle_t scon_assoc(struct scon* scon, scon_handle_t* list, scon_handle_t* key, scon_handle_t* val);
SCON_API scon_handle_t scon_dissoc(struct scon* scon, scon_handle_t* list, scon_handle_t* key);
SCON_API scon_handle_t scon_update(struct scon* scon, scon_handle_t* list, scon_handle_t* key, scon_handle_t* callable);

#endif