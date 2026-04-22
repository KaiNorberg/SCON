#ifndef SCON_EVAL_IMPL_H
#define SCON_EVAL_IMPL_H 1

#include "closure.h"
#include "defs.h"
#include "eval.h"
#include "item.h"
#include "stdlib_type_casting.h"

static void scon_eval_state_init(scon_t* scon, scon_eval_state_t* state)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(state != SCON_NULL);

    state->frameCount = 0;
    state->frameCapacity = SCON_EVAL_FRAMES_INITIAL;
    state->frames = (scon_eval_frame_t*)SCON_MALLOC(sizeof(scon_eval_frame_t) * state->frameCapacity);
    if (state->frames == SCON_NULL)
    {
        SCON_ERROR_INTERNAL(scon, "out of memory");
    }

    state->regCount = 1;
    state->regCapacity = SCON_EVAL_REGS_INITIAL;
    state->regs = (scon_handle_t*)SCON_MALLOC(sizeof(scon_handle_t) * state->regCapacity);
    if (state->regs == SCON_NULL)
    {
        SCON_FREE(state->frames);
        SCON_ERROR_INTERNAL(scon, "out of memory");
    }
    state->regs[0] = scon_handle_nil(scon);
}

SCON_API void scon_eval_state_deinit(scon_eval_state_t* state)
{
    SCON_ASSERT(state != SCON_NULL);

    SCON_FREE(state->frames);
    SCON_FREE(state->regs);
}

static inline SCON_ALWAYS_INLINE void scon_eval_push_frame(scon_t* scon, scon_eval_state_t* state, scon_closure_t* closure, scon_uint32_t target)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(state != SCON_NULL);
    SCON_ASSERT(closure != SCON_NULL);

    if (SCON_UNLIKELY(state->frameCount >= state->frameCapacity))
    {
        state->frameCapacity *= SCON_EVAL_FRAMES_GROWTH_FACTOR;
        scon_eval_frame_t* newFrames =
            (scon_eval_frame_t*)SCON_REALLOC(state->frames, sizeof(scon_eval_frame_t) * state->frameCapacity);
        if (newFrames == SCON_NULL)
        {
            scon_eval_state_deinit(state);
            SCON_ERROR_INTERNAL(scon, "out of memory");
        }
        state->frames = newFrames;
    }

    scon_uint32_t neededRegs = target + closure->function->registerCount;
    if (SCON_UNLIKELY(neededRegs > state->regCapacity))
    {
        while (neededRegs > state->regCapacity)
        {
            state->regCapacity *= SCON_EVAL_REGS_GROWTH_FACTOR;
        }
        scon_handle_t* newRegs = (scon_handle_t*)SCON_REALLOC(state->regs, sizeof(scon_handle_t) * state->regCapacity);
        if (newRegs == SCON_NULL)
        {
            scon_eval_state_deinit(state);
            SCON_ERROR_INTERNAL(scon, "out of memory");
        }
        state->regs = newRegs;
    }

    scon_eval_frame_t* frame = &state->frames[state->frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->insts;
    frame->base = target;
    frame->target = target;

    state->regCount = neededRegs;
}

static inline SCON_ALWAYS_INLINE void scon_eval_pop_frame(scon_eval_state_t* state)
{
    SCON_ASSERT(state != SCON_NULL);
    SCON_ASSERT(state->frameCount > 0);

    scon_eval_frame_t* frame = &state->frames[--state->frameCount];
    state->regCount = frame->base;
}

static scon_handle_t scon_eval_run(scon_t* scon, scon_eval_state_t* state, scon_uint32_t initialFrameCount)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(state != SCON_NULL);

    scon_eval_frame_t* frame = &state->frames[state->frameCount - 1];
    scon_inst_t* ip = frame->ip;
    scon_handle_t* base = state->regs + frame->base;
    scon_handle_t* constants = frame->closure->constants;
    scon_inst_t inst;
    scon_opcode_t op;
    scon_handle_t result = SCON_HANDLE_NONE;

    /// @todo Write some macro magic to turn this into a switch case if not using GCC or Clang
