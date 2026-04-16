#ifndef SCON_ATOM_INTERNAL_H
#define SCON_ATOM_INTERNAL_H 1

#include "core_api.h"

#define _SCON_ATOM_SHORT_MAX sizeof(scon_size_t)

typedef struct _scon_atom
{
    scon_uint32_t length;
    union {
        union {
            char raw[_SCON_ATOM_SHORT_MAX];
            scon_size_t value;
        } shortStr;
        char* longStr;
    };
    union {
        scon_int64_t integerValue;
        scon_float_t floatValue;
    };
    struct _scon_atom* next;
    scon_size_t hash;
} _scon_atom_t;

#define _SCON_ATOM_STRING(_atom) ((_atom)->length < _SCON_ATOM_SHORT_MAX ? (_atom)->shortStr.raw : (_atom)->longStr)

#define _SCON_FNV_OFFSET 14695981039346656037ULL
#define _SCON_FNV_PRIME 1099511628211ULL

static inline scon_size_t _scon_hash(const char* str, scon_size_t len)
{
    scon_size_t hash = _SCON_FNV_OFFSET;
    for (scon_size_t i = 0; i < len; i++)
    {
        hash ^= (unsigned char)str[i];
        hash *= _SCON_FNV_PRIME;
    }
    return hash;
}

static inline void _scon_atom_init(scon_t* scon, _scon_atom_t* atom);

static inline void _scon_atom_deinit(scon_t* scon, _scon_atom_t* atom);

static inline scon_bool_t _scon_atom_is_equal(_scon_atom_t* atom, const char* str, scon_size_t len);

static inline _scon_atom_t* _scon_atom_lookup_int(scon_t* scon, scon_int64_t value);

static inline _scon_atom_t* _scon_atom_lookup_float(scon_t* scon, scon_float_t value);

static inline _scon_atom_t* _scon_atom_lookup(scon_t* scon, const char* str, scon_size_t len);

static inline void _scon_atom_normalize_escape(scon_t* scon, _scon_atom_t* atom);

#endif
