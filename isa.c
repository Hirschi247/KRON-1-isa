#include "isa.h"

#include <string.h>

static const Isa_Entry isa_table[] = {
    {.name = "ADD", .format = FMT_RRR, .Prefix = PREFIX_ALU, .Controls = 0x00, .Subopcode = ALU_ADD},
    {.name = "LI", .format = FMT_RLI, .Prefix = PREFIX_MEM, .Controls = 0x00, .Subopcode = MEM_LI},
    {.name = "STORE", .format = FMT_ROR, .Prefix = PREFIX_MEM, .Controls = 0x00, .Subopcode = MEM_STORE},
    {.name = "JMP", .format = FMT_LIOR, .Prefix = PREFIX_JUMP, .Controls = 0x00, .Subopcode = JUMP_ALWAYS},
    {.name = "NOP", .format = FMT_NOP, .Prefix = PREFIX_ALU, .Controls = 0x00, .Subopcode = ALU_NOP},
    {.name = "HALT", .format = FMT_NOP, .Prefix = PREFIX_ALU, .Controls = 0x00, .Subopcode = ALU_HALT}
};

const Isa_Entry *isa_lookup (const char *name) {
    for (int i = 0; i < sizeof(isa_table) / sizeof(isa_table[0]); i++) {
        if (strcmp(name, isa_table[i].name) == 0)
            return &isa_table[i];
    }

    return NULL;
}