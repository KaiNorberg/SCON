#include "item.h"
#ifndef SCON_EVAL_IMPL_H
#define SCON_EVAL_IMPL_H 1

#include "closure.h"
#include "defs.h"
#include "eval.h"
#ifdef SCON_PROFILE_OPCODES
#include <stdio.h>
#include <time.h>
#endif

typedef struct
{
    scon_closure_t* closure;
    scon_inst_t* ip;
    scon_uint32_t base;
    scon_uint32_t target;
} scon_eval_frame_t;

typedef struct
{
    scon_eval_frame_t* frames;
    scon_uint32_t frameCount;
    scon_uint32_t frameCapacity;
    scon_handle_t* regs;
    scon_uint32_t regCount;
    scon_uint32_t regCapacity;
} scon_eval_state_t;

static void scon_eval_state_init(scon_t* scon, scon_eval_state_t* state)
{
    state->frameCount = 0;
    state->frameCapacity = SCON_EVAL_FRAMES_INITIAL;
    state->frames = (scon_eval_frame_t*)SCON_MALLOC(sizeof(scon_eval_frame_t) * state->frameCapacity);
    if (state->frames == SCON_NULL)
    {
        SCON_THROW(scon, "out of memory");
    }

    state->regCount = 1;
    state->regCapacity = SCON_EVAL_REGS_INITIAL;
    state->regs = (scon_handle_t*)SCON_MALLOC(sizeof(scon_handle_t) * state->regCapacity);
    if (state->regs == SCON_NULL)
    {
        SCON_FREE(state->frames);
        SCON_THROW(scon, "out of memory");
    }
    state->regs[0] = scon_handle_nil(scon);
}

static void scon_eval_state_deinit(scon_eval_state_t* state)
{
    SCON_FREE(state->frames);
    SCON_FREE(state->regs);
}

