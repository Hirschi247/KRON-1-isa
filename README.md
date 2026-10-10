# KRON-1 ISA

Gemeinsame Definition des KRON-1-Befehlssatzes als kleine C-Bibliothek. Sie ist die **einzige
Stelle**, an der Bitpositionen, Feldbreiten, Opcodes und Mnemonics festgelegt sind. Assembler und
Simulator binden sie ein, damit beide garantiert dasselbe Befehlsformat verwenden.

| Datei     | Inhalt |
|-----------|--------|
| `isa.h`   | Bitfelder (`KRON_*_POS`, `KRON_*_WIDTH`), Control-Bits, feste Register, `KRON_MASK`, Enums für Prefixe, Subopcodes und Condition-Codes, Formatklassen, `Isa_Entry` |
| `isa.c`   | Befehlstabelle `isa_table` und `isa_lookup()` |

Die verbindliche Spezifikation des Befehlssatzes ist `KRON-ISA.md` (im übergeordneten
`KRON1`-Ordner). Diese Bibliothek setzt sie in Code um. Weichen beide voneinander ab, ist eins
von beiden falsch.

---

## Befehlsformat in Kürze

Jeder Befehl ist ein **36-Bit-Wort** mit einheitlichem Feldlayout:

```
 35 34 | 33 32 31 | 30 ...  26 | 25 ... 21 | 20 ... 16 | 15 ............ 0
prefix | control  | subopcode  |    rd     |    rs1    |     operand
  2    |    3     |     5      |    5      |    5      |       16
```

| Prefix | Kategorie | Bedeutung von `control` |
|--------|-----------|-------------------------|
| `00`   | ALU       | `[33]` imm, `[32]` suppress |
| `01`   | Memory    | reserviert |
| `10`   | Jump      | `[33]` link, `[32]` ind – `subopcode` enthält hier die Bedingung |
| `11`   | reserviert | |

32 Register `R0` … `R31`: `R0` ist fest 0, `R30` der Stack-Pointer, `R31` das Link-Register.

---

## Benutzung

### Einbinden mit CMake

Das Repo ist eine statische CMake-Bibliothek mit dem Target `isa`. Das Include-Verzeichnis wird
automatisch weitergegeben:

```cmake
add_subdirectory(../isa isa)          # Pfad relativ zum eigenen Projekt
target_link_libraries(mein_target PUBLIC isa)
```

Danach reicht im Code:

```c
#include "isa.h"
```

### Befehl nachschlagen

```c
const Isa_Entry *entry = isa_lookup("STORE");

if (entry == NULL)
{
    // unbekanntes Mnemonic
}

// entry->format    -> FMT_ROR
// entry->Prefix    -> PREFIX_MEM
// entry->Subopcode -> MEM_STORE
```

`isa_lookup()` vergleicht exakt (Groß-/Kleinschreibung zählt) und gibt `NULL` zurück, wenn es das
Mnemonic nicht gibt.

### Felder kodieren und dekodieren

Die Makros werden immer paarweise benutzt: `_WIDTH` zum Maskieren, `_POS` zum Verschieben.

```c
// Kodieren: rd in das Befehlswort schreiben
word |= ((uint64_t)rd & KRON_MASK(KRON_REGISTER_WIDTH)) << KRON_RD_POS;

// Dekodieren: rd aus dem Befehlswort lesen
uint8_t rd = (word >> KRON_RD_POS) & KRON_MASK(KRON_REGISTER_WIDTH);

// Einzelnes Control-Bit prüfen (absolute Bitposition)
int is_imm = (word >> KRON_ALU_IMM_BIT) & 1;
```

Feste Zahlen wie `21` oder `0x1F` gehören nicht in den Code von Assembler oder Simulator, sondern
immer die Makros aus `isa.h`. Nur so wirkt sich eine Änderung am Format überall gleich aus.

---

## Aufbau von `Isa_Entry`

```c
typedef struct
{
    const char *name;         // Mnemonic, z.B. "ADD"
    enum Formatclass format;  // Operandenform, bestimmt das Parsen im Assembler
    enum Prefix Prefix;       // ALU, MEM oder JUMP
    uint8_t Controls;         // Wert des 3-Bit-Felds control [33:31]
    uint8_t Subopcode;        // Operation bzw. Bedingung bei Jumps
} Isa_Entry;
```

