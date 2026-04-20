#ifndef SCON_DISASM_IMPL_H
#define SCON_DISASM_IMPL_H 1

#include "compile.h"
#include "disasm.h"

SCON_API void scon_disasm(scon_t* scon, scon_function_t* function, scon_file_t out)
{
    if (scon == SCON_NULL || function == SCON_NULL || out == SCON_NULL)
    {
        return;
    }

    SCON_FPRINTF(out, "===== Instructions =====\n");
    for (scon_size_t i = 0; i < function->instCount; ++i)
    {
        scon_inst_t inst = function->insts[i];
        scon_opcode_t op = SCON_INST_GET_OP_BASE(inst);
        scon_bool_t isConst = (SCON_INST_GET_OP(inst) & SCON_MODE_CONST) != 0;
        scon_uint32_t a = SCON_INST_GET_A(inst);
        scon_uint32_t b = SCON_INST_GET_B(inst);
        scon_uint32_t c = SCON_INST_GET_C(inst);

        SCON_FPRINTF(out, "%04u  ", (unsigned int)i);

        switch (op)
        {
        case SCON_OPCODE_LIST:
            SCON_FPRINTF(out, "LIST         R%u\n", a);
            break;
        case SCON_OPCODE_JMP:
            SCON_FPRINTF(out, "JMP          %d\n", (int)SCON_INST_GET_SBX(inst));
            break;
        case SCON_OPCODE_JMP_FALSE:
            SCON_FPRINTF(out, "JMP_FALSE    R%u %d\n", a, (int)SCON_INST_GET_SBX(inst));
            break;
        case SCON_OPCODE_JMP_TRUE:
            SCON_FPRINTF(out, "JMP_TRUE     R%u %d\n", a, (int)SCON_INST_GET_SBX(inst));
            break;
        case SCON_OPCODE_CALL:
            SCON_FPRINTF(out, "CALL         R%u %u %c%u\n", a, b, isConst ? 'K' : 'R', c);
            break;
        case SCON_OPCODE_RETURN:
            SCON_FPRINTF(out, "RETURN       %c%u\n", isConst ? 'K' : 'R', c);
            break;
        case SCON_OPCODE_APPEND:
            SCON_FPRINTF(out, "APPEND       R%u %c%u\n", a, isConst ? 'K' : 'R', c);
            break;
        case SCON_OPCODE_MOVE:
            SCON_FPRINTF(out, "MOVE         R%u %c%u\n", a, isConst ? 'K' : 'R', c);
            break;
        case SCON_OPCODE_EQUAL:
        case SCON_OPCODE_NOT_EQUAL:
        case SCON_OPCODE_STRICT_EQUAL:
        case SCON_OPCODE_LESS:
        case SCON_OPCODE_LESS_EQUAL:
        case SCON_OPCODE_GREATER:
        case SCON_OPCODE_GREATER_EQUAL:
        {
            const char* opName = "UNKNOWN";
            switch (op)
            {
            case SCON_OPCODE_EQUAL:
                opName = "EQUAL";
                break;
            case SCON_OPCODE_NOT_EQUAL:
                opName = "NOT_EQUAL";
                break;
            case SCON_OPCODE_STRICT_EQUAL:
                opName = "STRICT_EQ";
                break;
            case SCON_OPCODE_LESS:
                opName = "LESS";
                break;
            case SCON_OPCODE_LESS_EQUAL:
                opName = "LESS_EQ";
                break;
            case SCON_OPCODE_GREATER:
                opName = "GREATER";
                break;
            case SCON_OPCODE_GREATER_EQUAL:
                opName = "GREATER_EQ";
                break;
            }
            SCON_FPRINTF(out, "%-12s R%u R%u %c%u\n", opName, a, b, isConst ? 'K' : 'R', c);
        }
        break;
        case SCON_OPCODE_ADD:
        case SCON_OPCODE_SUB:
        case SCON_OPCODE_MUL:
        case SCON_OPCODE_DIV:
        case SCON_OPCODE_MOD:
        {
            const char* opName = "UNKNOWN";
            switch (op)
            {
            case SCON_OPCODE_ADD:
                opName = "ADD";
                break;
            case SCON_OPCODE_SUB:
                opName = "SUB";
                break;
            case SCON_OPCODE_MUL:
                opName = "MUL";
                break;
            case SCON_OPCODE_DIV:
                opName = "DIV";
                break;
            case SCON_OPCODE_MOD:
                opName = "MOD";
                break;
            }
            SCON_FPRINTF(out, "%-12s R%u R%u %c%u\n", opName, a, b, isConst ? 'K' : 'R', c);
        }
        break;
        case SCON_OPCODE_BIT_AND:
        case SCON_OPCODE_BIT_OR:
        case SCON_OPCODE_BIT_XOR:
        case SCON_OPCODE_BIT_SHL:
        case SCON_OPCODE_BIT_SHR:
        {
            const char* opName = "UNKNOWN";
            switch (op)
            {
            case SCON_OPCODE_BIT_AND:
                opName = "BIT_AND";
                break;
            case SCON_OPCODE_BIT_OR:
                opName = "BIT_OR";
                break;
            case SCON_OPCODE_BIT_XOR:
                opName = "BIT_XOR";
                break;
            case SCON_OPCODE_BIT_SHL:
                opName = "BIT_SHL";
                break;
            case SCON_OPCODE_BIT_SHR:
                opName = "BIT_SHR";
                break;
            }
            SCON_FPRINTF(out, "%-12s R%u R%u %c%u\n", opName, a, b, isConst ? 'K' : 'R', c);
        }
        break;
        case SCON_OPCODE_BIT_NOT:
            SCON_FPRINTF(out, "%-12s R%u %c%u\n", "BIT_NOT", a, isConst ? 'K' : 'R', c);
            break;
        case SCON_OPCODE_CLOSURE:
            SCON_FPRINTF(out, "CLOSURE      R%u K%u\n", a, c);
            break;
        case SCON_OPCODE_CAPTURE:
            SCON_FPRINTF(out, "CAPTURE      R%u %u %c%u\n", a, b, isConst ? 'K' : 'R', c);
            break;
        default:
            SCON_FPRINTF(out, "UNKNOWN      OP%u\n", (unsigned int)op);
            break;
        }
    }

    if (function->constantCount > 0)
    {
        SCON_FPRINTF(out, "\n===== Constants =====\n");
        for (scon_uint16_t i = 0; i < function->constantCount; ++i)
        {
            scon_const_slot_t* slot = &function->constants[i];
            if (slot->type == SCON_CONST_SLOT_ITEM)
            {
                scon_item_t* item = slot->item;
                if (item->type == SCON_ITEM_TYPE_ATOM)
                {
                    SCON_FPRINTF(out, "K%03u  %.*s\n", (unsigned int)i, (int)item->atom.length, item->atom.string);
                }
                else if (item->type == SCON_ITEM_TYPE_LIST)
                {
                    SCON_FPRINTF(out, "K%03u  (list of %u items)\n", (unsigned int)i, (unsigned int)item->list.length);
                }
                else if (item->type == SCON_ITEM_TYPE_FUNCTION)
                {
                    SCON_FPRINTF(out, "K%03u  (function %p)\n", (unsigned int)i, (void*)item);
                }
                else
                {
                    SCON_FPRINTF(out, "K%03u  (unknown type)\n", (unsigned int)i);
                }
            }
            else
            {
                SCON_FPRINTF(out, "K%03u  (capture %.*s)\n", (unsigned int)i, (int)slot->capture->length,
                    slot->capture->string);
            }
        }
    }
}

#endif
