#include "atom.h"
#ifndef SCON_STDLIB_IMPL_H
#define SCON_STDLIB_IMPL_H 1

#include "core.h"
#include "defs.h"
#include "eval.h"
#include "gc.h"
#include "handle.h"
#include "item.h"
#include "native.h"
#include "stdlib.h"

#include "stdlib_error_impl.h"
#include "stdlib_higher_order_impl.h"
#include "stdlib_sequences_impl.h"
#include "stdlib_string_impl.h"
#include "stdlib_introspection_impl.h"
#include "stdlib_type_casting_impl.h"
#include "stdlib_assoc_impl.h"
#include "stdlib_system_impl.h"
#include "stdlib_math_impl.h"

static scon_handle_t scon_stdlib_assert(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "assert expects exactly 2 arguments, got %zu", argc);
    }
    return scon_assert(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_throw(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "throw expects exactly 1 argument, got %zu", argc);
    }
    return scon_throw(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_map(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "map expects exactly 2 arguments, got %zu", argc);
    }
    return scon_map(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_filter(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "filter expects exactly 2 arguments, got %zu", argc);
    }
    return scon_filter(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_reduce(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 3)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "reduce expects exactly 3 arguments (fn, initial, list), got %zu", argc);
    }
    return scon_reduce(scon, &argv[0], &argv[1], &argv[2]);
}

static scon_handle_t scon_stdlib_sort(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1 && argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "sort expects 1 or 2 arguments, got %zu", argc);
    }
    return scon_sort(scon, &argv[0], argc == 2 ? &argv[1] : SCON_NULL);
}

static scon_handle_t scon_stdlib_concat(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    return scon_concat(scon, argc, argv);
}

static scon_handle_t scon_stdlib_first(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "first expects exactly 1 argument, got %zu", argc);
    }
    return scon_first(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_last(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "last expects exactly 1 argument, got %zu", argc);
    }
    return scon_last(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_rest(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "rest expects exactly 1 argument, got %zu", argc);
    }
    return scon_rest(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_init(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "init expects exactly 1 argument, got %zu", argc);
    }
    return scon_init(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_nth(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "nth expects exactly 2 arguments (list/atom, index), got %zu", argc);
    }
    return scon_nth(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_index(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "index expects exactly 2 arguments (list/atom, value), got %zu", argc);
    }
    return scon_index(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_reverse(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "reverse expects exactly 1 argument, got %zu", argc);
    }
    return scon_reverse(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_slice(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 2 || argc > 3)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "slice expects 2 or 3 arguments (list/atom, start, [end]), got %zu", argc);
    }
    return scon_slice(scon, &argv[0], &argv[1], argc == 3 ? &argv[2] : SCON_NULL);
}

static scon_handle_t scon_stdlib_starts_with(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "starts-with? expects exactly 2 arguments (list/atom, prefix), got %zu", argc);
    }
    return scon_starts_with(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_ends_with(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "ends-with? expects exactly 2 arguments (list/atom, suffix), got %zu", argc);
    }
    return scon_ends_with(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_contains(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "contains? expects exactly 2 arguments (list/atom, value), got %zu", argc);
    }
    return scon_contains(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_replace(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{   
    if (argc != 3)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "replace expects exactly 3 arguments (atom, old, new), got %zu", argc);
    }
    return scon_replace(scon, &argv[0], &argv[1], &argv[2]);
}

static scon_handle_t scon_stdlib_join(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "join expects exactly 2 arguments (list, separator), got %zu", argc);
    }
    return scon_join(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_split(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "split expects exactly 2 arguments (atom, separator), got %zu", argc);
    }
    return scon_split(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_upper(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "upper expects exactly 1 argument, got %zu", argc);
    }
    return scon_upper(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_lower(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "lower expects exactly 1 argument, got %zu", argc);
    }
    return scon_lower(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_trim(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "trim expects exactly 1 argument, got %zu", argc);
    }
    return scon_trim(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_len(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "len expects at least 1 argument, got %zu", argc);
    }
    return scon_len(scon, argc, argv);
}

static scon_handle_t scon_stdlib_is_atom(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "atom? expects at least 1 argument, got %zu", argc);
    }
    return scon_is_atom(scon, argc, argv);
}

static scon_handle_t scon_stdlib_is_int(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "int? expects at least 1 argument, got %zu", argc);
    }
    return scon_is_int(scon, argc, argv);
}

static scon_handle_t scon_stdlib_is_float(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "float? expects at least 1 argument, got %zu", argc);
    }
    return scon_is_float(scon, argc, argv);
}

static scon_handle_t scon_stdlib_is_number(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "number? expects at least 1 argument, got %zu", argc);
    }
    return scon_is_number(scon, argc, argv);
}

static scon_handle_t scon_stdlib_is_lambda(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "lambda? expects at least 1 argument, got %zu", argc);
    }
    return scon_is_lambda(scon, argc, argv);
}

static scon_handle_t scon_stdlib_is_native(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "native? expects at least 1 argument, got %zu", argc);
    }
    return scon_is_native(scon, argc, argv);
}

