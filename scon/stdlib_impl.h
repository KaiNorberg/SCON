#include "atom.h"
#include "stdlib_sequences.h"
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
#include "stdlib_introspection_impl.h"
#include "stdlib_math_impl.h"
#include "stdlib_sequences_impl.h"
#include "stdlib_string_impl.h"
#include "stdlib_system_impl.h"
#include "stdlib_type_casting_impl.h"

static scon_handle_t scon_stdlib_assert(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 2)
    {
        SCON_ERROR_RUNTIME(scon, "assert expects exactly 2 arguments, got %zu", argc);
    }
    return scon_assert(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_throw(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, "throw expects exactly 1 argument, got %zu", argc);
    }
    return scon_throw(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_try(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "try expects exactly 2 arguments, got %zu", argc);
    }
    return scon_try(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_map(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "map expects exactly 2 arguments, got %zu", argc);
    }
    return scon_map(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_filter(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "filter expects exactly 2 arguments, got %zu", argc);
    }
    return scon_filter(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_reduce(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2 && argc != 3)
    {
        SCON_ERROR_RUNTIME(scon, "reduce expects 2 or 3 arguments, got %zu", argc);
    }

    return scon_reduce(scon, &argv[0], argc == 3 ? &argv[1] : SCON_NULL, argc == 3 ? &argv[2] : &argv[1]);
}

static scon_handle_t scon_stdlib_apply(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "apply expects exactly 2 arguments, got %zu", argc);
    }
    return scon_apply(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_any(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1 && argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "any? expects 1 or 2 arguments, got %zu", argc);
    }
    return scon_any(scon, &argv[0], argc == 2 ? &argv[1] : SCON_NULL);
}

static scon_handle_t scon_stdlib_all(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1 && argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "all? expects 1 or 2 arguments, got %zu", argc);
    }
    return scon_all(scon, &argv[0], argc == 2 ? &argv[1] : SCON_NULL);
}

static scon_handle_t scon_stdlib_sort(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1 && argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "sort expects 1 or 2 arguments, got %zu", argc);
    }
    return scon_sort(scon, &argv[0], argc == 2 ? &argv[1] : SCON_NULL);
}

static scon_handle_t scon_stdlib_len(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, "len expects at least 1 argument, got %zu", argc);
    }
    return scon_len(scon, argc, argv);
}

static scon_handle_t scon_stdlib_range(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1 || argc > 3)
    {
        SCON_ERROR_RUNTIME(scon, "range expects 1 to 3 arguments (end, [start], [step]), got %zu", argc);
    }

    scon_handle_t* start = (argc >= 2) ? &argv[0] : SCON_NULL;
    scon_handle_t* end = (argc == 1) ? &argv[0] : (argc >= 2 ? &argv[1] : SCON_NULL);
    scon_handle_t* step = (argc == 3) ? &argv[2] : SCON_NULL;

    if (argc == 1)
    {
        scon_handle_t zero = SCON_HANDLE_FROM_INT(0);
        return scon_range(scon, &zero, end, SCON_NULL);
    }

    return scon_range(scon, start, end, step);
}

static scon_handle_t scon_stdlib_concat(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    return scon_concat(scon, argc, argv);
}

static scon_handle_t scon_stdlib_first(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "first expects exactly 1 argument, got %zu", argc);
    }
    return scon_first(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_last(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "last expects exactly 1 argument, got %zu", argc);
    }
    return scon_last(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_rest(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "rest expects exactly 1 argument, got %zu", argc);
    }
    return scon_rest(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_init(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "init expects exactly 1 argument, got %zu", argc);
    }
    return scon_init(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_nth(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 2 || argc > 3)
    {
        SCON_ERROR_RUNTIME(scon, "nth expects 2 or 3 arguments (list/atom, index, [default]), got %zu", argc);
    }
    return scon_nth(scon, &argv[0], &argv[1], argc == 3 ? &argv[2] : SCON_NULL);
}

static scon_handle_t scon_stdlib_assoc(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 3 || argc > 4)
    {
        SCON_ERROR_RUNTIME(scon, "assoc expects 3 or 4 arguments (list/atom, index, value, [fill]), got %zu", argc);
    }
    return scon_assoc(scon, &argv[0], &argv[1], &argv[2], argc == 4 ? &argv[3] : SCON_NULL);
}