#define DISPATCH() \
    do \
    { \
        inst = *ip++; \
        op = SCON_INST_GET_OP(inst); \
        goto* dispatchTable[op]; \
    } while (0)

#define DECODE_A() scon_uint32_t a = SCON_INST_GET_A(inst)
#define DECODE_B() scon_uint32_t b = SCON_INST_GET_B(inst)
#define DECODE_C_REG() \
    scon_uint32_t c = SCON_INST_GET_C(inst); \
    scon_handle_t valC = base[c]
#define DECODE_C_CONST() \
    scon_uint32_t c = SCON_INST_GET_C(inst); \
    scon_handle_t valC = constants[c]
#define DECODE_SBX() scon_int32_t sbx = SCON_INST_GET_SBX(inst)

#define ERROR_CHECK(_expr, ...) \
    do \
    { \
        if (SCON_UNLIKELY(!(_expr))) \
        { \
            frame->ip = ip; \
            SCON_ERROR_RUNTIME(__VA_ARGS__); \
        } \
    } while (0)

#define OP_ENTRY(_op, _label) \
    [_op] = &&_label, \
    [_op | SCON_MODE_CONST] = &&_label

#define OP_ENTRY_C(_op, _label) \
    [_op] = &&_label, \
    [_op | SCON_MODE_CONST] = &&_label##_k

    void* dispatchTable[] = {
        OP_ENTRY(SCON_OPCODE_NONE, label_none),
        OP_ENTRY(SCON_OPCODE_LIST, label_list),
        OP_ENTRY(SCON_OPCODE_JMP, label_jmp),
        OP_ENTRY(SCON_OPCODE_JMPF, label_jmpf),
        OP_ENTRY(SCON_OPCODE_JMPT, label_jmpt),
        OP_ENTRY_C(SCON_OPCODE_CALL, label_call),
        OP_ENTRY_C(SCON_OPCODE_MOV, label_mov),
        OP_ENTRY_C(SCON_OPCODE_RET, label_ret),
        OP_ENTRY_C(SCON_OPCODE_APPEND, label_append),
        OP_ENTRY_C(SCON_OPCODE_EQ, label_eq),
        OP_ENTRY_C(SCON_OPCODE_NEQ, label_neq),
        OP_ENTRY_C(SCON_OPCODE_SEQ, label_seq),
        OP_ENTRY_C(SCON_OPCODE_SNEQ, label_sneq),
        OP_ENTRY_C(SCON_OPCODE_LT, label_lt),
        OP_ENTRY_C(SCON_OPCODE_LE, label_le),
        OP_ENTRY_C(SCON_OPCODE_GT, label_gt),
        OP_ENTRY_C(SCON_OPCODE_GE, label_ge),
        OP_ENTRY_C(SCON_OPCODE_ADD, label_add),
        OP_ENTRY_C(SCON_OPCODE_SUB, label_sub),
        OP_ENTRY_C(SCON_OPCODE_MUL, label_mul),
        OP_ENTRY_C(SCON_OPCODE_DIV, label_div),
        OP_ENTRY_C(SCON_OPCODE_MOD, label_mod),
        OP_ENTRY_C(SCON_OPCODE_BAND, label_band),
        OP_ENTRY_C(SCON_OPCODE_BOR, label_bor),
        OP_ENTRY_C(SCON_OPCODE_BXOR, label_bxor),
        OP_ENTRY_C(SCON_OPCODE_BNOT, label_bnot),
        OP_ENTRY_C(SCON_OPCODE_SHL, label_shl),
        OP_ENTRY_C(SCON_OPCODE_SHR, label_shr),
        OP_ENTRY(SCON_OPCODE_CLOSURE, label_closure),
        OP_ENTRY_C(SCON_OPCODE_CAPTURE, label_capture),
        OP_ENTRY_C(SCON_OPCODE_TAILCALL, label_tailcall),
    };

#define LABEL_C_OP(_label, ...) \
    _label: \
    { \
        DECODE_C_REG(); \
        __VA_ARGS__ \
    } \
    _label##_k: \
    { \
        DECODE_C_CONST(); \
        __VA_ARGS__ \
    }

    DISPATCH();
