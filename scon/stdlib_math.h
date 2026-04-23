#ifndef SCON_STDLIB_MATH_H
#define SCON_STDLIB_MATH_H 1

#include "defs.h"

struct scon;

SCON_API scon_handle_t scon_min(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_max(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_clamp(struct scon* scon, scon_handle_t* val, scon_handle_t* minVal, scon_handle_t* maxVal);
SCON_API scon_handle_t scon_abs(struct scon* scon, scon_handle_t* val);
SCON_API scon_handle_t scon_floor(struct scon* scon, scon_handle_t* val);
SCON_API scon_handle_t scon_ceil(struct scon* scon, scon_handle_t* val);
SCON_API scon_handle_t scon_round(struct scon* scon, scon_handle_t* val);
SCON_API scon_handle_t scon_pow(struct scon* scon, scon_handle_t* base, scon_handle_t* exp);
SCON_API scon_handle_t scon_exp(struct scon* scon, scon_handle_t* val);
SCON_API scon_handle_t scon_log(struct scon* scon, scon_handle_t* val, scon_handle_t* base);
SCON_API scon_handle_t scon_sqrt(struct scon* scon, scon_handle_t* val);
SCON_API scon_handle_t scon_sin(struct scon* scon, scon_handle_t* val);
SCON_API scon_handle_t scon_cos(struct scon* scon, scon_handle_t* val);
SCON_API scon_handle_t scon_tan(struct scon* scon, scon_handle_t* val);
SCON_API scon_handle_t scon_asin(struct scon* scon, scon_handle_t* val);
SCON_API scon_handle_t scon_acos(struct scon* scon, scon_handle_t* val);
SCON_API scon_handle_t scon_atan(struct scon* scon, scon_handle_t* val);
SCON_API scon_handle_t scon_atan2(struct scon* scon, scon_handle_t* y, scon_handle_t* x);
SCON_API scon_handle_t scon_sinh(struct scon* scon, scon_handle_t* val);
SCON_API scon_handle_t scon_cosh(struct scon* scon, scon_handle_t* val);
SCON_API scon_handle_t scon_tanh(struct scon* scon, scon_handle_t* val);
SCON_API scon_handle_t scon_asinh(struct scon* scon, scon_handle_t* val);
SCON_API scon_handle_t scon_acosh(struct scon* scon, scon_handle_t* val);
SCON_API scon_handle_t scon_atanh(struct scon* scon, scon_handle_t* val);
SCON_API scon_handle_t scon_rand(struct scon* scon, scon_handle_t* minVal, scon_handle_t* maxVal);
SCON_API scon_handle_t scon_seed(struct scon* scon, scon_handle_t* val);

#endif