static scon_handle_t scon_stdlib_dissoc(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "dissoc expects exactly 2 arguments (list/atom, index), got %zu", argc);
    }
    return scon_dissoc(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_update(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 3 || argc > 4)
    {
        SCON_ERROR_RUNTIME(scon, "update expects 3 or 4 arguments (list/atom, index, callable, [fill]), got %zu", argc);
    }
    return scon_update(scon, &argv[0], &argv[1], &argv[2], argc == 4 ? &argv[3] : SCON_NULL);
}

static scon_handle_t scon_stdlib_index_of(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "index-of expects exactly 2 arguments (list/atom, value), got %zu", argc);
    }
    return scon_index_of(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_reverse(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "reverse expects exactly 1 argument, got %zu", argc);
    }
    return scon_reverse(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_slice(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 2 || argc > 3)
    {
        SCON_ERROR_RUNTIME(scon, "slice expects 2 or 3 arguments (list/atom, start, [end]), got %zu", argc);
    }
    return scon_slice(scon, &argv[0], &argv[1], argc == 3 ? &argv[2] : SCON_NULL);
}

static scon_handle_t scon_stdlib_flatten(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1 || argc > 2)
    {
        SCON_ERROR_RUNTIME(scon, "flatten expects 1 or 2 arguments (list/atom, [depth]), got %zu", argc);
    }
    return scon_flatten(scon, &argv[0], argc == 2 ? &argv[1] : SCON_NULL);
}

static scon_handle_t scon_stdlib_contains(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "contains? expects exactly 2 arguments (list/atom, value), got %zu", argc);
    }
    return scon_contains(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_replace(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 3)
    {
        SCON_ERROR_RUNTIME(scon, "replace expects exactly 3 arguments (atom, old, new), got %zu", argc);
    }
    return scon_replace(scon, &argv[0], &argv[1], &argv[2]);
}

static scon_handle_t scon_stdlib_unique(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "unique expects exactly 1 argument (list), got %zu", argc);
    }
    return scon_unique(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_chunk(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "chunk expects exactly 2 arguments (list, size), got %zu", argc);
    }
    return scon_chunk(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_find(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "find expects exactly 2 arguments (list, callable), got %zu", argc);
    }
    return scon_find(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_get_in(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 2 || argc > 3)
    {
        SCON_ERROR_RUNTIME(scon, "get-in expects 2 or 3 arguments (list, key, [default]), got %zu", argc);
    }
    return scon_get_in(scon, &argv[0], &argv[1], argc == 3 ? &argv[2] : SCON_NULL);
}

static scon_handle_t scon_stdlib_assoc_in(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 3)
    {
        SCON_ERROR_RUNTIME(scon, "assoc-in expects exactly 3 arguments (list, key, value), got %zu", argc);
    }
    return scon_assoc_in(scon, &argv[0], &argv[1], &argv[2]);
}

static scon_handle_t scon_stdlib_dissoc_in(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "dissoc-in expects exactly 2 arguments (list, key), got %zu", argc);
    }
    return scon_dissoc_in(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_update_in(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 3)
    {
        SCON_ERROR_RUNTIME(scon, "update-in expects exactly 3 arguments (list, key, fn), got %zu", argc);
    }
    return scon_update_in(scon, &argv[0], &argv[1], &argv[2]);
}

static scon_handle_t scon_stdlib_keys(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "keys expects exactly 1 argument (list), got %zu", argc);
    }
    return scon_keys(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_values(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "values expects exactly 1 argument (list), got %zu", argc);
    }
    return scon_values(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_merge(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    return scon_merge(scon, argc, argv);
}

static scon_handle_t scon_stdlib_explode(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    return scon_explode(scon, argc, argv);
}

static scon_handle_t scon_stdlib_implode(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    return scon_implode(scon, argc, argv);
}

static scon_handle_t scon_stdlib_repeat(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "repeat expects exactly 2 arguments (count, value), got %zu", argc);
    }
    return scon_repeat(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_starts_with(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "starts-with? expects exactly 2 arguments (list/atom, prefix), got %zu", argc);
    }
    return scon_starts_with(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_ends_with(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "ends-with? expects exactly 2 arguments (list/atom, suffix), got %zu", argc);
    }
    return scon_ends_with(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_join(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "join expects exactly 2 arguments (list, separator), got %zu", argc);
    }
    return scon_join(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_split(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "split expects exactly 2 arguments (atom, separator), got %zu", argc);
    }
    return scon_split(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_upper(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "upper expects exactly 1 argument, got %zu", argc);
    }
    return scon_upper(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_lower(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "lower expects exactly 1 argument, got %zu", argc);
    }
    return scon_lower(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_trim(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "trim expects exactly 1 argument, got %zu", argc);
    }
    return scon_trim(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_is_atom(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, "atom? expects at least 1 argument, got %zu", argc);
    }
    return scon_is_atom(scon, argc, argv);
}

