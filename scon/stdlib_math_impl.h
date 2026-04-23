#ifndef SCON_STDLIB_MATH_IMPL_H
#define SCON_STDLIB_MATH_IMPL_H 1

#include "core.h"
#include "handle.h"
#include "stdlib_math.h"
#include "stdlib_type_casting.h"

SCON_API scon_handle_t scon_min(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    if (argc == 0)
    {
        return scon_handle_nil(scon);
    }

    scon_handle_t current = argv[0];
    for (scon_size_t i = 1; i < argc; i++)
    {
        scon_promotion_t prom;
        scon_handle_promote(scon, &current, &argv[i], &prom);
        if (prom.type == SCON_PROMOTION_TYPE_INT)
        {
            current = SCON_HANDLE_FROM_INT(prom.a.intVal < prom.b.intVal ? prom.a.intVal : prom.b.intVal);
        }
        else
        {
            current = SCON_HANDLE_FROM_FLOAT(prom.a.floatVal < prom.b.floatVal ? prom.a.floatVal : prom.b.floatVal);
        }
    }
    return current;
}

SCON_API scon_handle_t scon_max(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    if (argc == 0)
    {
        return scon_handle_nil(scon);
    }

    scon_handle_t current = argv[0];
    for (scon_size_t i = 1; i < argc; i++)
    {
        scon_promotion_t prom;
        scon_handle_promote(scon, &current, &argv[i], &prom);
        if (prom.type == SCON_PROMOTION_TYPE_INT)
        {
            current = SCON_HANDLE_FROM_INT(prom.a.intVal > prom.b.intVal ? prom.a.intVal : prom.b.intVal);
        }
        else
        {
            current = SCON_HANDLE_FROM_FLOAT(prom.a.floatVal > prom.b.floatVal ? prom.a.floatVal : prom.b.floatVal);
        }
    }
    return current;
}

SCON_API scon_handle_t scon_clamp(scon_t* scon, scon_handle_t* val, scon_handle_t* minVal, scon_handle_t* maxVal)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_t current = *val;
    scon_promotion_t prom;

    scon_handle_promote(scon, &current, minVal, &prom);
    if (prom.type == SCON_PROMOTION_TYPE_INT)
    {
        current = SCON_HANDLE_FROM_INT(prom.a.intVal < prom.b.intVal ? prom.b.intVal : prom.a.intVal);
    }
    else
    {
        current = SCON_HANDLE_FROM_FLOAT(prom.a.floatVal < prom.b.floatVal ? prom.b.floatVal : prom.a.floatVal);
    }

    scon_handle_promote(scon, &current, maxVal, &prom);
    if (prom.type == SCON_PROMOTION_TYPE_INT)
    {
        current = SCON_HANDLE_FROM_INT(prom.a.intVal > prom.b.intVal ? prom.b.intVal : prom.a.intVal);
    }
    else
    {
        current = SCON_HANDLE_FROM_FLOAT(prom.a.floatVal > prom.b.floatVal ? prom.b.floatVal : prom.a.floatVal);
    }

    return current;
}

SCON_API scon_handle_t scon_abs(scon_t* scon, scon_handle_t* val)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (SCON_HANDLE_IS_INT_SHAPED(val))
    {
        scon_handle_t iVal = scon_get_int(scon, val);
        scon_int64_t i = SCON_HANDLE_TO_INT(&iVal);
        return SCON_HANDLE_FROM_INT(i < 0 ? -i : i);
    }
    scon_handle_t floatVal = scon_get_float(scon, val);
    scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal);
    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_FABS(f));
}

SCON_API scon_handle_t scon_floor(struct scon* scon, scon_handle_t* val)
{
    SCON_ASSERT(scon != SCON_NULL);
    if (SCON_HANDLE_IS_INT_SHAPED(val))
    {
        return scon_get_int(scon, val);
    }
    scon_handle_t floatVal = scon_get_float(scon, val);
    scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal);
    return SCON_HANDLE_FROM_INT((scon_int64_t)SCON_FLOOR(f));
}

SCON_API scon_handle_t scon_ceil(struct scon* scon, scon_handle_t* val)
{
    SCON_ASSERT(scon != SCON_NULL);
    if (SCON_HANDLE_IS_INT_SHAPED(val))
    {
        return scon_get_int(scon, val);
    }
    scon_handle_t floatVal = scon_get_float(scon, val);
    scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal);
    return SCON_HANDLE_FROM_INT((scon_int64_t)SCON_CEIL(f));
}

