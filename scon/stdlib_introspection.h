#ifndef SCON_STDLIB_INTROSPECTION_H
#define SCON_STDLIB_INTROSPECTION_H 1

#include "defs.h"

struct scon;

SCON_API scon_handle_t scon_len(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_is_atom(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_is_int(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_is_float(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_is_number(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_is_lambda(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_is_native(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_is_callable(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_is_list(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_is_empty(struct scon* scon, scon_size_t argc, scon_handle_t* argv);

#endif