static scon_handle_t scon_stdlib_is_int(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, "int? expects at least 1 argument, got %zu", argc);
    }
    return scon_is_int(scon, argc, argv);
}

static scon_handle_t scon_stdlib_is_float(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, "float? expects at least 1 argument, got %zu", argc);
    }
    return scon_is_float(scon, argc, argv);
}

static scon_handle_t scon_stdlib_is_number(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, "number? expects at least 1 argument, got %zu", argc);
    }
    return scon_is_number(scon, argc, argv);
}

static scon_handle_t scon_stdlib_is_string(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, "string? expects at least 1 argument, got %zu", argc);
    }
    return scon_is_string(scon, argc, argv);
}

static scon_handle_t scon_stdlib_is_lambda(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, "lambda? expects at least 1 argument, got %zu", argc);
    }
    return scon_is_lambda(scon, argc, argv);
}

static scon_handle_t scon_stdlib_is_native(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, "native? expects at least 1 argument, got %zu", argc);
    }
    return scon_is_native(scon, argc, argv);
}

static scon_handle_t scon_stdlib_is_callable(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, "callable? expects at least 1 argument, got %zu", argc);
    }
    return scon_is_callable(scon, argc, argv);
}

static scon_handle_t scon_stdlib_is_list(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, "list? expects at least 1 argument, got %zu", argc);
    }
    return scon_is_list(scon, argc, argv);
}

static scon_handle_t scon_stdlib_is_empty(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, "empty? expects at least 1 argument, got %zu", argc);
    }
    return scon_is_empty(scon, argc, argv);
}

static scon_handle_t scon_stdlib_is_nil(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, "nil? expects at least 1 argument, got %zu", argc);
    }
    return scon_is_nil(scon, argc, argv);
}

static scon_handle_t scon_stdlib_int(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "int expects exactly 1 argument, got %zu", argc);
    }
    return scon_get_int(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_float(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "float expects exactly 1 argument, got %zu", argc);
    }
    return scon_get_float(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_string(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "string expects exactly 1 argument, got %zu", argc);
    }
    return scon_get_string(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_eval(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "eval expects exactly 1 argument, got %zu", argc);
    }
    
    scon_function_t* function = scon_compile(scon, &argv[0]);
    return scon_eval(scon, function);
}

static scon_handle_t scon_stdlib_parse(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "parse expects exactly 1 argument (atom), got %zu", argc);
    }

    char* str;
    scon_size_t len;
    scon_handle_get_string_params(scon, &argv[0], &str, &len);
    return scon_parse(scon, str, len, "<parse>");
}

static scon_handle_t scon_stdlib_load(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "load! expects exactly 1 argument (path), got %zu", argc);
    }
    return scon_load(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_read_file(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "read-file! expects exactly 1 argument (path), got %zu", argc);
    }
    return scon_read_file(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_write_file(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "write-file! expects exactly 2 arguments (path, content), got %zu", argc);
    }
    return scon_write_file(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_read_char(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 0)
    {
        SCON_ERROR_RUNTIME(scon, "read-char! expects exactly 0 arguments, got %zu", argc);
    }
    return scon_read_char(scon);
}

static scon_handle_t scon_stdlib_read_line(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 0)
    {
        SCON_ERROR_RUNTIME(scon, "read-line! expects exactly 0 arguments, got %zu", argc);
    }
    return scon_read_line(scon);
}

static scon_handle_t scon_stdlib_print(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    return scon_print(scon, argc, argv);
}

