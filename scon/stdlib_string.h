#ifndef SCON_STDLIB_STRING_H
#define SCON_STDLIB_STRING_H 1

#include "defs.h"

struct scon;

SCON_API scon_handle_t scon_starts_with(struct scon* scon, scon_handle_t* handle, scon_handle_t* prefix);
SCON_API scon_handle_t scon_ends_with(struct scon* scon, scon_handle_t* handle, scon_handle_t* suffix);
SCON_API scon_handle_t scon_join(struct scon* scon, scon_handle_t* listHandle, scon_handle_t* sepHandle);
SCON_API scon_handle_t scon_split(struct scon* scon, scon_handle_t* srcHandle, scon_handle_t* sepHandle);
SCON_API scon_handle_t scon_upper(struct scon* scon, scon_handle_t* srcHandle);
SCON_API scon_handle_t scon_lower(struct scon* scon, scon_handle_t* srcHandle);
SCON_API scon_handle_t scon_trim(struct scon* scon, scon_handle_t* srcHandle);

#endif