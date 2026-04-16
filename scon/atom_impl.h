#include "char_internal.h"
#include "core_api.h"
#ifndef SCON_ATOM_IMPL_H
#define SCON_ATOM_IMPL_H 1

#include "atom_internal.h"
#include "core_internal.h"

static inline void _scon_atom_init(scon_t* scon, _scon_atom_t* atom)
{
    atom->length = 0;
    atom->next = SCON_NULL;
    atom->hash = 0;
}

static inline void _scon_atom_deinit(scon_t* scon, _scon_atom_t* atom)
{
    if (atom->length >= _SCON_ATOM_SHORT_MAX && atom->longStr != SCON_NULL)
    {
        SCON_FREE(atom->longStr);
        atom->longStr = SCON_NULL;
    }
}

static inline scon_bool_t _scon_atom_is_equal(_scon_atom_t* atom, const char* str, scon_size_t len)
{
    if (atom->length != len)
    {
        return SCON_FALSE;
    }

    if (atom->length < _SCON_ATOM_SHORT_MAX)
    {
        scon_uint64_t value = 0;
        for (scon_size_t i = 0; i < len; i++)
        {
            value |= (scon_uint64_t)((unsigned char)str[i]) << (i * 8ULL);
        }

        return value == atom->shortStr.value;
    }

    return memcmp(str, atom->longStr, len) == 0;
}

static inline _scon_atom_t* _scon_atom_lookup_int(scon_t* scon, scon_int64_t value)
{
    char buf[32];
    scon_size_t len = 0;

    unsigned long long uval;
    scon_bool_t isNegative;
    if (value < 0)
    {
        isNegative = SCON_TRUE;
        uval = (unsigned long long)(-value);
    }
    else
    {
        isNegative = SCON_FALSE;
        uval = (unsigned long long)value;
    }

    if (uval == 0)
    {
        buf[sizeof(buf) - 1 - len++] = '0';
    }
    while (uval > 0)
    {
        buf[sizeof(buf) - 1 - len++] = (char)('0' + (uval % 10));
        uval /= 10;
    }
    if (isNegative)
    {
        buf[sizeof(buf) - 1 - len++] = '-';
    }

    const char* str = buf + sizeof(buf) - len;
    _scon_atom_t* atom = _scon_atom_lookup(scon, str, len);
    atom->integerValue = value;
    _scon_node_t* item = SCON_CONTAINER_OF(atom, _scon_node_t, atom);
    item->flags |= _SCON_NODE_FLAG_INT_SHAPED;
    return atom;
}

static inline _scon_atom_t* _scon_atom_lookup_float(scon_t* scon, scon_float_t value)
{
    char buf[64];
    char sign = 1;
    double val = value;

    if (val < 0)
    {
        sign = -1;
        val = -val;
    }

    val += 0.0000005;
    unsigned long long intPart = (unsigned long long)val;
    double fracPart = val - (double)intPart;
    if (fracPart < 0)
    {
        fracPart = 0;
    }

    scon_size_t len = 0;
    char* p = buf + 32;
    unsigned long long uIntPart = intPart;

    if (uIntPart == 0)
    {
        *--p = '0';
        len++;
    }
    else
    {
        while (uIntPart > 0)
        {
            *--p = (char)('0' + (uIntPart % 10));
            uIntPart /= 10;
            len++;
        }
    }
    if (sign == -1)
    {
        *--p = '-';
        len++;
    }

    char* res = p;
    res[len++] = '.';

    for (int i = 0; i < 6; i++)
    {
        fracPart *= 10.0;
        int digit = (int)fracPart;
        res[len++] = (char)('0' + digit);
        fracPart -= (double)digit;
    }

    while (len > 1 && res[len - 1] == '0' && res[len - 2] != '.')
    {
        len--;
    }

    _scon_atom_t* atom = _scon_atom_lookup(scon, res, len);
    atom->floatValue = value;
    _scon_node_t* item = SCON_CONTAINER_OF(atom, _scon_node_t, atom);
    item->flags |= _SCON_NODE_FLAG_FLOAT_SHAPED;
    return atom;
}

static inline _scon_atom_t* _scon_atom_lookup(scon_t* scon, const char* str, scon_size_t len)
{
    scon_size_t hash = _scon_hash(str, len);
    scon_size_t bucket = hash % _SCON_BUCKETS_MAX;

    _scon_atom_t* current = scon->atomBuckets[bucket];
    while (current)
    {
        _scon_atom_t* atom = current;
        if (atom->hash == hash && _scon_atom_is_equal(atom, str, len))
        {
            return atom;
        }
        current = current->next;
    }

    _scon_node_t* node = _scon_node_new(scon);
    node->type = SCON_ITEM_ATOM;
    _scon_atom_t* atom = &node->atom;
    atom->length = len;
    atom->hash = hash;
    atom->next = scon->atomBuckets[bucket];
    scon->atomBuckets[bucket] = atom;

    if (len < _SCON_ATOM_SHORT_MAX)
    {
        atom->shortStr.value = 0;
        SCON_MEMCPY(atom->shortStr.raw, str, len);
    }
    else
    {
        atom->longStr = SCON_MALLOC(len + 1);
        if (atom->longStr == SCON_NULL)
        {
            SCON_THROW(scon, "out of memory");
        }
        SCON_MEMCPY(atom->longStr, str, len);
        atom->longStr[len] = '\0';
    }

    if (len == 0)
    {
        node->flags |= _SCON_NODE_FLAG_FALSY;
    }

    return atom;
}

static inline void _scon_atom_normalize_escape(scon_t* scon, _scon_atom_t* atom)
{
    char* str = _SCON_ATOM_STRING(atom);
    scon_size_t len = atom->length;
    scon_size_t j = 0;
    for (scon_size_t i = 0; i < len; i++)
    {
        if (str[i] == '\\' && i + 1 < len)
        {
            i++;
            const _scon_char_info_t* info = &_sconCharTable[str[i]];
            if (info->decodeEscape != 0)
            {
                str[j++] = info->decodeEscape;
                continue;
            }
            else if (str[i] == 'x' && i + 2 < len)
            {
                unsigned char high = _sconCharTable[(unsigned char)str[i + 1]].integer;
                unsigned char low = _sconCharTable[(unsigned char)str[i + 2]].integer;
                str[j++] = (char)((high << 4) | low);                
                i += 2;
                continue;
            }
        }
        else
        {
            str[j++] = str[i];
        }
    }
    atom->length = j;
    if (atom->length < _SCON_ATOM_SHORT_MAX)
    {
        str[j] = '\0';
    }
    else
    {
        atom->longStr[j] = '\0';
    }
}

#endif
