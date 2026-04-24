#ifndef SCON_ATOM_IMPL_H
#define SCON_ATOM_IMPL_H 1

#include "atom.h"
#include "char.h"
#include "core.h"

SCON_API void scon_atom_deinit(scon_t* scon, scon_atom_t* atom)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(atom != SCON_NULL);

    scon_size_t bucket = atom->hash % SCON_BUCKETS_MAX;
    scon_atom_t** current = &scon->atomBuckets[bucket];
    while (*current)
    {
        if (*current == atom)
        {
            *current = atom->next;
            break;
        }
        current = &(*current)->next;
    }

    if (atom->length >= SCON_ATOM_SMALL_MAX)
    {
        SCON_FREE(atom->string);
    }
}

SCON_API scon_atom_t* scon_atom_lookup_int(scon_t* scon, scon_int64_t value)
{
    SCON_ASSERT(scon != SCON_NULL);

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
    scon_atom_t* atom = scon_atom_lookup(scon, str, len, SCON_ATOM_LOOKUP_NONE);
    atom->integerValue = value;
    scon_item_t* item = SCON_CONTAINER_OF(atom, scon_item_t, atom);
    item->flags |= SCON_ITEM_FLAG_INT_SHAPED;
    return atom;
}

SCON_API scon_atom_t* scon_atom_lookup_float(scon_t* scon, scon_float_t value)
{
    SCON_ASSERT(scon != SCON_NULL);

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

    scon_atom_t* atom = scon_atom_lookup(scon, res, len, SCON_ATOM_LOOKUP_NONE);
    atom->floatValue = value;
    scon_item_t* item = SCON_CONTAINER_OF(atom, scon_item_t, atom);
    item->flags |= SCON_ITEM_FLAG_FLOAT_SHAPED;
    return atom;
}

SCON_API scon_atom_t* scon_atom_lookup(scon_t* scon, const char* str, scon_size_t len, scon_atom_lookup_flags_t flags)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(str != SCON_NULL);

    scon_size_t hash = scon_hash(str, len);
    if (flags & SCON_ATOM_LOOKUP_QUOTED)
    {
        hash ^= 0x5bd1e995;
    }
    scon_size_t bucket = hash % SCON_BUCKETS_MAX;

    scon_atom_t** current = &scon->atomBuckets[bucket];
    while (*current)
    {
        scon_atom_t* atom = *current;
        scon_item_t* item = SCON_CONTAINER_OF(atom, scon_item_t, atom);
        scon_bool_t isQuoted = (item->flags & SCON_ITEM_FLAG_QUOTED) != 0;
        scon_bool_t wantQuoted = (flags & SCON_ATOM_LOOKUP_QUOTED) != 0;
        if (atom->hash == hash && isQuoted == wantQuoted && scon_atom_is_equal(atom, str, len))
        {
            if (current != &scon->atomBuckets[bucket])
            {
                *current = atom->next;
                atom->next = scon->atomBuckets[bucket];
                scon->atomBuckets[bucket] = atom;
            }
            return atom;
        }
        current = &(*current)->next;
    }

    scon_item_t* item = scon_item_new(scon);
    item->type = SCON_ITEM_TYPE_ATOM;
    if (flags & SCON_ATOM_LOOKUP_QUOTED)
    {
        item->flags |= SCON_ITEM_FLAG_QUOTED;
    }
    scon_atom_t* atom = &item->atom;
    atom->length = len;
    atom->hash = hash;
    atom->intrinsic = SCON_INTRINSIC_NONE;
    atom->next = scon->atomBuckets[bucket];
    scon->atomBuckets[bucket] = atom;

    if (len < SCON_ATOM_SMALL_MAX)
    {
        SCON_MEMCPY(atom->small, str, len);
        atom->small[len] = '\0';
        atom->string = atom->small;
    }
    else
    {
        atom->string = SCON_MALLOC(len + 1);
        if (atom->string == SCON_NULL)
        {
            SCON_ERROR_INTERNAL(scon, "out of memory");
        }
        SCON_MEMCPY(atom->string, str, len);
        atom->string[len] = '\0';
    }

    return atom;
}

