#ifndef SCON_STDLIB_SYSTEM_H
#define SCON_STDLIB_SYSTEM_H 1

#include "defs.h"

struct scon;

SCON_API scon_handle_t scon_include(struct scon* scon, scon_handle_t path);
SCON_API scon_handle_t scon_read_file(struct scon* scon, scon_handle_t path);
SCON_API scon_handle_t scon_print(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_println(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_format(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_time(struct scon* scon);
SCON_API scon_handle_t scon_clock(struct scon* scon);
SCON_API scon_handle_t scon_env(struct scon* scon, scon_handle_t* nameHandle);

#endif