static scon_handle_t scon_stdlib_println(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    return scon_println(scon, argc, argv);
}

static scon_handle_t scon_stdlib_ord(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "ord expects exactly 1 argument, got %zu", argc);
    }
    return scon_ord(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_chr(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "chr expects exactly 1 argument, got %zu", argc);
    }
    return scon_chr(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_format(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    return scon_format(scon, argc, argv);
}

static scon_handle_t scon_stdlib_now(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 0)
    {
        SCON_ERROR_RUNTIME(scon, "now! expects exactly 0 arguments, got %zu", argc);
    }
    return scon_now(scon);
}

static scon_handle_t scon_stdlib_uptime(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 0)
    {
        SCON_ERROR_RUNTIME(scon, "uptime! expects exactly 0 arguments, got %zu", argc);
    }
    return scon_uptime(scon);
}

static scon_handle_t scon_stdlib_env(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 0)
    {
        SCON_ERROR_RUNTIME(scon, "env! expects exactly 0 arguments, got %zu", argc);
    }
    return scon_env(scon);
}

SCON_API scon_handle_t scon_stdlib_args(struct scon* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 0)
    {
        SCON_ERROR_RUNTIME(scon, "args! expects exactly 0 arguments, got %zu", argc);
    }
    return scon_args(scon);
}

static scon_handle_t scon_stdlib_min(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, "min expects at least 1 argument, got %zu", argc);
    }
    return scon_min(scon, argc, argv);
}

static scon_handle_t scon_stdlib_max(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1)
    {
        SCON_ERROR_RUNTIME(scon, "max expects at least 1 argument, got %zu", argc);
    }
    return scon_max(scon, argc, argv);
}

static scon_handle_t scon_stdlib_clamp(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 3)
    {
        SCON_ERROR_RUNTIME(scon, "clamp expects exactly 3 arguments, got %zu", argc);
    }
    return scon_clamp(scon, &argv[0], &argv[1], &argv[2]);
}

static scon_handle_t scon_stdlib_abs(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "abs expects exactly 1 argument, got %zu", argc);
    }
    return scon_abs(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_floor(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "floor expects exactly 1 argument, got %zu", argc);
    }
    return scon_floor(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_ceil(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "ceil expects exactly 1 argument, got %zu", argc);
    }
    return scon_ceil(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_round(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "round expects exactly 1 argument, got %zu", argc);
    }
    return scon_round(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_pow(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "pow expects exactly 2 arguments, got %zu", argc);
    }
    return scon_pow(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_exp(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "exp expects exactly 1 argument, got %zu", argc);
    }
    return scon_exp(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_log(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc < 1 || argc > 2)
    {
        SCON_ERROR_RUNTIME(scon, "log expects 1 or 2 arguments, got %zu", argc);
    }
    return scon_log(scon, &argv[0], argc == 2 ? &argv[1] : SCON_NULL);
}

static scon_handle_t scon_stdlib_sqrt(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "sqrt expects exactly 1 argument, got %zu", argc);
    }
    return scon_sqrt(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_sin(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "sin expects exactly 1 argument, got %zu", argc);
    }
    return scon_sin(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_cos(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "cos expects exactly 1 argument, got %zu", argc);
    }
    return scon_cos(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_tan(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "tan expects exactly 1 argument, got %zu", argc);
    }
    return scon_tan(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_asin(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "asin expects exactly 1 argument, got %zu", argc);
    }
    return scon_asin(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_acos(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "acos expects exactly 1 argument, got %zu", argc);
    }
    return scon_acos(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_atan(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "atan expects exactly 1 argument, got %zu", argc);
    }
    return scon_atan(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_atan2(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "atan2 expects exactly 2 arguments, got %zu", argc);
    }
    return scon_atan2(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_sinh(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "sinh expects exactly 1 argument, got %zu", argc);
    }
    return scon_sinh(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_cosh(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "cosh expects exactly 1 argument, got %zu", argc);
    }
    return scon_cosh(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_tanh(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "tanh expects exactly 1 argument, got %zu", argc);
    }
    return scon_tanh(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_asinh(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "asinh expects exactly 1 argument, got %zu", argc);
    }
    return scon_asinh(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_acosh(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "acosh expects exactly 1 argument, got %zu", argc);
    }
    return scon_acosh(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_atanh(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "atanh expects exactly 1 argument, got %zu", argc);
    }
    return scon_atanh(scon, &argv[0]);
}

