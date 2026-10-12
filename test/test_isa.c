/* Prüft isa.h und isa.c gegen die Spezifikation KRON-ISA.md:
 * - das Feldlayout füllt genau das 36-Bit-Wort (Kapitel 2)
 * - die Kodierungsbeispiele aus Kapitel 6 und 8 ergeben die angegebenen Hex-Werte
 * - isa_lookup() findet die eingetragenen Befehle und nur diese */

#include "isa.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            printf("FEHLER %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
            failures++;                                                 \
        }                                                               \
    } while (0)

/* Setzt ein Befehlswort ausschließlich über die Makros aus isa.h zusammen. */
static uint64_t encode(unsigned prefix, unsigned controls, unsigned subopcode,
                       unsigned rd, unsigned rs1, unsigned operand)
{
    uint64_t word = 0;
    word |= ((uint64_t)prefix    & KRON_MASK(KRON_PREFIX_WIDTH))    << KRON_PREFIX_POS;
    word |= ((uint64_t)controls  & KRON_MASK(KRON_CONTROLS_WIDTH))  << KRON_CONTROLS_POS;
    word |= ((uint64_t)subopcode & KRON_MASK(KRON_SUBOPCODE_WIDTH)) << KRON_SUBOPCODE_POS;
    word |= ((uint64_t)rd        & KRON_MASK(KRON_REGISTER_WIDTH))  << KRON_RD_POS;
    word |= ((uint64_t)rs1       & KRON_MASK(KRON_REGISTER_WIDTH))  << KRON_RS1_POS;
    word |= ((uint64_t)operand   & KRON_MASK(KRON_OPERAND_WIDTH))   << KRON_OPERAND_POS;
    return word;
}

/* Kodiert einen Befehl aus der Tabelle; Prefix, Controls und Subopcode kommen aus isa_lookup(). */
static uint64_t encode_entry(const char *name, unsigned rd, unsigned rs1, unsigned operand)
{
    const Isa_Entry *entry = isa_lookup(name);
    CHECK(entry != NULL);
    if (entry == NULL)
        return 0;
    return encode(entry->Prefix, entry->Controls, entry->Subopcode, rd, rs1, operand);
}

static void check_word(const char *text, uint64_t actual, uint64_t expected)
{
    if (actual != expected) {
        printf("FEHLER %-20s erwartet %09" PRIX64 ", erhalten %09" PRIX64 "\n", text, expected, actual);
        failures++;
    }
}

static void test_layout(void)
{
    /* Die Felder liegen lückenlos von Bit 35 bis Bit 0. */
    CHECK(KRON_PREFIX_POS + KRON_PREFIX_WIDTH == KRON_WORD_WIDTH);
    CHECK(KRON_CONTROLS_POS + KRON_CONTROLS_WIDTH == KRON_PREFIX_POS);
    CHECK(KRON_SUBOPCODE_POS + KRON_SUBOPCODE_WIDTH == KRON_CONTROLS_POS);
    CHECK(KRON_RD_POS + KRON_REGISTER_WIDTH == KRON_SUBOPCODE_POS);
    CHECK(KRON_RS1_POS + KRON_REGISTER_WIDTH == KRON_RD_POS);
    CHECK(KRON_OPERAND_POS + KRON_OPERAND_WIDTH == KRON_RS1_POS);
    CHECK(KRON_OPERAND_POS == 0);

    /* Control-Bits liegen im Control-Feld. */
    CHECK(KRON_CTRL_ALU_IMM == 0x4 && KRON_CTRL_ALU_SUPPRESS == 0x2);
    CHECK(KRON_CTRL_JUMP_LINK == 0x4 && KRON_CTRL_JUMP_IND == 0x2);

    CHECK(KRON_REGISTER_COUNT == (1 << KRON_REGISTER_WIDTH));
}

static void test_spec_examples(void)
{
    /* Kapitel 6 */
    check_word("LI R1, 0",            encode_entry("LI", 1, 0, 0),         0x408200000ULL);
    check_word("ADD R1, R1, R2",      encode_entry("ADD", 1, 1, 2),        0x004210002ULL);
    check_word("ADDI R1, R1, 1",      encode_entry("ADDI", 1, 1, 1),       0x204210001ULL);
    check_word("CMP R1, R2",          encode(PREFIX_ALU, KRON_CTRL_ALU_SUPPRESS, ALU_SUB, 0, 1, 2),
                                                                           0x108010002ULL);
    check_word("STORE R1, 0xF00(R0)", encode_entry("STORE", 1, KRON_REG_ZERO, 0xF00), 0x404200F00ULL);
    check_word("JMP 0x002",           encode_entry("JMP", 0, 0, 0x002),    0x800000002ULL);
    check_word("JEQ 0x002",           encode(PREFIX_JUMP, 0, JUMP_EQ, 0, 0, 0x002),
                                                                           0x804000002ULL);
    check_word("RET",                 encode(PREFIX_JUMP, KRON_CTRL_JUMP_IND, JUMP_ALWAYS, 0, 0, KRON_REG_LINK),
                                                                           0x90000001FULL);

    /* Kapitel 8 */
    check_word("CALLEQ 0x010",        encode(PREFIX_JUMP, KRON_CTRL_JUMP_LINK, JUMP_EQ, 0, 0, 0x010),
                                                                           0xA04000010ULL);
    check_word("RETNE",               encode(PREFIX_JUMP, KRON_CTRL_JUMP_IND, JUMP_NE, 0, 0, KRON_REG_LINK),
                                                                           0x90800001FULL);

    /* RET ist dasselbe Wort wie JMPR R31 */
    check_word("JMPR R31",            encode_entry("JMPR", 0, 0, KRON_REG_LINK), 0x90000001FULL);
}

static void test_lookup(void)
{
    const char *known[] = {"ADD", "ADDI", "SUB", "SUBI", "LI", "STORE", "LOAD", "JMP", "JMPR", "NOP", "HALT"};

    for (size_t i = 0; i < sizeof(known) / sizeof(known[0]); i++) {
        const Isa_Entry *entry = isa_lookup(known[i]);
        CHECK(entry != NULL);
        if (entry != NULL)
            CHECK(strcmp(entry->name, known[i]) == 0);
    }

    CHECK(isa_lookup("add") == NULL);  // Groß-/Kleinschreibung zählt
    CHECK(isa_lookup("FOO") == NULL);
    CHECK(isa_lookup("") == NULL);

    const Isa_Entry *nop = isa_lookup("NOP");
    CHECK(nop != NULL && encode(nop->Prefix, nop->Controls, nop->Subopcode, 0, 0, 0) == 0);  // NOP = Nullwort
}

int main(void)
{
    test_layout();
    test_spec_examples();
    test_lookup();

    if (failures == 0)
        printf("Alle Tests bestanden.\n");
    return failures == 0 ? 0 : 1;
}
