#ifndef SCON_DISASM_IMPL_H
#define SCON_DISASM_IMPL_H 1

#include "compile.h"
#include "disasm.h"

static void scon_disasm_internal(scon_t* scon, scon_function_t* function, scon_file_t out)
{
    if (scon == SCON_NULL || function == SCON_NULL || out == SCON_NULL)
    {
        return;
    }

    SCON_FPRINTF(out, "================================================================================\n");
    SCON_FPRINTF(out, "Function: %p\n", (void*)function);
    SCON_FPRINTF(out, "Arity: %u\n", (unsigned int)function->arity);
    SCON_FPRINTF(out, "Instruction count: %u\n", (unsigned int)function->instCount);
    SCON_FPRINTF(out, "Constant count: %u\n", (unsigned int)function->constantCount);
    SCON_FPRINTF(out, "--------------------------------------------------------------------------------\n");

    for (scon_size_t i = 0; i < function->instCount; ++i)
    {
        scon_inst_t inst = function->insts[i];
        scon_opcode_t op = SCON_INST_GET_OP_BASE(inst);
        scon_bool_t isConst = (SCON_INST_GET_OP(inst) & SCON_MODE_CONST) != 0;
        scon_uint32_t a = SCON_INST_GET_A(inst);
        scon_uint32_t b = SCON_INST_GET_B(inst);
        scon_uint32_t c = SCON_INST_GET_C(inst);
        scon_uint32_t sbx = SCON_INST_GET_SBX(inst);

        const char* opName = "UNKNOWN";
        switch (op)
        {
        case SCON_OPCODE_LIST:
            opName = "LIST";
            break;
        case SCON_OPCODE_JMP:
            opName = "JMP";
            break;
        case SCON_OPCODE_JMPF:
            opName = "JMPF";
            break;
        case SCON_OPCODE_JMPT:
            opName = "JMPT";
            break;
        case SCON_OPCODE_CALL:
            opName = "CALL";
            break;
        case SCON_OPCODE_RET:
            opName = "RET";
            break;
        case SCON_OPCODE_APPEND:
            opName = "APPEND";
            break;
        case SCON_OPCODE_MOV:
            opName = "MOV";
            break;
        case SCON_OPCODE_EQ:
            opName = "EQ";
            break;
        case SCON_OPCODE_NEQ:
            opName = "NEQ";
            break;
        case SCON_OPCODE_SEQ:
            opName = "SEQ";
            break;
        case SCON_OPCODE_SNEQ:
            opName = "SNEQ";
            break;
        case SCON_OPCODE_LT:
            opName = "LT";
            break;
        case SCON_OPCODE_LE:
            opName = "LE";
            break;
        case SCON_OPCODE_GT:
            opName = "GT";
            break;
        case SCON_OPCODE_GE:
            opName = "GE";
            break;
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
        case SCON_OPCODE_BAND:
            opName = "BAND";
            break;
        case SCON_OPCODE_BOR:
            opName = "BOR";
            break;
        case SCON_OPCODE_BXOR:
            opName = "BXOR";
            break;
        case SCON_OPCODE_BNOT:
            opName = "BNOT";
            break;
        case SCON_OPCODE_SHL:
            opName = "SHL";
            break;
        case SCON_OPCODE_SHR:
            opName = "SHR";
            break;
        case SCON_OPCODE_CLOSURE:
            opName = "CLOSURE";
            break;
        case SCON_OPCODE_CAPTURE:
            opName = "CAPTURE";
            break;
        default:
            break;
        }

        SCON_FPRINTF(out, "[%04u] %-12s ", (unsigned int)i, opName);

        switch (SCON_INST_GET_OP_BASE(inst))
        {
        case SCON_OPCODE_LIST:
            SCON_FPRINTF(out, "R%-5u %-6s %-6s", a, "", "");
            break;
        case SCON_OPCODE_JMP:
            SCON_FPRINTF(out, "%-6d %-6s %-6s", sbx, "", "");
            break;
        case SCON_OPCODE_JMPF:
        case SCON_OPCODE_JMPT:
            SCON_FPRINTF(out, "R%-5u %-6d %-6s", a, sbx, "");
            break;
        case SCON_OPCODE_CALL:
        case SCON_OPCODE_CAPTURE:
            SCON_FPRINTF(out, "R%-5u %-6u %c%-5u", a, b, isConst ? 'K' : 'R', c);
            break;
        case SCON_OPCODE_RET:
            SCON_FPRINTF(out, "%c%-5u %-6s %-6s", isConst ? 'K' : 'R', c, "", "");
            break;
        case SCON_OPCODE_APPEND:
        case SCON_OPCODE_MOV:
        case SCON_OPCODE_BNOT:
            SCON_FPRINTF(out, "R%-5u %-6s %c%-5u", a, "", isConst ? 'K' : 'R', c);
            break;
        case SCON_OPCODE_CLOSURE:
            SCON_FPRINTF(out, "R%-5u %-6s K%-5u", a, "", c);
            break;
        default:
            SCON_FPRINTF(out, "R%-5u R%-5u %c%-5u", a, b, isConst ? 'K' : 'R', c);
            break;
        }

        scon_bool_t hasInlineConst = SCON_FALSE;
        if (op == SCON_OPCODE_CLOSURE || isConst)
        {
            hasInlineConst = SCON_TRUE;
        }

        if (op == SCON_OPCODE_JMP || op == SCON_OPCODE_JMPF || op == SCON_OPCODE_JMPT)
        {
            int target = (int)i + 1 + sbx;
            SCON_FPRINTF(out, " ; -> [%04u]\n", target);
        }
        else if (hasInlineConst && c < function->constantCount)
        {
            scon_const_slot_t* slot = &function->constants[c];
            SCON_FPRINTF(out, " ; ");
            if (slot->type == SCON_CONST_SLOT_ITEM)
            {
                scon_item_t* item = slot->item;
                if (item->type == SCON_ITEM_TYPE_ATOM)
                {
                    SCON_FPRINTF(out, "\"%.*s\"\n", (int)item->atom.length, item->atom.string);
                }
                else if (item->type == SCON_ITEM_TYPE_LIST)
                {
                    SCON_FPRINTF(out, "(list of %u items)\n", (unsigned int)item->list.length);
                }
                else if (item->type == SCON_ITEM_TYPE_FUNCTION)
                {
                    SCON_FPRINTF(out, "(function %p)\n", (void*)&item->function);
                }
                else
                {
                    SCON_FPRINTF(out, "(unknown item)\n");
                }
            }
            else if (slot->type == SCON_CONST_SLOT_CAPTURE)
            {
                SCON_FPRINTF(out, "(capture %.*s)\n", (int)slot->capture->length, slot->capture->string);
            }
            else
            {
                SCON_FPRINTF(out, "(none)\n");
            }
        }
        else
        {
            SCON_FPRINTF(out, "\n");
        }
    }

    if (function->constantCount > 0)
    {
        SCON_FPRINTF(out, "--------------------------------------------------------------------------------\n");
        for (scon_uint16_t i = 0; i < function->constantCount; ++i)
        {
            scon_const_slot_t* slot = &function->constants[i];
            if (slot->type == SCON_CONST_SLOT_ITEM)
            {
                scon_item_t* item = slot->item;
                if (item->type == SCON_ITEM_TYPE_ATOM)
                {
                    SCON_FPRINTF(out, "[K%03u] \"%.*s\"\n", (unsigned int)i, (int)item->atom.length, item->atom.string);
                }
                else if (item->type == SCON_ITEM_TYPE_LIST)
                {
                    SCON_FPRINTF(out, "[K%03u] (list of %u items)\n", (unsigned int)i, (unsigned int)item->list.length);
                }
                else if (item->type == SCON_ITEM_TYPE_FUNCTION)
                {
                    SCON_FPRINTF(out, "[K%03u] (function %p)\n", (unsigned int)i, (void*)&item->function);
                }
                else
                {
                    SCON_FPRINTF(out, "[K%03u] (unknown type)\n", (unsigned int)i);
                }
            }
            else if (slot->type == SCON_CONST_SLOT_CAPTURE)
            {
                SCON_FPRINTF(out, "[K%03u] (capture %.*s)\n", (unsigned int)i, (int)slot->capture->length,
                    slot->capture->string);
            }
            else
            {
                SCON_FPRINTF(out, "[K%03u] (none)\n", (unsigned int)i);
            }
        }
    }

    SCON_FPRINTF(out, "================================================================================\n");

    for (scon_uint16_t i = 0; i < function->constantCount; ++i)
    {
        scon_const_slot_t* slot = &function->constants[i];
        if (slot->type == SCON_CONST_SLOT_ITEM)
        {
            scon_item_t* item = slot->item;
            if (item->type == SCON_ITEM_TYPE_FUNCTION)
            {
                scon_disasm_internal(scon, &item->function, out);
            }
        }
    }
}

SCON_API void scon_disasm(scon_t* scon, scon_function_t* function, scon_file_t out)
{
    scon_disasm_internal(scon, function, out);
}

#endif