label_none:
    frame->ip = ip;
    SCON_ERROR_RUNTIME(scon, SCON_NULL, "invalid opcode, %u", inst);
label_list:
{
    DECODE_A();
    base[a] = SCON_HANDLE_FROM_LIST(scon_list_new(scon, 0));
    DISPATCH();
}
label_jmp:
{
    DECODE_SBX();
    ip += sbx;
    DISPATCH();
}
label_jmpf:
{
    DECODE_A();
    DECODE_SBX();
    scon_handle_t val = base[a];
    if (SCON_LIKELY(val == SCON_HANDLE_FALSE()) || SCON_UNLIKELY(val != SCON_HANDLE_TRUE() && !SCON_HANDLE_IS_TRUTHY(&val)))
    {
        ip += sbx;
    }
    DISPATCH();
}
label_jmpt:
{
    DECODE_A();
    DECODE_SBX();
    scon_handle_t val = base[a];
    if (SCON_LIKELY(val == SCON_HANDLE_TRUE()) || SCON_UNLIKELY(val != SCON_HANDLE_FALSE() && SCON_HANDLE_IS_TRUTHY(&val)))
    {
        ip += sbx;
    }
    DISPATCH();
}
LABEL_C_OP(label_call,
{
    DECODE_A();
    DECODE_B();
    ERROR_CHECK(SCON_HANDLE_IS_ITEM(&valC), scon, SCON_NULL, "attempt to call non-callable %s", scon_item_type_str(scon_handle_get_type(scon, &valC)));
    scon_item_t* item = SCON_HANDLE_TO_ITEM(&valC);
    if (SCON_LIKELY(item->type == SCON_ITEM_TYPE_CLOSURE))
    {
        scon_closure_t* closure = &item->closure;
        ERROR_CHECK(b == closure->function->arity, scon, SCON_NULL, "expected %d arguments, got %d", closure->function->arity, b);

        frame->ip = ip;
        scon_eval_push_frame(scon, state, closure, frame->base + a);

        frame = &state->frames[state->frameCount - 1];
        ip = frame->ip;
        base = state->regs + frame->base;
        constants = frame->closure->constants;

        DISPATCH();
    }
    if (SCON_LIKELY(item->flags & SCON_ITEM_FLAG_NATIVE))
    {
        scon_handle_t* args = &base[a];
        frame->ip = ip;
        scon_handle_t result = item->atom.native(scon, b, args);

        frame = &state->frames[state->frameCount - 1];
        base = state->regs + frame->base;
        base[a] = result;
        constants = frame->closure->constants;

        DISPATCH();
    }

    frame->ip = ip;
    SCON_ERROR_RUNTIME(scon, SCON_NULL,  "attempt to call non-callable %s", scon_item_type_str(item->type));
})
LABEL_C_OP(label_tailcall,
{
    DECODE_A();
    DECODE_B();
    ERROR_CHECK(SCON_HANDLE_IS_ITEM(&valC), scon, SCON_NULL, "attempt to call non-callable %s", scon_item_type_str(scon_handle_get_type(scon, &valC)));
    scon_item_t* item = SCON_HANDLE_TO_ITEM(&valC);
    if (SCON_LIKELY(item->type == SCON_ITEM_TYPE_CLOSURE))
    {
        scon_closure_t* closure = &item->closure;
        ERROR_CHECK(b == closure->function->arity, scon, SCON_NULL, "expected %d arguments, got %d", closure->function->arity, b);

        if (a != 0)
        {
            for (scon_uint32_t i = 0; i < b; i++)
            {
                base[i] = base[a + i];
            }
        }

        scon_uint32_t neededRegs = frame->base + closure->function->registerCount;
        if (SCON_UNLIKELY(neededRegs > state->regCapacity))
        {
            while (neededRegs > state->regCapacity)
            {
                state->regCapacity *= SCON_EVAL_REGS_GROWTH_FACTOR;
            }
            scon_handle_t* newRegs = (scon_handle_t*)SCON_REALLOC(state->regs, sizeof(scon_handle_t) * state->regCapacity);
            if (newRegs == SCON_NULL)
            {
                scon_eval_state_deinit(state);
                SCON_ERROR_INTERNAL(scon, "out of memory");
            }
            state->regs = newRegs;
            base = state->regs + frame->base;
        }
        state->regCount = neededRegs;

        frame->closure = closure;
        frame->ip = closure->function->insts;

        ip = frame->ip;
        constants = frame->closure->constants;

        DISPATCH();
    }
    if (SCON_LIKELY(item->flags & SCON_ITEM_FLAG_NATIVE))
    {
        scon_handle_t* args = &base[a];
        frame->ip = ip;
        scon_handle_t res = item->atom.native(scon, b, args);

        state->regs[frame->target] = res;
        scon_eval_pop_frame(state);

        if (SCON_UNLIKELY(state->frameCount == initialFrameCount))
        {
            result = res;
            goto eval_end;
        }

        frame = &state->frames[state->frameCount - 1];
        ip = frame->ip;
        base = state->regs + frame->base;
        constants = frame->closure->constants;

        DISPATCH();
    }

    frame->ip = ip;
    SCON_ERROR_RUNTIME(scon, SCON_NULL,  "attempt to call non-callable %s", scon_item_type_str(item->type));
})
LABEL_C_OP(label_mov,
{
    DECODE_A();
    base[a] = valC;
    DISPATCH();
})
LABEL_C_OP(label_ret,
{
    state->regs[frame->target] = valC;
    scon_eval_pop_frame(state);

    if (SCON_UNLIKELY(state->frameCount == initialFrameCount))
    {
        result = valC;
        goto eval_end;
    }

    frame = &state->frames[state->frameCount - 1];
    ip = frame->ip;
    base = state->regs + frame->base;
    constants = frame->closure->constants;

    DISPATCH();
})
LABEL_C_OP(label_append,
{
    DECODE_A();
    ERROR_CHECK(SCON_HANDLE_IS_ITEM(&base[a]), scon, SCON_NULL, "APPEND expected a list");

    scon_item_t* item = SCON_HANDLE_TO_ITEM(&base[a]);
    ERROR_CHECK(item->type == SCON_ITEM_TYPE_LIST, scon, SCON_NULL, "APPEND expected a list");

    scon_list_t* listPtr = &item->list;
    scon_list_append(scon, listPtr, valC);

    DISPATCH();
})
LABEL_C_OP(label_eq,
{
    DECODE_A();
    DECODE_B();
    base[a] = SCON_HANDLE_COMPARE_FAST(scon, &base[b], &valC, ==) ? SCON_HANDLE_TRUE() : SCON_HANDLE_FALSE();
    DISPATCH();
})
LABEL_C_OP(label_neq,
{
    DECODE_A();
    DECODE_B();
    base[a] = SCON_HANDLE_COMPARE_FAST(scon, &base[b], &valC, !=) ? SCON_HANDLE_TRUE() : SCON_HANDLE_FALSE();
    DISPATCH();
})
LABEL_C_OP(label_seq,
{
    DECODE_A();
    DECODE_B();
    base[a] = scon_handle_is_equal(scon, &base[b], &valC) ? SCON_HANDLE_TRUE() : SCON_HANDLE_FALSE();
    DISPATCH();
})
LABEL_C_OP(label_sneq,
{
    DECODE_A();
    DECODE_B();
    base[a] = scon_handle_is_equal(scon, &base[b], &valC) ? SCON_HANDLE_FALSE() : SCON_HANDLE_TRUE();
    DISPATCH();
})
LABEL_C_OP(label_lt,
{
    DECODE_A();
    DECODE_B();
    base[a] = SCON_HANDLE_COMPARE_FAST(scon, &base[b], &valC, <) ? SCON_HANDLE_TRUE() : SCON_HANDLE_FALSE();
    DISPATCH();
})
LABEL_C_OP(label_le,
{
    DECODE_A();
    DECODE_B();
    base[a] = SCON_HANDLE_COMPARE_FAST(scon, &base[b], &valC, <=) ? SCON_HANDLE_TRUE() : SCON_HANDLE_FALSE();
    DISPATCH();
})
LABEL_C_OP(label_gt,
{
    DECODE_A();
    DECODE_B();
    base[a] = SCON_HANDLE_COMPARE_FAST(scon, &base[b], &valC, >) ? SCON_HANDLE_TRUE() : SCON_HANDLE_FALSE();
    DISPATCH();
})
LABEL_C_OP(label_ge,
{
    DECODE_A();
    DECODE_B();
    base[a] = SCON_HANDLE_COMPARE_FAST(scon, &base[b], &valC, >=) ? SCON_HANDLE_TRUE() : SCON_HANDLE_FALSE();
    DISPATCH();
})
LABEL_C_OP(label_add,
{
    DECODE_A();
    DECODE_B();
    SCON_HANDLE_ARITHMETIC_FAST(scon, &base[a], &base[b], &valC, +);
    DISPATCH();
})
LABEL_C_OP(label_sub,
{
    DECODE_A();
    DECODE_B();
    SCON_HANDLE_ARITHMETIC_FAST(scon, &base[a], &base[b], &valC, -);
    DISPATCH();
})
LABEL_C_OP(label_mul,
{
    DECODE_A();
    DECODE_B();
    SCON_HANDLE_ARITHMETIC_FAST(scon, &base[a], &base[b], &valC, *);
    DISPATCH();
})
LABEL_C_OP(label_div,
{
    DECODE_A();
    DECODE_B();
    SCON_HANDLE_ARITHMETIC_FAST(scon, &base[a], &base[b], &valC, /);
    DISPATCH();
})
LABEL_C_OP(label_mod,
{
    DECODE_A();
    DECODE_B();
    scon_promotion_t prom;
    scon_handle_promote(scon, &base[b], &valC, &prom);
    ERROR_CHECK(prom.type == SCON_PROMOTION_TYPE_INT, scon, SCON_NULL, "invalid item type");
    ERROR_CHECK(prom.b.intVal != 0, scon, SCON_NULL, "modulo by zero");
    base[a] = SCON_HANDLE_FROM_INT(prom.a.intVal % prom.b.intVal);
    DISPATCH();
})
LABEL_C_OP(label_band,
{
    DECODE_A();
    DECODE_B();
    base[a] = SCON_HANDLE_FROM_INT(scon_get_int(scon, &base[b]) & scon_get_int(scon, &valC));
    DISPATCH();
})
LABEL_C_OP(label_bor,
{
    DECODE_A();
    DECODE_B();
    base[a] = SCON_HANDLE_FROM_INT(scon_get_int(scon, &base[b]) | scon_get_int(scon, &valC));
    DISPATCH();
})
LABEL_C_OP(label_bxor,
{
    DECODE_A();
    DECODE_B();
    base[a] = SCON_HANDLE_FROM_INT(scon_get_int(scon, &base[b]) ^ scon_get_int(scon, &valC));
    DISPATCH();
})
LABEL_C_OP(label_bnot,
{
    DECODE_A();
    base[a] = SCON_HANDLE_FROM_INT(~scon_get_int(scon, &valC));
    DISPATCH();
})
LABEL_C_OP(label_shl,
{
    DECODE_A();
    DECODE_B();
    scon_int64_t left = scon_get_int(scon, &valC);
    ERROR_CHECK(left >= 0 && left < 64, scon, SCON_NULL, "expected left shift amount 0-63, got %ld", left);
    base[a] = SCON_HANDLE_FROM_INT(scon_get_int(scon, &base[b]) << left);
    DISPATCH();
})
LABEL_C_OP(label_shr,
{
    DECODE_A();
    DECODE_B();
    scon_int64_t right = scon_get_int(scon, &valC);
    ERROR_CHECK(right >= 0 && right < 64, scon, SCON_NULL, "expected right shift amount 0-63, got %ld", right);
    base[a] = SCON_HANDLE_FROM_INT(scon_get_int(scon, &base[b]) >> right);
    DISPATCH();
})
label_closure:
{
    DECODE_A();
    scon_uint32_t c = SCON_INST_GET_C(inst);
    scon_handle_t protoHandle = frame->closure->constants[c];
    ERROR_CHECK(SCON_HANDLE_IS_ITEM(&protoHandle), scon, SCON_NULL, "expected closure prototype to be an item");
    scon_item_t* protoItem = SCON_HANDLE_TO_ITEM(&protoHandle);
    ERROR_CHECK(protoItem->type == SCON_ITEM_TYPE_FUNCTION, scon, SCON_NULL, "expected closure prototype to be a function, got %s", scon_item_type_str(protoItem->type));

    scon_function_t* proto = &protoItem->function;
    base[a] = SCON_HANDLE_FROM_CLOSURE(scon_closure_new(scon, proto));
    DISPATCH();
}
LABEL_C_OP(label_capture,
{
    DECODE_A();
    DECODE_B();
    scon_handle_t closureHandle = base[a];
    scon_closure_t* closurePtr = &SCON_HANDLE_TO_ITEM(&closureHandle)->closure;
    closurePtr->constants[b] = valC;
    DISPATCH();
})
eval_end:
// clang-format on
    return result;
}

