#ifndef ISA_LIBRARY_H
#define ISA_LIBRARY_H

#include <stdint.h>

/* ---------- Befehlswort ---------- */
#define KRON_WORD_WIDTH             36

/* ---------- Felder ---------- */
#define KRON_PREFIX_POS             34
#define KRON_PREFIX_WIDTH           2

#define KRON_CONTROLS_POS           31
#define KRON_CONTROLS_WIDTH         3

#define KRON_SUBOPCODE_POS          26
#define KRON_SUBOPCODE_WIDTH        5

#define KRON_RD_POS                 21
#define KRON_RS1_POS                16
#define KRON_REGISTER_WIDTH         5

#define KRON_OPERAND_POS            0
#define KRON_OPERAND_WIDTH          16

#define KRON_JUMP_ADDR_POS          0

/* Register im Operand-Feld (ALU-rs2, Jump-Register) */
#define KRON_OPERAND_REG_POS        0

/* ---------- Sprechende Aliase (keine eigenen Zahlen!) ---------- */
#define KRON_MEM_RD_RSRC_POS        KRON_RD_POS
#define KRON_MEM_RBASE_POS          KRON_RS1_POS
#define KRON_ALU_RS2_POS            KRON_OPERAND_REG_POS
#define KRON_JUMP_REG_POS           KRON_OPERAND_REG_POS
#define KRON_JUMP_COND_POS          KRON_SUBOPCODE_POS

/* ---------- Control-Bits (absolute Bitposition) ---------- */
#define KRON_ALU_IMM_BIT            33
#define KRON_ALU_SUPPRESS_BIT       32
#define KRON_JUMP_LINK_BIT          33
#define KRON_JUMP_IND_BIT           32

/* ---------- Control-Masken (relativ zum Control-Feld, für Isa_Entry.Controls) ---------- */
#define KRON_CTRL_ALU_IMM           (1u << (KRON_ALU_IMM_BIT      - KRON_CONTROLS_POS))
#define KRON_CTRL_ALU_SUPPRESS      (1u << (KRON_ALU_SUPPRESS_BIT - KRON_CONTROLS_POS))
#define KRON_CTRL_JUMP_LINK         (1u << (KRON_JUMP_LINK_BIT    - KRON_CONTROLS_POS))
#define KRON_CTRL_JUMP_IND          (1u << (KRON_JUMP_IND_BIT     - KRON_CONTROLS_POS))

/* ---------- Genutzte Teilbreiten ---------- */
#define KRON_MEM_SUBOPCODE_WIDTH    2
#define KRON_COND_WIDTH             3
#define KRON_JUMP_ADDR_WIDTH        12
#define KRON_SHIFT_WIDTH            4

/* ---------- Feste Register ---------- */

#define KRON_REGISTER_COUNT         32
#define KRON_REG_ZERO               0
#define KRON_REG_SP                 30
#define KRON_REG_LINK               31

#define KRON_MASK(width) ((1ULL << (width)) - 1)

enum Prefix {
    PREFIX_ALU = 0x00,
    PREFIX_MEM = 0x01,
    PREFIX_JUMP = 0x02
};

enum Alu_Subopcode {
    ALU_NOP = 0x00,
    ALU_ADD = 0x01,
    ALU_SUB = 0x02,
    ALU_AND = 0x03,
    ALU_OR = 0x04,
    ALU_XOR = 0x05,
    ALU_NOT = 0x06,
    ALU_SHL = 0x07,
    ALU_SHR = 0x08,
    ALU_HALT = 0x1F
};

enum Mem_Subopcode {
    MEM_LOAD = 0x00,
    MEM_STORE = 0x01,
    MEM_LI = 0x02
};

enum Jump_Condition_Code {
    JUMP_ALWAYS = 0x00,
    JUMP_EQ = 0x01,
    JUMP_NE = 0x02,
    JUMP_LT = 0x03,
    JUMP_LE = 0x04,
    JUMP_GT = 0x05,
    JUMP_GE = 0x06
};

/* Operandenform eines Befehls. Wird noch nicht ausgewertet; gedacht für eine spätere Prüfung
 * von Anzahl und Art der Operanden in encode(). */
enum Formatclass {
    FMT_RRR, // Register, Register, Register   z. B. ADD rd, rs1, rs2
    FMT_RRI, // Register, Register, Immediate  z. B. ADDI rd, rs1, 5
    FMT_ROR, // Register, Offset(Register)     z. B. STORE rsrc, offset(rbase)
    FMT_RLI, // Register, 16-Bit-Immediate     z. B. LI rd, 5
    FMT_R,   // nur Register                   z. B. JMPR rx
    FMT_LI,  // nur Adresse (12 Bit)           z. B. JMP 0x100
    FMT_NOP  // keine Operanden                z. B. NOP, HALT
};

typedef struct
{
    const char *name;         // Mnemonic, z. B. "ADD"
    enum Formatclass format;  // Operandenform
    enum Prefix Prefix;       // ALU, MEM oder JUMP
    uint8_t Controls;         // Wert des 3-Bit-Felds control [33:31], aus KRON_CTRL_* zusammengesetzt
    uint8_t Subopcode;        // Operation bzw. Bedingung bei Jumps
} Isa_Entry;

const Isa_Entry *isa_lookup (const char *name);

#endif // ISA_LIBRARY_H