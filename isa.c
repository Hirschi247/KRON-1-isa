#include "isa.h"

#include <string.h>

static const Isa_Entry isa_table[] = {
    {.name = "ADD",   .format = FMT_RRR,  .Prefix = PREFIX_ALU,   .Controls = 0,                  .Subopcode = ALU_ADD},
    {.name = "ADDI",  .format = FMT_RRI,  .Prefix = PREFIX_ALU,   .Controls = KRON_CTRL_ALU_IMM,  .Subopcode = ALU_ADD},
    {.name = "SUB",   .format = FMT_RRR,  .Prefix = PREFIX_ALU,   .Controls = 0,                  .Subopcode = ALU_SUB},
    {.name = "SUBI",  .format = FMT_RRI,  .Prefix = PREFIX_ALU,   .Controls = KRON_CTRL_ALU_IMM,  .Subopcode = ALU_SUB},
    {.name = "LI",    .format = FMT_RLI,  .Prefix = PREFIX_MEM,   .Controls = 0,                  .Subopcode = MEM_LI},
    {.name = "STORE", .format = FMT_ROR,  .Prefix = PREFIX_MEM,   .Controls = 0,                  .Subopcode = MEM_STORE},
    {.name = "LOAD",  .format = FMT_ROR,  .Prefix = PREFIX_MEM,   .Controls = 0,                  .Subopcode = MEM_LOAD},
    {.name = "JMP",   .format = FMT_LI,   .Prefix = PREFIX_JUMP,  .Controls = 0,                  .Subopcode = JUMP_ALWAYS},
    {.name = "JMPR",  .format = FMT_R,    .Prefix = PREFIX_JUMP,  .Controls = KRON_CTRL_JUMP_IND, .Subopcode = JUMP_ALWAYS},
    {.name = "NOP",   .format = FMT_NOP,  .Prefix = PREFIX_ALU,   .Controls = 0,                  .Subopcode = ALU_NOP},
    {.name = "HALT",  .format = FMT_NOP,  .Prefix = PREFIX_ALU,   .Controls = 0,                  .Subopcode = ALU_HALT}
};

const Isa_Entry *isa_lookup (const char *name) {
    for (int i = 0; i < sizeof(isa_table) / sizeof(isa_table[0]); i++) {
        if (strcmp(name, isa_table[i].name) == 0)
            return &isa_table[i];
    }

    return NULL;
}