SCON_API scon_handle_t scon_eval(struct scon* scon, scon_function_t* function)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(function != SCON_NULL);

    if (scon->evalState == SCON_NULL)
    {
        scon->evalState = (scon_eval_state_t*)SCON_MALLOC(sizeof(scon_eval_state_t));
        scon_eval_state_init(scon, scon->evalState);
    }

    scon_closure_t* closure = scon_closure_new(scon, function);
    scon_eval_state_t* state = scon->evalState;
    scon_uint32_t initialFrameCount = state->frameCount;

    scon_eval_push_frame(scon, state, closure, state->regCount);

    return scon_eval_run(scon, state, initialFrameCount);
}

SCON_API scon_handle_t scon_eval_call(scon_t* scon, scon_handle_t callable, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(argv != SCON_NULL || argc == 0);

    if (!SCON_HANDLE_IS_ITEM(&callable))
    {
        SCON_ERROR_RUNTIME(scon, SCON_NULL, "attempt to call non-callable value");
    }

    scon_item_t* item = SCON_HANDLE_TO_ITEM(&callable);
    if (item->flags & SCON_ITEM_FLAG_NATIVE)
    {
        return item->atom.native(scon, argc, argv);
    }

    if (item->type == SCON_ITEM_TYPE_CLOSURE)
    {
        scon_closure_t* closure = &item->closure;
        if (argc != closure->function->arity)
        {
            SCON_ERROR_RUNTIME(scon, SCON_NULL, "expected %ld arguments, got %ld", closure->function->arity, argc);
        }

        if (scon->evalState == SCON_NULL)
        {
            scon->evalState = (scon_eval_state_t*)SCON_MALLOC(sizeof(scon_eval_state_t));
            if (scon->evalState == SCON_NULL)
            {
                SCON_ERROR_INTERNAL(scon, "out of memory");
            }
            scon_eval_state_init(scon, scon->evalState);
        }

        scon_eval_state_t* state = scon->evalState;
        scon_uint32_t target = state->regCount;

        while (target + argc > state->regCapacity)
        {
            state->regCapacity *= SCON_EVAL_REGS_GROWTH_FACTOR;
            scon_handle_t* newRegs = (scon_handle_t*)SCON_REALLOC(state->regs, sizeof(scon_handle_t) * state->regCapacity);
            if (newRegs == SCON_NULL)
            {
                SCON_ERROR_INTERNAL(scon, "out of memory");
            }
            state->regs = newRegs;
        }

        for (scon_size_t i = 0; i < argc; i++)
        {
            state->regs[target + i] = argv[i];
        }

        scon_uint32_t initialFrameCount = state->frameCount;
        scon_eval_push_frame(scon, state, closure, target);

        return scon_eval_run(scon, state, initialFrameCount);
    }

    SCON_ERROR_RUNTIME(scon, SCON_NULL, "attempt to call non-callable value");
}

#endif
