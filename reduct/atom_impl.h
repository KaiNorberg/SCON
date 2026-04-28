#ifndef REDUCT_ATOM_IMPL_H
#define REDUCT_ATOM_IMPL_H 1

#include "atom.h"
#include "char.h"
#include "core.h"

REDUCT_API void reduct_atom_deinit(reduct_t* reduct, reduct_atom_t* atom)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(atom != REDUCT_NULL);

    reduct_size_t bucket = atom->hash % REDUCT_BUCKETS_MAX;
    reduct_atom_t** current = &reduct->atomBuckets[bucket];
    while (*current)
    {
        if (*current == atom)
        {
            *current = atom->next;
            break;
        }
        current = &(*current)->next;
    }

    if (atom->string != atom->small)
    {
        REDUCT_FREE(atom->string);
    }
}

REDUCT_API reduct_atom_t* reduct_atom_lookup_int(reduct_t* reduct, reduct_int64_t value)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);

    char buf[32];
    reduct_size_t len = 0;

    unsigned long long uval;
    reduct_bool_t isNegative;
    if (value < 0)
    {
        isNegative = REDUCT_TRUE;
        uval = (unsigned long long)(-value);
    }
    else
    {
        isNegative = REDUCT_FALSE;
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
    reduct_atom_t* atom = reduct_atom_lookup(reduct, str, len, REDUCT_ATOM_LOOKUP_NONE);
    atom->integerValue = value;
    reduct_item_t* item = REDUCT_CONTAINER_OF(atom, reduct_item_t, atom);
    item->flags |= REDUCT_ITEM_FLAG_INT_SHAPED;
    return atom;
}

REDUCT_API reduct_atom_t* reduct_atom_lookup_float(reduct_t* reduct, reduct_float_t value)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);

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

    reduct_size_t len = 0;
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

    reduct_atom_t* atom = reduct_atom_lookup(reduct, res, len, REDUCT_ATOM_LOOKUP_NONE);
    atom->floatValue = value;
    reduct_item_t* item = REDUCT_CONTAINER_OF(atom, reduct_item_t, atom);
    item->flags |= REDUCT_ITEM_FLAG_FLOAT_SHAPED;
    return atom;
}

REDUCT_API reduct_atom_t* reduct_atom_lookup(reduct_t* reduct, const char* str, reduct_size_t len,
    reduct_atom_lookup_flags_t flags)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(str != REDUCT_NULL);

    reduct_size_t hash = reduct_hash(str, len);
    if (flags & REDUCT_ATOM_LOOKUP_QUOTED)
    {
        hash ^= 0x5bd1e995;
    }
    reduct_size_t bucket = hash % REDUCT_BUCKETS_MAX;

    reduct_atom_t** current = &reduct->atomBuckets[bucket];
    while (*current)
    {
        reduct_atom_t* atom = *current;
        reduct_item_t* item = REDUCT_CONTAINER_OF(atom, reduct_item_t, atom);
        reduct_bool_t isQuoted = (item->flags & REDUCT_ITEM_FLAG_QUOTED) != 0;
        reduct_bool_t wantQuoted = (flags & REDUCT_ATOM_LOOKUP_QUOTED) != 0;
        if (atom->hash == hash && isQuoted == wantQuoted && reduct_atom_is_equal(atom, str, len))
        {
            if (current != &reduct->atomBuckets[bucket])
            {
                *current = atom->next;
                atom->next = reduct->atomBuckets[bucket];
                reduct->atomBuckets[bucket] = atom;
            }
            return atom;
        }
        current = &(*current)->next;
    }

    reduct_item_t* item = reduct_item_new(reduct);
    item->type = REDUCT_ITEM_TYPE_ATOM;
    if (flags & REDUCT_ATOM_LOOKUP_QUOTED)
    {
        item->flags |= REDUCT_ITEM_FLAG_QUOTED;
    }
    reduct_atom_t* atom = &item->atom;
    atom->length = len;
    atom->hash = hash;
    atom->intrinsic = REDUCT_INTRINSIC_NONE;
    atom->next = reduct->atomBuckets[bucket];
    reduct->atomBuckets[bucket] = atom;

    if (len < REDUCT_ATOM_SMALL_MAX)
    {
        REDUCT_MEMCPY(atom->small, str, len);
        atom->small[len] = '\0';
        atom->string = atom->small;
    }
    else
    {
        atom->string = REDUCT_MALLOC(len + 1);
        if (atom->string == REDUCT_NULL)
        {
            REDUCT_ERROR_INTERNAL(reduct, "out of memory");
        }
        REDUCT_MEMCPY(atom->string, str, len);
        atom->string[len] = '\0';
    }

    return atom;
}