static inline void scon_atom_normalize_escape(scon_t* scon, scon_atom_t* atom)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(atom != SCON_NULL);

    char* str = atom->string;
    scon_size_t len = atom->length;
    scon_size_t j = 0;
    for (scon_size_t i = 0; i < len; i++)
    {
        if (str[i] == '\\' && i + 1 < len)
        {
            i++;
            const scon_char_info_t* info = &sconCharTable[str[i]];
            if (info->decodeEscape != 0)
            {
                str[j++] = info->decodeEscape;
                continue;
            }
            else if (str[i] == 'x' && i + 2 < len)
            {
                unsigned char high = sconCharTable[(unsigned char)str[i + 1]].integer;
                unsigned char low = sconCharTable[(unsigned char)str[i + 2]].integer;
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
    if (atom->length < SCON_ATOM_SMALL_MAX)
    {
        str[j] = '\0';
    }
    else
    {
        atom->string[j] = '\0';
    }
}

SCON_API void scon_atom_normalize(scon_t* scon, scon_atom_t* atom)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(atom != SCON_NULL);

    scon_item_t* item = SCON_CONTAINER_OF(atom, scon_item_t, atom);
    if (atom->length == 0)
    {
        item->flags |= SCON_ITEM_FLAG_FALSY;
        return;
    }

    if (item->flags & SCON_ITEM_FLAG_QUOTED)
    {
        scon_atom_normalize_escape(scon, atom);
        return;
    }

    const char* p = atom->string;
    const char* end = p + atom->length;
    const char* start = p;
    int sign = 1;

    if (p < end && (*p == '+' || *p == '-'))
    {
        if (*p == '-')
        {
            sign = -1;
        }
        p++;
    }

    if (p == end)
    {
        return;
    }

    if (end - p == 3)
    {
        if (SCON_CHAR_TO_LOWER(p[0]) == 'i' && SCON_CHAR_TO_LOWER(p[1]) == 'n' && SCON_CHAR_TO_LOWER(p[2]) == 'f')
        {
            item->flags |= SCON_ITEM_FLAG_FLOAT_SHAPED;
            atom->floatValue = sign > 0 ? SCON_INF : -SCON_INF;
            return;
        }
        if (p == start && SCON_CHAR_TO_LOWER(p[0]) == 'n' && SCON_CHAR_TO_LOWER(p[1]) == 'a' &&
            SCON_CHAR_TO_LOWER(p[2]) == 'n')
        {
            item->flags |= SCON_ITEM_FLAG_FLOAT_SHAPED;
            atom->floatValue = SCON_NAN;
            return;
        }
    }

    int base = 10;
    if (p + 1 < end && *p == '0')
    {
        char l = SCON_CHAR_TO_LOWER(p[1]);
        if (l == 'x')
        {
            base = 16;
            p += 2;
        }
        else if (l == 'o')
        {
            base = 8;
            p += 2;
        }
        else if (l == 'b')
        {
            base = 2;
            p += 2;
        }
    }

    scon_bool_t hasDigits = SCON_FALSE;
    scon_bool_t sectionHasDigits = SCON_FALSE;
    scon_bool_t valid = SCON_TRUE;

    if (base != 10)
    {
        scon_uint64_t intValue = 0;
        while (p < end)
        {
            if (*p == '_')
            {
                if (!sectionHasDigits)
                {
                    valid = SCON_FALSE;
                    break;
                }
                p++;
                continue;
            }

            int d = -1;
            unsigned char c = (unsigned char)*p;
            if (SCON_CHAR_IS_HEX_DIGIT(c))
            {
                d = sconCharTable[c].integer;
            }

            if (d >= 0 && d < base)
            {
                intValue = intValue * base + d;
                hasDigits = SCON_TRUE;
                sectionHasDigits = SCON_TRUE;
            }
            else
            {
                valid = SCON_FALSE;
                break;
            }
            p++;
        }
        if (valid && hasDigits && p == end && *(end - 1) != '_')
        {
            item->flags |= SCON_ITEM_FLAG_INT_SHAPED;
            atom->integerValue = sign * (scon_int64_t)intValue;
            if (atom->integerValue == 0)
            {
                item->flags |= SCON_ITEM_FLAG_FALSY;
            }
        }
        return;
    }

    scon_bool_t isFloat = SCON_FALSE;
    scon_uint64_t intValue = 0;
    double floatValue = 0.0;
    double fractionDiv = 10.0;
    scon_bool_t inFraction = SCON_FALSE;
    scon_bool_t inExponent = SCON_FALSE;
    int expSign = 1;
    int expValue = 0;
    int fractionDigits = 0;
    int exponentDigits = 0;

    if (*p == '.')
    {
        isFloat = SCON_TRUE;
        inFraction = SCON_TRUE;
        p++;
    }

    while (p < end)
    {
        if (*p == '_')
        {
            if (!sectionHasDigits)
            {
                valid = SCON_FALSE;
                break;
            }
            p++;
            continue;
        }

        unsigned char c = (unsigned char)*p;
        if (SCON_CHAR_IS_DIGIT(c))
        {
            hasDigits = SCON_TRUE;
            sectionHasDigits = SCON_TRUE;
            if (inExponent)
            {
                expValue = expValue * 10 + sconCharTable[c].integer;
                exponentDigits++;
            }
            else if (inFraction)
            {
                floatValue = floatValue + sconCharTable[c].integer / fractionDiv;
                fractionDiv *= 10.0;
                fractionDigits++;
            }
            else
            {
                intValue = intValue * 10 + sconCharTable[c].integer;
                floatValue = floatValue * 10.0 + sconCharTable[c].integer;
            }
        }
        else if (c == '.' && !inFraction && !inExponent)
        {
            if (*(p - 1) == '_')
            {
                valid = SCON_FALSE;
                break;
            }
            isFloat = SCON_TRUE;
            inFraction = SCON_TRUE;
            sectionHasDigits = SCON_FALSE;
        }
        else if (SCON_CHAR_TO_LOWER(c) == 'e' && !inExponent && hasDigits)
        {
            if (*(p - 1) == '_')
            {
                valid = SCON_FALSE;
                break;
            }
            isFloat = SCON_TRUE;
            inExponent = SCON_TRUE;
            sectionHasDigits = SCON_FALSE;
            p++;
            if (p < end && (*p == '+' || *p == '-'))
            {
                if (*p == '-')
                {
                    expSign = -1;
                }
                p++;
            }
            continue;
        }
        else
        {
            valid = SCON_FALSE;
            break;
        }
        p++;
    }

    if (inExponent && exponentDigits == 0)
    {
        valid = SCON_FALSE;
    }

    if (valid && hasDigits && p == end && *(end - 1) != '_')
    {
        if (isFloat)
        {
            item->flags |= SCON_ITEM_FLAG_FLOAT_SHAPED;
            double finalVal = floatValue;
            if (inExponent && expValue != 0)
            {
                double eMult = 1.0;
                double baseMult = 10.0;
                int e = expValue;
                while (e > 0)
                {
                    if (e % 2 != 0)
                    {
                        eMult *= baseMult;
                    }
                    baseMult *= baseMult;
                    e /= 2;
                }
                if (expSign < 0)
                {
                    finalVal /= eMult;
                }
                else
                {
                    finalVal *= eMult;
                }
            }
            atom->floatValue = sign * finalVal;
            if (atom->floatValue == 0.0)
            {
                item->flags |= SCON_ITEM_FLAG_FALSY;
            }
        }
        else
        {
            item->flags |= SCON_ITEM_FLAG_INT_SHAPED;
            atom->integerValue = sign * (scon_int64_t)intValue;
            if (atom->integerValue == 0)
            {
                item->flags |= SCON_ITEM_FLAG_FALSY;
            }
        }
    }
}

#endif