static scon_handle_t scon_stdlib_rand(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 2)
    {
        SCON_ERROR_RUNTIME(scon, "rand expects exactly 2 arguments, got %zu", argc);
    }
    return scon_rand(scon, &argv[0], &argv[1]);
}

static scon_handle_t scon_stdlib_seed(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    if (argc != 1)
    {
        SCON_ERROR_RUNTIME(scon, "seed! expects exactly 1 argument, got %zu", argc);
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
            {"try", scon_stdlib_try},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
    if (sets & SCON_STDLIB_HIGHER_ORDER)
    {
        scon_native_t natives[] = {
            {"map", scon_stdlib_map},
            {"filter", scon_stdlib_filter},
            {"reduce", scon_stdlib_reduce},
            {"apply", scon_stdlib_apply},
            {"any?", scon_stdlib_any},
            {"all?", scon_stdlib_all},
            {"sort", scon_stdlib_sort},
        };
        scon_native_register(scon, natives, sizeof(natives) / sizeof(scon_native_t));
    }
    if (sets & SCON_STDLIB_SEQUENCES)
    {
        scon_native_t natives[] = {
            {"len", scon_stdlib_len},
            {"range", scon_stdlib_range},
            {"concat", scon_stdlib_concat},
            {"first", scon_stdlib_first},
            {"last", scon_stdlib_last},
            {"rest", scon_stdlib_rest},
            {"init", scon_stdlib_init},
            {"nth", scon_stdlib_nth},
            {"assoc", scon_stdlib_assoc},
            {"dissoc", scon_stdlib_dissoc},
            {"update", scon_stdlib_update},
            {"index-of", scon_stdlib_index_of},
            {"reverse", scon_stdlib_reverse},
            {"slice", scon_stdlib_slice},
            {"flatten", scon_stdlib_flatten},
            {"contains?", scon_stdlib_contains},
            {"replace", scon_stdlib_replace},
            {"unique", scon_stdlib_unique},
            {"chunk", scon_stdlib_chunk},
            {"find", scon_stdlib_find},
            {"get-in", scon_stdlib_get_in},
            {"assoc-in", scon_stdlib_assoc_in},
            {"dissoc-in", scon_stdlib_dissoc_in},
            {"update-in", scon_stdlib_update_in},
            {"keys", scon_stdlib_keys},
            {"values", scon_stdlib_values},
            {"merge", scon_stdlib_merge},
            {"explode", scon_stdlib_explode},
            {"implode", scon_stdlib_implode},
            {"repeat", scon_stdlib_repeat},
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
            {"atom?", scon_stdlib_is_atom},
            {"int?", scon_stdlib_is_int},
            {"float?", scon_stdlib_is_float},
            {"number?", scon_stdlib_is_number},
            {"string?", scon_stdlib_is_string},
            {"lambda?", scon_stdlib_is_lambda},
            {"native?", scon_stdlib_is_native},
            {"callable?", scon_stdlib_is_callable},
            {"list?", scon_stdlib_is_list},
            {"empty?", scon_stdlib_is_empty},
            {"nil?", scon_stdlib_is_nil},
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
    if (sets & SCON_STDLIB_SYSTEM)
    {
        scon_native_t natives[] = {
            {"eval", scon_stdlib_eval},
            {"parse", scon_stdlib_parse},
            {"load!", scon_stdlib_load},
            {"read-file!", scon_stdlib_read_file},
            {"write-file!", scon_stdlib_write_file},
            {"read-char!", scon_stdlib_read_char},
            {"read-line!", scon_stdlib_read_line},
            {"print!", scon_stdlib_print},
            {"println!", scon_stdlib_println},
            {"ord", scon_stdlib_ord},
            {"chr", scon_stdlib_chr},
            {"format", scon_stdlib_format},
            {"now!", scon_stdlib_now},
            {"uptime!", scon_stdlib_uptime},
            {"env!", scon_stdlib_env},
            {"args!", scon_stdlib_args},
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
            {"exp", scon_stdlib_exp},
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