SCON_API scon_handle_t scon_round(struct scon* scon, scon_handle_t* val)
{
    SCON_ASSERT(scon != SCON_NULL);
    if (SCON_HANDLE_IS_INT_SHAPED(val))
    {
        return scon_get_int(scon, val);
    }
    scon_handle_t floatVal = scon_get_float(scon, val);
    scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal);
    return SCON_HANDLE_FROM_INT((scon_int64_t)SCON_ROUND(f));
}

SCON_API scon_handle_t scon_pow(scon_t* scon, scon_handle_t* base, scon_handle_t* exp)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_promotion_t prom;
    scon_handle_promote(scon, base, exp, &prom);

    if (prom.type == SCON_PROMOTION_TYPE_INT)
    {
        return SCON_HANDLE_FROM_INT((scon_int64_t)SCON_POW((scon_float_t)prom.a.intVal, (scon_float_t)prom.b.intVal));
    }
    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_POW(prom.a.floatVal, prom.b.floatVal));
}

SCON_API scon_handle_t scon_exp(struct scon* scon, scon_handle_t* val)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (SCON_HANDLE_IS_INT_SHAPED(val))
    {
        scon_handle_t iVal = scon_get_int(scon, val);
        scon_int64_t i = SCON_HANDLE_TO_INT(&iVal);
        return SCON_HANDLE_FROM_INT((scon_int64_t)SCON_EXP((scon_float_t)i));
    }

    scon_handle_t floatVal = scon_get_float(scon, val);
    scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal);
    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_EXP(f));
}

SCON_API scon_handle_t scon_log(struct scon* scon, scon_handle_t* val, scon_handle_t* base)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (base == SCON_NULL)
    {
        if (SCON_HANDLE_IS_INT_SHAPED(val))
        {
            scon_handle_t iVal = scon_get_int(scon, val);
            scon_int64_t i = SCON_HANDLE_TO_INT(&iVal);
            return SCON_HANDLE_FROM_INT((scon_int64_t)SCON_LOG(i));
        }

        scon_handle_t floatVal = scon_get_float(scon, val);
        scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal);
        return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_LOG(f));
    }

    scon_promotion_t prom;
    scon_handle_promote(scon, val, base, &prom);

    if (prom.type == SCON_PROMOTION_TYPE_INT)
    {
        scon_float_t res = SCON_LOG((scon_float_t)prom.a.intVal) / SCON_LOG((scon_float_t)prom.b.intVal);
        return SCON_HANDLE_FROM_INT((scon_int64_t)res);
    }
    scon_float_t res = SCON_LOG(prom.a.floatVal) / SCON_LOG(prom.b.floatVal);
    return SCON_HANDLE_FROM_FLOAT((scon_float_t)res);
}

SCON_API scon_handle_t scon_sqrt(struct scon* scon, scon_handle_t* val)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (SCON_HANDLE_IS_INT_SHAPED(val))
    {
        scon_handle_t iVal = scon_get_int(scon, val);
        scon_int64_t i = SCON_HANDLE_TO_INT(&iVal);
        return SCON_HANDLE_FROM_INT((scon_int64_t)SCON_SQRT((scon_float_t)i));
    }
    scon_handle_t floatVal = scon_get_float(scon, val);
    scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal);
    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_SQRT(f));
}

SCON_API scon_handle_t scon_sin(struct scon* scon, scon_handle_t* val)
{
    SCON_ASSERT(scon != SCON_NULL);
    scon_handle_t floatVal = scon_get_float(scon, val);
    scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal);
    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_SIN(f));
}

SCON_API scon_handle_t scon_cos(struct scon* scon, scon_handle_t* val)
{
    SCON_ASSERT(scon != SCON_NULL);
    scon_handle_t floatVal = scon_get_float(scon, val);
    scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal);
    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_COS(f));
}

SCON_API scon_handle_t scon_tan(struct scon* scon, scon_handle_t* val)
{
    SCON_ASSERT(scon != SCON_NULL);
    scon_handle_t floatVal = scon_get_float(scon, val);
    scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal);
    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_TAN(f));
}

