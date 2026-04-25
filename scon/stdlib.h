#ifndef SCON_STDLIB_H
#define SCON_STDLIB_H 1

#include "defs.h"
#include "item.h"

struct scon;

/**
 * @brief SCON built-in library registration and operations.
 * @defgroup stdlib Stdlib
 * @file stdlib.h
 *
 * Built-in libraries provide a set of pre-defined native functions for use in SCON expressions.
 *
 * @see native
 *
 * @{
 */

/**
 * @brief Built-in library sets.
 */
typedef enum
{
    SCON_STDLIB_ERROR = (1 << 0),
    SCON_STDLIB_HIGHER_ORDER = (1 << 1),
    SCON_STDLIB_SEQUENCES = (1 << 2),
    SCON_STDLIB_STRING = (1 << 3),
    SCON_STDLIB_INTROSPECTION = (1 << 4),
    SCON_STDLIB_TYPE_CASTING = (1 << 5),
    SCON_STDLIB_SYSTEM = (1 << 6),
    SCON_STDLIB_MATH = (1 << 7),
    SCON_STDLIB_ALL = 0xFFFF,
} scon_stdlib_sets_t;

SCON_API scon_handle_t scon_assert(struct scon* scon, scon_handle_t* cond, scon_handle_t* msg);
SCON_API scon_handle_t scon_throw(struct scon* scon, scon_handle_t* msg);
SCON_API scon_handle_t scon_try(struct scon* scon, scon_handle_t* callable, scon_handle_t* catchFn);

SCON_API scon_handle_t scon_map(struct scon* scon, scon_handle_t* list, scon_handle_t* callable);
SCON_API scon_handle_t scon_filter(struct scon* scon, scon_handle_t* list, scon_handle_t* callable);
SCON_API scon_handle_t scon_reduce(struct scon* scon, scon_handle_t* list, scon_handle_t* initial,
    scon_handle_t* callable);
SCON_API scon_handle_t scon_apply(struct scon* scon, scon_handle_t* list, scon_handle_t* callable);
SCON_API scon_handle_t scon_any(struct scon* scon, scon_handle_t* list, scon_handle_t* callable);
SCON_API scon_handle_t scon_all(struct scon* scon, scon_handle_t* list, scon_handle_t* callable);
SCON_API scon_handle_t scon_sort(struct scon* scon, scon_handle_t* list, scon_handle_t* callable);

SCON_API scon_handle_t scon_len(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_range(struct scon* scon, scon_handle_t* start, scon_handle_t* end, scon_handle_t* step);
SCON_API scon_handle_t scon_concat(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_first(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_last(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_rest(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_init(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_nth(struct scon* scon, scon_handle_t* handle, scon_handle_t* index,
    scon_handle_t* defaultVal);
SCON_API scon_handle_t scon_assoc(struct scon* scon, scon_handle_t* handle, scon_handle_t* index, scon_handle_t* value,
    scon_handle_t* fillVal);
SCON_API scon_handle_t scon_dissoc(struct scon* scon, scon_handle_t* handle, scon_handle_t* index);
SCON_API scon_handle_t scon_update(struct scon* scon, scon_handle_t* handle, scon_handle_t* index,
    scon_handle_t* callable, scon_handle_t* fillVal);
SCON_API scon_handle_t scon_index_of(struct scon* scon, scon_handle_t* handle, scon_handle_t* target);
SCON_API scon_handle_t scon_reverse(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_slice(struct scon* scon, scon_handle_t* handle, scon_handle_t* start, scon_handle_t* end);
SCON_API scon_handle_t scon_flatten(struct scon* scon, scon_handle_t* handle, scon_handle_t* depth);
SCON_API scon_handle_t scon_contains(struct scon* scon, scon_handle_t* handle, scon_handle_t* target);
SCON_API scon_handle_t scon_replace(struct scon* scon, scon_handle_t* handle, scon_handle_t* oldVal,
    scon_handle_t* newVal);
SCON_API scon_handle_t scon_unique(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_chunk(struct scon* scon, scon_handle_t* handle, scon_handle_t* size);
SCON_API scon_handle_t scon_find(struct scon* scon, scon_handle_t* handle, scon_handle_t* callable);
SCON_API scon_handle_t scon_get_in(struct scon* scon, scon_handle_t* list, scon_handle_t* path,
    scon_handle_t* defaultVal);
SCON_API scon_handle_t scon_assoc_in(struct scon* scon, scon_handle_t* list, scon_handle_t* path, scon_handle_t* val);
SCON_API scon_handle_t scon_dissoc_in(struct scon* scon, scon_handle_t* list, scon_handle_t* path);
SCON_API scon_handle_t scon_update_in(struct scon* scon, scon_handle_t* list, scon_handle_t* path,
    scon_handle_t* callable);
SCON_API scon_handle_t scon_keys(struct scon* scon, scon_handle_t* list);
SCON_API scon_handle_t scon_values(struct scon* scon, scon_handle_t* list);
SCON_API scon_handle_t scon_merge(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_explode(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_implode(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_repeat(struct scon* scon, scon_handle_t* handle, scon_handle_t* count);

SCON_API scon_handle_t scon_starts_with(struct scon* scon, scon_handle_t* handle, scon_handle_t* prefix);
SCON_API scon_handle_t scon_ends_with(struct scon* scon, scon_handle_t* handle, scon_handle_t* suffix);
SCON_API scon_handle_t scon_join(struct scon* scon, scon_handle_t* listHandle, scon_handle_t* sepHandle);
SCON_API scon_handle_t scon_split(struct scon* scon, scon_handle_t* srcHandle, scon_handle_t* sepHandle);
SCON_API scon_handle_t scon_upper(struct scon* scon, scon_handle_t* srcHandle);
SCON_API scon_handle_t scon_lower(struct scon* scon, scon_handle_t* srcHandle);
SCON_API scon_handle_t scon_trim(struct scon* scon, scon_handle_t* srcHandle);

SCON_API scon_handle_t scon_is_atom(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_is_int(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_is_float(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_is_number(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_is_string(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_is_lambda(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_is_native(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_is_callable(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_is_list(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_is_empty(struct scon* scon, scon_size_t argc, scon_handle_t* argv);
SCON_API scon_handle_t scon_is_nil(struct scon* scon, scon_size_t argc, scon_handle_t* argv);

SCON_API scon_handle_t scon_get_int(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_get_float(struct scon* scon, scon_handle_t* handle);
SCON_API scon_handle_t scon_get_string(struct scon* scon, scon_handle_t* handle);

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

/**
 * @brief Register a set from the standard library to the SCON instance.
 *
 * @param scon The SCON structure.
 * @param sets The sets to register.
 */
SCON_API void scon_stdlib_register(struct scon* scon, scon_stdlib_sets_t sets);

/** @} */

#endif
