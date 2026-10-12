# KRON-1 ISA

Der Befehlssatz (Instruction Set Architecture) der **KRON-1**, einer eigenen 16-Bit-CPU mit
36-Bit-Befehlswort, als kleine C-Bibliothek.

Zum KRON-1-Projekt gehören ein [Assembler](https://github.com/Hirschi247/assembler) und ein
[Simulator](https://github.com/Hirschi247/Aeon-Kron-1-Simulator). Beide nutzen dieses Repo als
**Single Source of Truth**: Bitpositionen, Feldbreiten, Opcodes und Mnemonics sind nur hier
festgelegt. So verwenden Assembler und Simulator garantiert dasselbe Befehlsformat.

Die vollständige Spezifikation mit Semantik aller Befehle steht in **[KRON-ISA.md](KRON-ISA.md)**.
Diese Bibliothek setzt sie in Code um.

| Datei     | Inhalt |
|-----------|--------|
| `isa.h`   | Bitfelder (`KRON_*_POS`, `KRON_*_WIDTH`), Control-Bits (`KRON_*_BIT`) und Control-Masken (`KRON_CTRL_*`), feste Register, `KRON_MASK`, Enums für Prefixe, Subopcodes und Condition-Codes, Formatklassen, `Isa_Entry` |
| `isa.c`   | Befehlstabelle `isa_table` und `isa_lookup()` |
| `test/test_isa.c` | Prüft Feldlayout, Befehlstabelle und die Kodierungsbeispiele aus der Spezifikation |

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
    enum Formatclass format;  // Operandenform (wird noch nicht ausgewertet)
    enum Prefix Prefix;       // ALU, MEM oder JUMP
    uint8_t Controls;         // Wert des 3-Bit-Felds control [33:31]
    uint8_t Subopcode;        // Operation bzw. Bedingung bei Jumps
} Isa_Entry;
```

**`Controls`** ist der Wert des ganzen 3-Bit-Felds, nicht die absolute Bitposition. Bit 2 steht für
`control[33]`, Bit 1 für `control[32]`. Statt Zahlen nimmt man die Masken `KRON_CTRL_*` aus `isa.h`.
Sie werden aus den absoluten Bitpositionen (`KRON_*_BIT`) abgeleitet. Mehrere Bits verknüpft man
mit `|`. Beispiele:

| Befehl  | control `[33:31]`   | `Controls` |
|---------|---------------------|------------|
| `ADD`   | `000`               | `0` |
| `ADDI`  | `100` (imm)         | `KRON_CTRL_ALU_IMM` (`0x4`) |
| `CMP`   | `010` (suppress)    | `KRON_CTRL_ALU_SUPPRESS` (`0x2`) |
| `CALL`  | `100` (link)        | `KRON_CTRL_JUMP_LINK` (`0x4`) |
| `JMPR`  | `010` (ind)         | `KRON_CTRL_JUMP_IND` (`0x2`) |
| `CALLR` | `110` (link + ind)  | `KRON_CTRL_JUMP_LINK \| KRON_CTRL_JUMP_IND` (`0x6`) |

Die absoluten Positionen `KRON_*_BIT` sind für das ganze Befehlswort gedacht, zum Beispiel beim
Dekodieren im Simulator. Auf `Controls` darf man sie nicht direkt anwenden.

### Formatklassen

Die Formatklasse beschreibt, welche Operanden ein Befehl erwartet. Der Assembler wertet sie
**noch nicht aus**: `encode()` in `asmcore` wählt die Kodierung nach `Prefix`, `Subopcode` und
`Controls` und prüft die Operanden nicht. Später soll damit geprüft werden, ob Anzahl und Art der
Operanden stimmen (z. B. `ADD R1, R2, 5` als Syntaxfehler).

| Formatklasse | Operanden                | Beispiel |
|--------------|--------------------------|----------|
| `FMT_RRR`    | Register, Register, Register | `ADD R1, R2, R3` |
| `FMT_RRI`    | Register, Register, Immediate | `ADDI R1, R2, 5` |
| `FMT_ROR`    | Register, Offset(Register) | `STORE R1, 0xF00(R0)` |
| `FMT_RLI`    | Register, 16-Bit-Immediate | `LI R1, 42` |
| `FMT_LI`     | Adresse                  | `JMP loop` |
| `FMT_R`      | Register                 | `JMPR R5` |
| `FMT_NOP`    | keine                    | `NOP`, `HALT` |

---

## Stand der Befehlstabelle

Aktuell in `isa_table` eingetragen:

| Mnemonic | Format     | Prefix | Controls             | Subopcode     |
|----------|------------|--------|----------------------|---------------|
| `ADD`    | `FMT_RRR`  | ALU    | `0`                  | `ALU_ADD`     |
| `ADDI`   | `FMT_RRI`  | ALU    | `KRON_CTRL_ALU_IMM`  | `ALU_ADD`     |
| `SUB`    | `FMT_RRR`  | ALU    | `0`                  | `ALU_SUB`     |
| `SUBI`   | `FMT_RRI`  | ALU    | `KRON_CTRL_ALU_IMM`  | `ALU_SUB`     |
| `LI`     | `FMT_RLI`  | MEM    | `0`                  | `MEM_LI`      |
| `STORE`  | `FMT_ROR`  | MEM    | `0`                  | `MEM_STORE`   |
| `LOAD`   | `FMT_ROR`  | MEM    | `0`                  | `MEM_LOAD`    |
| `JMP`    | `FMT_LI`   | JUMP   | `0`                  | `JUMP_ALWAYS` |
| `JMPR`   | `FMT_R`    | JUMP   | `KRON_CTRL_JUMP_IND` | `JUMP_ALWAYS` |
| `NOP`    | `FMT_NOP`  | ALU    | `0`                  | `ALU_NOP`     |
| `HALT`   | `FMT_NOP`  | ALU    | `0`                  | `ALU_HALT`    |

Die Subopcodes für `AND`, `OR`, `XOR`, `NOT`, `SHL`, `SHR` und alle bedingten Sprünge sind in
`isa.h` schon definiert, fehlen aber noch in der Tabelle. Die übrigen I-Formen (`ANDI`, …),
`CMP`/`TEST`, `CALL`/`CALLR` und `RET` sind in der [Spezifikation](KRON-ISA.md) beschrieben, aber
noch nicht umgesetzt.

### Neuen Befehl eintragen

1. Zeile in `isa_table` in `isa.c` ergänzen:
   ```c
   {.name = "AND", .format = FMT_RRR, .Prefix = PREFIX_ALU, .Controls = 0, .Subopcode = ALU_AND},
   ```
2. Falls nötig, neuen Subopcode oder Condition-Code im passenden Enum in `isa.h` ergänzen.
3. Braucht der Befehl eine neue Operandenform: neue `Formatklasse` in `isa.h` anlegen und
   `encode()` in `asmcore` so erweitern, dass es die neue Form übersetzt.
4. Assembler und Simulator neu bauen.

---

## Wer nutzt die Bibliothek

| Projekt | Beschreibung |
|---------|--------------|
| [assembler](https://github.com/Hirschi247/assembler) | Übersetzt KRON-1-Assembler in `.mem`-Dateien (Kernlogik in [asmcore](https://github.com/Hirschi247/asmcore)) |
| [Aeon-Kron-1-Simulator](https://github.com/Hirschi247/Aeon-Kron-1-Simulator) | Führt `.mem`-Dateien aus |

Beide binden dieses Repo als Ordner `isa` ein. Wie die Ordner genau liegen müssen, steht in der
README des jeweiligen Projekts. Wichtig ist nur, beim Klonen den Zielordner `isa` anzugeben:

```bash
git clone https://github.com/Hirschi247/kron-1-isa.git isa
```

Eine Änderung hier wirkt sich auf Assembler **und** Simulator aus. Nach Änderungen am Format also
beide neu bauen und testen.

---

## Bauen

Voraussetzungen: C11-Compiler und **CMake ≥ 3.16**. Allein gebaut entstehen die statische
Bibliothek (`libisa.a` bzw. `isa.lib`) und das Testprogramm:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Der Test kodiert die Beispiele aus Kapitel 6 und 8 der [Spezifikation](KRON-ISA.md) nur mit den
Makros aus `isa.h` und den Einträgen aus `isa_lookup()` und vergleicht sie mit den dort angegebenen
Hex-Werten. Weichen Code und Spezifikation voneinander ab, schlägt er fehl.

Normalerweise baut man die Bibliothek aber nicht einzeln, sondern sie wird über `add_subdirectory`
vom Assembler oder Simulator mitgebaut. Der Test wird dabei nicht mitgebaut.

---

## Lizenz

[MIT](LICENSE)