static scon_handle_t scon_stdlib_is_callable(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "callable? expects at least 1 argument, got %zu", argc);
    }
    return scon_is_callable(scon, argc, argv);
}

static scon_handle_t scon_stdlib_is_list(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "list? expects at least 1 argument, got %zu", argc);
    }
    return scon_is_list(scon, argc, argv);
}

static scon_handle_t scon_stdlib_is_empty(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "empty? expects at least 1 argument, got %zu", argc);
    }
    return scon_is_empty(scon, argc, argv);
}

static scon_handle_t scon_stdlib_int(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "int expects exactly 1 argument, got %zu", argc);
    }
    return scon_get_int(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_float(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "float expects exactly 1 argument, got %zu", argc);
    }
    return scon_get_float(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_string(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "string expects exactly 1 argument, got %zu", argc);
    }
    return scon_get_string(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_get(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "get expects exactly 2 arguments (list, key), got %zu", argc);
    }
    return scon_get(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_keys(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "keys expects exactly 1 argument (list), got %zu", argc);
    }
    return scon_keys(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_values(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "values expects exactly 1 argument (list), got %zu", argc);
    }
    return scon_values(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_assoc(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 3)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "assoc expects exactly 3 arguments (list, key, value), got %zu", argc);
    }
    return scon_assoc(scon, &argv[0], &argv[1], &argv[2]);
}

static scon_handle_t scon_stdlib_dissoc(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "dissoc expects exactly 2 arguments (list, key), got %zu", argc);
    }
    return scon_dissoc(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_update(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 3)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "update expects exactly 3 arguments (list, key, fn), got %zu", argc);
    }
    return scon_update(scon, &argv[0], &argv[1], &argv[2]);
}

static scon_handle_t scon_stdlib_include(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "include expects exactly 1 argument (path), got %zu", argc);
    }
    return scon_include(scon, argv[0]);
}

static scon_handle_t scon_stdlib_read_file(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "read-file expects exactly 1 argument (path), got %zu", argc);
    }
    return scon_read_file(scon, argv[0]);
}

static scon_handle_t scon_stdlib_print(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    return scon_print(scon, argc, argv);
}

static scon_handle_t scon_stdlib_println(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    return scon_println(scon, argc, argv);
}

static scon_handle_t scon_stdlib_format(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    return scon_format(scon, argc, argv);
}

static scon_handle_t scon_stdlib_time(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 0)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "time expects exactly 0 arguments, got %zu", argc);
    }
    return scon_time(scon);
}

static scon_handle_t scon_stdlib_clock(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 0)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "clock expects exactly 0 arguments, got %zu", argc);
    }
    return scon_clock(scon);
}

static scon_handle_t scon_stdlib_env(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "getenv expects exactly 1 argument (name), got %zu", argc);
    }
    return scon_env(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_min(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "min expects at least 1 argument, got %zu", argc);
    }
    return scon_min(scon, argc, argv);
}

static scon_handle_t scon_stdlib_max(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "max expects at least 1 argument, got %zu", argc);
    }
    return scon_max(scon, argc, argv);
}

static scon_handle_t scon_stdlib_clamp(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 3)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "clamp expects exactly 3 arguments, got %zu", argc);
    }
    return scon_clamp(scon, &argv[0], &argv[1], &argv[2]);
}

static scon_handle_t scon_stdlib_abs(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "abs expects exactly 1 argument, got %zu", argc);
    }
    return scon_abs(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_floor(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "floor expects exactly 1 argument, got %zu", argc);
    }
    return scon_floor(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_ceil(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "ceil expects exactly 1 argument, got %zu", argc);
    }
    return scon_ceil(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_round(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "round expects exactly 1 argument, got %zu", argc);
    }
    return scon_round(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_pow(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "pow expects exactly 2 arguments, got %zu", argc);
    }
    return scon_pow(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_log(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1 || argc > 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "log expects 1 or 2 arguments, got %zu", argc);
    }
    return scon_log(scon, &argv[0], argc == 2 ? &argv[1] : SCON_NULL);
}

static scon_handle_t scon_stdlib_sqrt(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "sqrt expects exactly 1 argument, got %zu", argc);
    }
    return scon_sqrt(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_sin(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "sin expects exactly 1 argument, got %zu", argc);
    }
    return scon_sin(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_cos(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "cos expects exactly 1 argument, got %zu", argc);
    }
    return scon_cos(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_tan(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "tan expects exactly 1 argument, got %zu", argc);
    }
    return scon_tan(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_asin(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "asin expects exactly 1 argument, got %zu", argc);
    }
    return scon_asin(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_acos(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "acos expects exactly 1 argument, got %zu", argc);
    }
    return scon_acos(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_atan(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "atan expects exactly 1 argument, got %zu", argc);
    }
    return scon_atan(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_atan2(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "atan2 expects exactly 2 arguments, got %zu", argc);
    }
    return scon_atan2(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_sinh(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "sinh expects exactly 1 argument, got %zu", argc);
    }
    return scon_sinh(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_cosh(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "cosh expects exactly 1 argument, got %zu", argc);
    }
    return scon_cosh(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_tanh(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "tanh expects exactly 1 argument, got %zu", argc);
    }
    return scon_tanh(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_asinh(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "asinh expects exactly 1 argument, got %zu", argc);
    }
    return scon_asinh(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_acosh(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "acosh expects exactly 1 argument, got %zu", argc);
    }
    return scon_acosh(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_atanh(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "atanh expects exactly 1 argument, got %zu", argc);
    }
    return scon_atanh(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_rand(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "rand expects exactly 2 arguments, got %zu", argc);
    }
    return scon_rand(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_seed(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "seed! expects exactly 1 argument, got %zu", argc);
    }
    return scon_seed(scon, &argv[0]);
}

