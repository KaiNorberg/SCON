#ifndef SCON_STDLIB_SYSTEM_H
#define SCON_STDLIB_SYSTEM_H 1

#include "defs.h"

struct scon;

SCON_API scon_handle_t scon_load(struct scon* scon, scon_handle_t* path);
SCON_API scon_handle_t scon_read_file(struct scon* scon, scon_handle_t* path);
SCON_API scon_handle_t scon_write_file(struct scon* scon, scon_handle_t* path, scon_handle_t* content);
SCON_API scon_handle_t scon_read_char(struct scon* scon);
SCON_API scon_handle_t scon_read_line(struct scon* scon);
SCON_API scon_handle_t scon_print(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_println(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_ord(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_chr(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_format(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_now(struct scon* scon);
SCON_API scon_handle_t scon_uptime(struct scon* scon);
SCON_API scon_handle_t scon_env(struct scon* scon);
SCON_API scon_handle_t scon_args(struct scon* scon);

#endif