#ifndef SCON_STDLIB_SYSTEM_H
#define SCON_STDLIB_SYSTEM_H 1

#include "defs.h"

struct scon;

SCON_API scon_handle_t scon_print(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_println(struct scon* scon, scon_size_t argc, scon_handle_t* argv);

#endif