SCON_API void scon_stdlib_register(scon_t* scon, scon_stdlib_sets_t sets)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (sets & SCON_STDLIB_ERROR)
    {
        scon_native_t natives[] = {
            {"assert!", scon_stdlib_assert},
            {"throw!", scon_stdlib_throw},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
    if (sets & SCON_STDLIB_HIGHER_ORDER)
    {
        scon_native_t natives[] = {
            {"map", scon_stdlib_map},
            {"filter", scon_stdlib_filter},
            {"reduce", scon_stdlib_reduce},
            {"sort", scon_stdlib_sort},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
    if (sets & SCON_STDLIB_SEQUENCES)
    {
        scon_native_t natives[] = {
            {"concat", scon_stdlib_concat},
            {"first", scon_stdlib_first},
            {"last", scon_stdlib_last},
            {"rest", scon_stdlib_rest},
            {"init", scon_stdlib_init},
            {"nth", scon_stdlib_nth},
            {"index", scon_stdlib_index},
            {"reverse", scon_stdlib_reverse},
            {"slice", scon_stdlib_slice},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
    if (sets & SCON_STDLIB_STRING)
    {
        scon_native_t natives[] = {
            {"starts-with?", scon_stdlib_starts_with},
            {"ends-with?", scon_stdlib_ends_with},
            {"contains?", scon_stdlib_contains},
            {"replace", scon_stdlib_replace},
            {"join", scon_stdlib_join},
            {"split", scon_stdlib_split},
            {"upper", scon_stdlib_upper},
            {"lower", scon_stdlib_lower},
            {"trim", scon_stdlib_trim},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
    if (sets & SCON_STDLIB_INTROSPECTION)
    {
        scon_native_t natives[] = {
            {"len", scon_stdlib_len},
            {"atom?", scon_stdlib_is_atom},
            {"int?", scon_stdlib_is_int},
            {"float?", scon_stdlib_is_float},
            {"number?", scon_stdlib_is_number},
            {"lambda?", scon_stdlib_is_lambda},
            {"native?", scon_stdlib_is_native},
            {"callable?", scon_stdlib_is_callable},
            {"list?", scon_stdlib_is_list},
            {"empty?", scon_stdlib_is_empty},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
    if (sets & SCON_STDLIB_TYPE_CASTING)
    {
        scon_native_t natives[] = {
            {"int", scon_stdlib_int},
            {"float", scon_stdlib_float},
            {"string", scon_stdlib_string},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
    if (sets & SCON_STDLIB_ASSOC)
    {
        scon_native_t natives[] = {
            {"get", scon_stdlib_get},
            {"keys", scon_stdlib_keys},
            {"values", scon_stdlib_values},
            {"assoc", scon_stdlib_assoc},
            {"dissoc", scon_stdlib_dissoc},
            {"update", scon_stdlib_update},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
    if (sets & SCON_STDLIB_SYSTEM)
    {
        scon_native_t natives[] = {
            {"include", scon_stdlib_include},
            {"read-file", scon_stdlib_read_file},
            {"print!", scon_stdlib_print},
            {"println!", scon_stdlib_println},
            {"format", scon_stdlib_format},
            {"time", scon_stdlib_time},
            {"clock", scon_stdlib_clock},
            {"env", scon_stdlib_env},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
    if (sets & SCON_STDLIB_MATH)
    {
        scon_native_t natives[] = {
            {"min", scon_stdlib_min},
            {"max", scon_stdlib_max},
            {"clamp", scon_stdlib_clamp},
            {"abs", scon_stdlib_abs},
            {"floor", scon_stdlib_floor},
            {"ceil", scon_stdlib_ceil},
            {"round", scon_stdlib_round},
            {"pow", scon_stdlib_pow},
            {"log", scon_stdlib_log},
            {"sqrt", scon_stdlib_sqrt},
            {"sin", scon_stdlib_sin},
            {"cos", scon_stdlib_cos},
            {"tan", scon_stdlib_tan},
            {"asin", scon_stdlib_asin},
            {"acos", scon_stdlib_acos},
            {"atan", scon_stdlib_atan},
            {"atan2", scon_stdlib_atan2},
            {"sinh", scon_stdlib_sinh},
            {"cosh", scon_stdlib_cosh},
            {"tanh", scon_stdlib_tanh},
            {"asinh", scon_stdlib_asinh},
            {"acosh", scon_stdlib_acosh},
            {"atanh", scon_stdlib_atanh},
            {"rand", scon_stdlib_rand},
            {"seed!", scon_stdlib_seed},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
}

#endif