static inline void reduct_atom_normalize_escape(reduct_atom_t* atom)
{
    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(atom != REDUCT_NULL);

    char* str = atom->string;
    reduct_size_t len = atom->length;
    reduct_size_t j = 0;
    for (reduct_size_t i = 0; i < len; i++)
    {
        if (str[i] == '\\' && i + 1 < len)
        {
            i++;
            const reduct_char_info_t* info = &reductCharTable[(unsigned char)str[i]];
            if (info->decodeEscape != 0)
            {
                str[j++] = info->decodeEscape;
                continue;
            }
            else if (str[i] == 'x' && i + 2 < len)
            {
                unsigned char high = reductCharTable[(unsigned char)str[i + 1]].integer;
                unsigned char low = reductCharTable[(unsigned char)str[i + 2]].integer;
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
    if (atom->length < REDUCT_ATOM_SMALL_MAX)
    {
        str[j] = '\0';
    }
    else
    {
        atom->string[j] = '\0';
    }
}

REDUCT_API void reduct_atom_normalize(reduct_t* reduct, reduct_atom_t* atom)
{
    REDUCT_UNUSED(reduct);

    REDUCT_ASSERT(reduct != REDUCT_NULL);
    REDUCT_ASSERT(atom != REDUCT_NULL);

    reduct_item_t* item = REDUCT_CONTAINER_OF(atom, reduct_item_t, atom);
    if (atom->length == 0)
    {
        item->flags |= REDUCT_ITEM_FLAG_FALSY;
        return;
    }

    if (item->flags & REDUCT_ITEM_FLAG_QUOTED)
    {
        reduct_atom_normalize_escape(atom);
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
        if (REDUCT_CHAR_TO_LOWER(p[0]) == 'i' && REDUCT_CHAR_TO_LOWER(p[1]) == 'n' && REDUCT_CHAR_TO_LOWER(p[2]) == 'f')
        {
            item->flags |= REDUCT_ITEM_FLAG_FLOAT_SHAPED;
            atom->floatValue = sign > 0 ? REDUCT_INF : -REDUCT_INF;
            return;
        }
        if (p == start && REDUCT_CHAR_TO_LOWER(p[0]) == 'n' && REDUCT_CHAR_TO_LOWER(p[1]) == 'a' &&
            REDUCT_CHAR_TO_LOWER(p[2]) == 'n')
        {
            item->flags |= REDUCT_ITEM_FLAG_FLOAT_SHAPED;
            atom->floatValue = REDUCT_NAN;
            return;
        }
    }

    int base = 10;
    if (p + 1 < end && *p == '0')
    {
        char l = REDUCT_CHAR_TO_LOWER(p[1]);
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

    reduct_bool_t hasDigits = REDUCT_FALSE;
    reduct_bool_t sectionHasDigits = REDUCT_FALSE;
    reduct_bool_t valid = REDUCT_TRUE;

    if (base != 10)
    {
        reduct_uint64_t intValue = 0;
        while (p < end)
        {
            if (*p == '_')
            {
                if (!sectionHasDigits)
                {
                    valid = REDUCT_FALSE;
                    break;
                }
                p++;
                continue;
            }

            int d = -1;
            unsigned char c = (unsigned char)*p;
            if (REDUCT_CHAR_IS_HEX_DIGIT(c))
            {
                d = reductCharTable[c].integer;
            }

            if (d >= 0 && d < base)
            {
                intValue = intValue * base + d;
                hasDigits = REDUCT_TRUE;
                sectionHasDigits = REDUCT_TRUE;
            }
            else
            {
                valid = REDUCT_FALSE;
                break;
            }
            p++;
        }
        if (valid && hasDigits && p == end && *(end - 1) != '_')
        {
            item->flags |= REDUCT_ITEM_FLAG_INT_SHAPED;
            atom->integerValue = sign * (reduct_int64_t)intValue;
            if (atom->integerValue == 0)
            {
                item->flags |= REDUCT_ITEM_FLAG_FALSY;
            }
        }
        return;
    }

    reduct_bool_t isFloat = REDUCT_FALSE;
    reduct_uint64_t intValue = 0;
    double floatValue = 0.0;
    double fractionDiv = 10.0;
    reduct_bool_t inFraction = REDUCT_FALSE;
    reduct_bool_t inExponent = REDUCT_FALSE;
    reduct_int64_t expSign = 1;
    reduct_int64_t expValue = 0;
    reduct_int64_t exponentDigits = 0;

    if (*p == '.')
    {
        isFloat = REDUCT_TRUE;
        inFraction = REDUCT_TRUE;
        p++;
    }

    while (p < end)
    {
        if (*p == '_')
        {
            if (!sectionHasDigits)
            {
                valid = REDUCT_FALSE;
                break;
            }
            p++;
            continue;
        }

        unsigned char c = (unsigned char)*p;
        if (REDUCT_CHAR_IS_DIGIT(c))
        {
            hasDigits = REDUCT_TRUE;
            sectionHasDigits = REDUCT_TRUE;
            if (inExponent)
            {
                expValue = expValue * 10 + reductCharTable[c].integer;
                exponentDigits++;
            }
            else if (inFraction)
            {
                floatValue = floatValue + reductCharTable[c].integer / fractionDiv;
                fractionDiv *= 10.0;
            }
            else
            {
                intValue = intValue * 10 + reductCharTable[c].integer;
                floatValue = floatValue * 10.0 + reductCharTable[c].integer;
            }
        }
        else if (c == '.' && !inFraction && !inExponent)
        {
            if (*(p - 1) == '_')
            {
                valid = REDUCT_FALSE;
                break;
            }
            isFloat = REDUCT_TRUE;
            inFraction = REDUCT_TRUE;
            sectionHasDigits = REDUCT_FALSE;
        }
        else if (REDUCT_CHAR_TO_LOWER(c) == 'e' && !inExponent && hasDigits)
        {
            if (*(p - 1) == '_')
            {
                valid = REDUCT_FALSE;
                break;
            }
            isFloat = REDUCT_TRUE;
            inExponent = REDUCT_TRUE;
            sectionHasDigits = REDUCT_FALSE;
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
            valid = REDUCT_FALSE;
            break;
        }
        p++;
    }

    if (inExponent && exponentDigits == 0)
    {
        valid = REDUCT_FALSE;
    }

    if (valid && hasDigits && p == end && *(end - 1) != '_')
    {
        if (isFloat)
        {
            item->flags |= REDUCT_ITEM_FLAG_FLOAT_SHAPED;
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
                item->flags |= REDUCT_ITEM_FLAG_FALSY;
            }
        }
        else
        {
            item->flags |= REDUCT_ITEM_FLAG_INT_SHAPED;
            atom->integerValue = sign * (reduct_int64_t)intValue;
            if (atom->integerValue == 0)
            {
                item->flags |= REDUCT_ITEM_FLAG_FALSY;
            }
        }
    }
}

#endif