static void scon_eval_push_frame(scon_t* scon, scon_eval_state_t* state, scon_closure_t* closure, scon_uint32_t target)
{
    if (state->frameCount >= state->frameCapacity)
    {
        state->frameCapacity *= SCON_EVAL_FRAMES_GROWTH_FACTOR;
        scon_eval_frame_t* newFrames =
            (scon_eval_frame_t*)SCON_REALLOC(state->frames, sizeof(scon_eval_frame_t) * state->frameCapacity);
        if (newFrames == SCON_NULL)
        {
            scon_eval_state_deinit(state);
            SCON_THROW(scon, "out of memory");
        }
        state->frames = newFrames;
    }

    scon_uint32_t neededRegs = target + closure->function->registerCount;
    if (neededRegs > state->regCapacity)
    {
        while (neededRegs > state->regCapacity)
        {
            state->regCapacity *= SCON_EVAL_REGS_GROWTH_FACTOR;
        }
        scon_handle_t* newRegs = (scon_handle_t*)SCON_REALLOC(state->regs, sizeof(scon_handle_t) * state->regCapacity);
        if (newRegs == SCON_NULL)
        {
            scon_eval_state_deinit(state);
            SCON_THROW(scon, "out of memory");
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

static void scon_eval_pop_frame(scon_eval_state_t* state)
{
    if (state->frameCount == 0)
    {
        return;
    }

    scon_eval_frame_t* frame = &state->frames[--state->frameCount];
    state->regCount = frame->base;
}

SCON_API scon_handle_t scon_eval(struct scon* scon, scon_function_t* function)
{
    scon_closure_t* closure = scon_closure_new(scon, function);

    scon_eval_state_t state;
    scon_eval_state_init(scon, &state);

    scon_eval_push_frame(scon, &state, closure, 0);

    scon_eval_frame_t* frame = &state.frames[state.frameCount - 1];
    scon_inst_t* ip = frame->ip;
    scon_handle_t* base = state.regs + frame->base;
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
    };

#define LABEL_C_OP(_label, _decodeA, _decodeB, ...) \
    _label: \
    { \
        _decodeA; \
        _decodeB; \
        DECODE_C_REG(); \
        __VA_ARGS__ \
    } \
    _label##_k: \
    { \
        _decodeA; \
        _decodeB; \
        DECODE_C_CONST(); \
        __VA_ARGS__ \
    }

    DISPATCH();
label_none:
    frame->ip = ip;
    SCON_THROW(scon, "invalid opcode");
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
    if (!SCON_HANDLE_IS_TRUTHY(scon, &base[a]))
    {
        ip += sbx;
    }
    DISPATCH();
}
label_jmpt:
{
    DECODE_A();
    DECODE_SBX();
    if (SCON_HANDLE_IS_TRUTHY(scon, &base[a]))
    {
        ip += sbx;
    }
    DISPATCH();
}
LABEL_C_OP(label_call, DECODE_A(), DECODE_B(), {
    if (!SCON_HANDLE_IS_ITEM(&valC))
    {
        frame->ip = ip;
        SCON_THROW(scon, "attempt to call non-callable value");
    }
    scon_item_t* item = SCON_HANDLE_TO_ITEM(&valC);
    if (item->type == SCON_ITEM_TYPE_CLOSURE)
    {
        scon_closure_t* closure = &item->closure;
        if (b != closure->function->arity)
        {
            frame->ip = ip;
            SCON_THROW(scon, "invalid number of arguments");
        }

        frame->ip = ip;
        scon_eval_push_frame(scon, &state, closure, frame->base + a);

        frame = &state.frames[state.frameCount - 1];
        ip = frame->ip;
        base = state.regs + frame->base;
        constants = frame->closure->constants;

        DISPATCH();
    }
    if (item->flags & SCON_ITEM_FLAG_NATIVE)
    {
        scon_handle_t* args = &base[a];
        frame->ip = ip;
        base[a] = item->atom.native(scon, b, args);

        frame = &state.frames[state.frameCount - 1];
        base = state.regs + frame->base;
        constants = frame->closure->constants;

        DISPATCH();
    }

    frame->ip = ip;
    SCON_THROW(scon, "attempt to call non-callable value");
})
LABEL_C_OP(label_mov, DECODE_A(), (void)0, {
    base[a] = valC;
    DISPATCH();
})
LABEL_C_OP(label_ret, (void)0, (void)0, {
    state.regs[frame->target] = valC;
    scon_eval_pop_frame(&state);

    if (state.frameCount == 0)
    {
        result = valC;
        goto eval_end;
    }

    frame = &state.frames[state.frameCount - 1];
    ip = frame->ip;
    base = state.regs + frame->base;
    constants = frame->closure->constants;

    DISPATCH();
})
LABEL_C_OP(label_append, DECODE_A(), (void)0, {
    if (!SCON_HANDLE_IS_ITEM(&base[a]))
    {
        frame->ip = ip;
        SCON_THROW(scon, "APPEND target is not a list");
    }
    scon_item_t* item = SCON_HANDLE_TO_ITEM(&base[a]);
    if (item->type != SCON_ITEM_TYPE_LIST)
    {
        frame->ip = ip;
        SCON_THROW(scon, "APPEND target is not a list");
    }

    scon_list_t* listPtr = &item->list;

    scon_item_t* itemToAppend;
    if (SCON_HANDLE_IS_ITEM(&valC))
    {
        itemToAppend = SCON_HANDLE_TO_ITEM(&valC);
    }
    else if (SCON_HANDLE_IS_INT(&valC))
    {
        itemToAppend = SCON_CONTAINER_OF(scon_atom_lookup_int(scon, SCON_HANDLE_TO_INT(&valC)), scon_item_t, atom);
    }
    else
    {
        itemToAppend = SCON_CONTAINER_OF(scon_atom_lookup_float(scon, SCON_HANDLE_TO_FLOAT(&valC)), scon_item_t, atom);
    }

    scon_list_append(scon, listPtr, itemToAppend);
    DISPATCH();
})
LABEL_C_OP(label_eq, DECODE_A(), DECODE_B(), {
    base[a] = SCON_HANDLE_COMPARE_FAST(scon, &base[b], &valC, ==) ? SCON_HANDLE_TRUE() : SCON_HANDLE_FALSE();
    DISPATCH();
})
LABEL_C_OP(label_neq, DECODE_A(), DECODE_B(), {
    base[a] = SCON_HANDLE_COMPARE_FAST(scon, &base[b], &valC, !=) ? SCON_HANDLE_TRUE() : SCON_HANDLE_FALSE();
    DISPATCH();
})
LABEL_C_OP(label_seq, DECODE_A(), DECODE_B(), {
    base[a] = scon_handle_is_equal(scon, &base[b], &valC) ? SCON_HANDLE_TRUE() : SCON_HANDLE_FALSE();
    DISPATCH();
})
LABEL_C_OP(label_sneq, DECODE_A(), DECODE_B(), {
    base[a] = scon_handle_is_equal(scon, &base[b], &valC) ? SCON_HANDLE_FALSE() : SCON_HANDLE_TRUE();
    DISPATCH();
})
LABEL_C_OP(label_lt, DECODE_A(), DECODE_B(), {
    base[a] = SCON_HANDLE_COMPARE_FAST(scon, &base[b], &valC, <) ? SCON_HANDLE_TRUE() : SCON_HANDLE_FALSE();
    DISPATCH();
})
LABEL_C_OP(label_le, DECODE_A(), DECODE_B(), {
    base[a] = SCON_HANDLE_COMPARE_FAST(scon, &base[b], &valC, <=) ? SCON_HANDLE_TRUE() : SCON_HANDLE_FALSE();
    DISPATCH();
})
LABEL_C_OP(label_gt, DECODE_A(), DECODE_B(), {
    base[a] = SCON_HANDLE_COMPARE_FAST(scon, &base[b], &valC, >) ? SCON_HANDLE_TRUE() : SCON_HANDLE_FALSE();
    DISPATCH();
})
LABEL_C_OP(label_ge, DECODE_A(), DECODE_B(), {
    base[a] = SCON_HANDLE_COMPARE_FAST(scon, &base[b], &valC, >=) ? SCON_HANDLE_TRUE() : SCON_HANDLE_FALSE();
    DISPATCH();
})
LABEL_C_OP(label_add, DECODE_A(), DECODE_B(), {
    SCON_HANDLE_ARITHMETIC_FAST(scon, &base[a], &base[b], &valC, +);
    DISPATCH();
})
LABEL_C_OP(label_sub, DECODE_A(), DECODE_B(), {
    SCON_HANDLE_ARITHMETIC_FAST(scon, &base[a], &base[b], &valC, -);
    DISPATCH();
})
LABEL_C_OP(label_mul, DECODE_A(), DECODE_B(), {
    SCON_HANDLE_ARITHMETIC_FAST(scon, &base[a], &base[b], &valC, *);
    DISPATCH();
})
LABEL_C_OP(label_div, DECODE_A(), DECODE_B(), {
    SCON_HANDLE_ARITHMETIC_FAST(scon, &base[a], &base[b], &valC, /);
    DISPATCH();
})
LABEL_C_OP(label_mod, DECODE_A(), DECODE_B(), {
    scon_promotion_t prom;
    scon_handle_promote(scon, &base[b], &valC, &prom);
    if (prom.type == SCON_PROMOTION_TYPE_INT)
    {
        if (prom.b.intVal == 0)
        {
            frame->ip = ip;
            SCON_THROW(scon, "modulo by zero");
        }
        base[a] = SCON_HANDLE_FROM_INT(prom.a.intVal % prom.b.intVal);
    }
    else
    {
        frame->ip = ip;
        SCON_THROW(scon, "invalid item type");
    }
    DISPATCH();
})
LABEL_C_OP(label_band, DECODE_A(), DECODE_B(), {
    base[a] = SCON_HANDLE_FROM_INT(scon_handle_get_int(scon, &base[b]) & scon_handle_get_int(scon, &valC));
    DISPATCH();
})
LABEL_C_OP(label_bor, DECODE_A(), DECODE_B(), {
    base[a] = SCON_HANDLE_FROM_INT(scon_handle_get_int(scon, &base[b]) | scon_handle_get_int(scon, &valC));
    DISPATCH();
})
LABEL_C_OP(label_bxor, DECODE_A(), DECODE_B(), {
    base[a] = SCON_HANDLE_FROM_INT(scon_handle_get_int(scon, &base[b]) ^ scon_handle_get_int(scon, &valC));
    DISPATCH();
})
LABEL_C_OP(label_bnot, DECODE_A(), (void)0, {
    base[a] = SCON_HANDLE_FROM_INT(~scon_handle_get_int(scon, &valC));
    DISPATCH();
})
LABEL_C_OP(label_shl, DECODE_A(), DECODE_B(), {
    scon_int64_t right = scon_handle_get_int(scon, &valC);
    if (right < 0 || right >= 64)
    {
        frame->ip = ip;
        SCON_THROW(scon, "invalid shift amount");
    }
    base[a] = SCON_HANDLE_FROM_INT(scon_handle_get_int(scon, &base[b]) << right);
    DISPATCH();
})
LABEL_C_OP(label_shr, DECODE_A(), DECODE_B(), {
    scon_int64_t right = scon_handle_get_int(scon, &valC);
    if (right < 0 || right >= 64)
    {
        frame->ip = ip;
        SCON_THROW(scon, "invalid shift amount");
    }
    base[a] = SCON_HANDLE_FROM_INT(scon_handle_get_int(scon, &base[b]) >> right);
    DISPATCH();
})
label_closure:
{
    DECODE_A();
    scon_uint32_t c = SCON_INST_GET_C(inst);
    scon_handle_t protoHandle = frame->closure->constants[c];
    if (!SCON_HANDLE_IS_ITEM(&protoHandle))
    {
        frame->ip = ip;
        SCON_THROW(scon, "CLOSURE prototype is not a function");
    }

    if (SCON_HANDLE_TO_ITEM(&protoHandle)->type != SCON_ITEM_TYPE_FUNCTION)
    {
        frame->ip = ip;
        SCON_THROW(scon, "CLOSURE prototype is not a function");
    }

    scon_function_t* proto = &SCON_HANDLE_TO_ITEM(&protoHandle)->function;
    base[a] = SCON_HANDLE_FROM_CLOSURE(scon_closure_new(scon, proto));
    DISPATCH();
}
LABEL_C_OP(label_capture, DECODE_A(), DECODE_B(), {
    scon_handle_t closureHandle = base[a];
    scon_closure_t* closurePtr = &SCON_HANDLE_TO_ITEM(&closureHandle)->closure;
    closurePtr->constants[b] = valC;
    DISPATCH();
})
eval_end:
    scon_eval_state_deinit(&state);
    return result;
}

#endif