**`Controls`** ist der Wert des ganzen 3-Bit-Felds, nicht die absolute Bitposition. Bit 2 steht für
`control[33]`, Bit 1 für `control[32]`. Beispiele:

| Befehl | control `[33:31]` | `Controls` |
|--------|-------------------|------------|
| `ADD`  | `000`             | `0x0` |
| `ADDI` | `100` (imm)       | `0x4` |
| `CMP`  | `010` (suppress)  | `0x2` |
| `CALL` | `100` (link)      | `0x4` |
| `RET`  | `010` (ind)       | `0x2` |

### Formatklassen

| Formatklasse | Operanden                | Beispiel |
|--------------|--------------------------|----------|
| `FMT_RRR`    | Register, Register, Register | `ADD R1, R2, R3` |
| `FMT_RRI`    | Register, Register, Immediate | `ADDI R1, R2, 5` |
| `FMT_ROR`    | Register, Offset(Register) | `STORE R1, 0xF00(R0)` |
| `FMT_RLI`    | Register, 16-Bit-Immediate | `LI R1, 42` |
| `FMT_LIOR`   | Adresse oder Register     | `JMP loop`, `JMP R5` |
| `FMT_NOP`    | keine                    | `NOP`, `HALT` |

---

## Stand der Befehlstabelle

Aktuell in `isa_table` eingetragen:

| Mnemonic | Format     | Prefix | Subopcode   |
|----------|------------|--------|-------------|
| `ADD`    | `FMT_RRR`  | ALU    | `ALU_ADD`   |
| `LI`     | `FMT_RLI`  | MEM    | `MEM_LI`    |
| `STORE`  | `FMT_ROR`  | MEM    | `MEM_STORE` |
| `JMP`    | `FMT_LIOR` | JUMP   | `JUMP_ALWAYS` |
| `NOP`    | `FMT_NOP`  | ALU    | `ALU_NOP`   |
| `HALT`   | `FMT_NOP`  | ALU    | `ALU_HALT`  |

Die Subopcodes für `SUB`, `AND`, `OR`, `XOR`, `NOT`, `SHL`, `SHR`, `LOAD` und alle bedingten
Sprünge sind in `isa.h` schon definiert, fehlen aber noch in der Tabelle. Die I-Formen (`ADDI`, …),
`CMP`/`TEST`, `CALL` und `RET` stehen bisher nur in `KRON-ISA.md`.

### Neuen Befehl eintragen

1. Zeile in `isa_table` in `isa.c` ergänzen:
   ```c
   {.name = "SUB", .format = FMT_RRR, .Prefix = PREFIX_ALU, .Controls = 0x00, .Subopcode = ALU_SUB},
   ```
2. Falls nötig, neuen Subopcode oder Condition-Code im passenden Enum in `isa.h` ergänzen.
3. Braucht der Befehl eine neue Operandenform: neue `Formatklasse` in `isa.h` anlegen und den
   passenden Encoder in `asmcore` implementieren.
4. Assembler und Simulator neu bauen.

---

## Wer nutzt die Bibliothek

| Projekt | Einbindung |
|---------|------------|
| [assembler](https://github.com/Hirschi247/assembler) / [asmcore](https://github.com/Hirschi247/asmcore) | `add_subdirectory(../../isa isa)` aus `Assembler/assembler` |
| [Aeon-Kron-1-Simulator](https://github.com/Hirschi247/Aeon-Kron-1-Simulator) | `add_subdirectory(../isa isa)` |

Beide erwarten das Repo als Ordner **`isa`** direkt im `KRON1`-Ordner:

```
KRON1/
├── isa/                    ← dieses Repo
├── Aeon-Kron-1-Simulator/
└── Assembler/
    ├── asmcore/
    └── assembler/
```

Klonen deshalb mit Zielordner:

```bash
git clone https://github.com/Hirschi247/kron-1-isa.git isa
```

Eine Änderung hier wirkt sich auf Assembler **und** Simulator aus. Nach Änderungen am Format also
beide neu bauen und testen.

---

## Bauen

Voraussetzungen: C11-Compiler und **CMake ≥ 4.3**. Allein gebaut entsteht nur die statische
Bibliothek (`libisa.a` bzw. `isa.lib`):

```bash
cmake -S . -B build
cmake --build build
```

Normalerweise baut man sie aber nicht einzeln, sondern sie wird über `add_subdirectory` vom
Assembler oder Simulator mitgebaut.