SCON_API scon_handle_t scon_asin(struct scon* scon, scon_handle_t* val)
{
    SCON_ASSERT(scon != SCON_NULL);
    scon_handle_t floatVal = scon_get_float(scon, val);
    scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal);
    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_ASIN(f));
}

SCON_API scon_handle_t scon_acos(struct scon* scon, scon_handle_t* val)
{
    SCON_ASSERT(scon != SCON_NULL);
    scon_handle_t floatVal = scon_get_float(scon, val);
    scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal);
    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_ACOS(f));
}

SCON_API scon_handle_t scon_atan(struct scon* scon, scon_handle_t* val)
{
    SCON_ASSERT(scon != SCON_NULL);
    scon_handle_t floatVal = scon_get_float(scon, val);
    scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal);
    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_ATAN(f));
}

SCON_API scon_handle_t scon_atan2(struct scon* scon, scon_handle_t* y, scon_handle_t* x)
{
    SCON_ASSERT(scon != SCON_NULL);
    scon_handle_t yFloatVal = scon_get_float(scon, y);
    scon_handle_t xFloatVal = scon_get_float(scon, x);
    scon_float_t yf = SCON_HANDLE_TO_FLOAT(&yFloatVal);
    scon_float_t xf = SCON_HANDLE_TO_FLOAT(&xFloatVal);
    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_ATAN2(yf, xf));
}

SCON_API scon_handle_t scon_sinh(struct scon* scon, scon_handle_t* val)
{
    SCON_ASSERT(scon != SCON_NULL);
    scon_handle_t floatVal = scon_get_float(scon, val);
    scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal);
    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_SINH(f));
}

SCON_API scon_handle_t scon_cosh(struct scon* scon, scon_handle_t* val)
{
    SCON_ASSERT(scon != SCON_NULL);
    scon_handle_t floatVal = scon_get_float(scon, val);
    scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal);
    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_COSH(f));
}

SCON_API scon_handle_t scon_tanh(struct scon* scon, scon_handle_t* val)
{
    SCON_ASSERT(scon != SCON_NULL);
    scon_handle_t floatVal = scon_get_float(scon, val);
    scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal);
    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_TANH(f));
}

SCON_API scon_handle_t scon_asinh(struct scon* scon, scon_handle_t* val)
{
    SCON_ASSERT(scon != SCON_NULL);
    scon_handle_t floatVal = scon_get_float(scon, val);
    scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal);
    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_ASINH(f));
}

SCON_API scon_handle_t scon_acosh(struct scon* scon, scon_handle_t* val)
{
    SCON_ASSERT(scon != SCON_NULL);
    scon_handle_t floatVal = scon_get_float(scon, val);
    scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal);
    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_ACOSH(f));
}

SCON_API scon_handle_t scon_atanh(struct scon* scon, scon_handle_t* val)
{
    SCON_ASSERT(scon != SCON_NULL);
    scon_handle_t floatVal = scon_get_float(scon, val);
    scon_float_t f = SCON_HANDLE_TO_FLOAT(&floatVal);
    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_ATANH(f));
}

SCON_API scon_handle_t scon_rand(struct scon* scon, scon_handle_t* minVal, scon_handle_t* maxVal)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_promotion_t prom;
    scon_handle_promote(scon, minVal, maxVal, &prom);

    scon_float_t r = (scon_float_t)SCON_RAND() / (scon_float_t)SCON_RAND_MAX;

    if (prom.type == SCON_PROMOTION_TYPE_INT)
    {
        scon_int64_t res = prom.a.intVal + (scon_int64_t)(r * (prom.b.intVal - prom.a.intVal));
        return SCON_HANDLE_FROM_INT(res);
    }
    scon_float_t res = prom.a.floatVal + (r * (prom.b.floatVal - prom.a.floatVal));
    return SCON_HANDLE_FROM_FLOAT(res);
}

SCON_API scon_handle_t scon_seed(struct scon* scon, scon_handle_t* val)
{
    SCON_ASSERT(scon != SCON_NULL);
    scon_handle_t iVal = scon_get_int(scon, val);
    scon_int64_t i = SCON_HANDLE_TO_INT(&iVal);
    SCON_SRAND((unsigned int)i);
    return scon_handle_nil(scon);
}

#endif