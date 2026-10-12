# KRON-1 — Instruction Set Architecture

**Stand:** 2026-10-11
**Zweck:** Verbindliche Referenz für das Befehlsformat. Assembler und Simulator richten sich nach diesem Dokument, ebenso die geplanten Komponenten KronC (Compiler) und Verilog-Decoder. Weicht Code davon ab, ist der Code falsch — oder dieses Dokument wird bewusst geändert.

Das Dokument beschreibt das **Soll-Verhalten**. Nicht alles davon ist schon umgesetzt; den aktuellen Stand der Befehlstabelle zeigt die [README](README.md).

---

## 1. Grundparameter

| Parameter | Wert |
|---|---|
| Registerbreite | 16 bit |
| Register | 32 GPRs, `R0`–`R31` (Registerfeld 5 bit) |
| Instruction Word | 36 bit, feste Breite |
| Architektur | Harvard |
| Instruction Memory | 4096 × 36 bit (12-bit Adresse) |
| Data Memory | 4096 × 16 bit (12-bit Adresse) |
| Flags | Z (Zero), N (Negative), V (Overflow) — folgen in einer späteren Ausbaustufe |
| `.mem`-Format | 9 Hex-Stellen pro Zeile |

### Registerrollen

| Register | Rolle |
|---|---|
| `R0` | hardwired `0x0000`, Schreibzugriffe werden verworfen |
| `R1`–`R29` | frei |
| `R30` | Stack-Pointer (Softwarekonvention) |
| `R31` | Link-Register (Hardware-Ziel bei `link = 1`) |

Register werden im Assembler **groß** geschrieben (`R0`–`R31`). `r1` ist kein Register, sondern wird als Label gelesen.

---

## 2. Einheitliches Feldlayout

Alle Instruktionen benutzen dieses Layout. Jedes Feld liegt bei jedem Prefix an derselben Stelle; Prefixe unterscheiden sich nur darin, welche Felder sie benutzen.

```
 35 34 | 33 32 31 | 30 ...  26 | 25 ... 21 | 20 ... 16 | 15 ............ 0
prefix | control  | subopcode  |    rd     |    rs1    |     operand
  2    |    3     |     5      |    5      |    5      |       16
```

| Feld | Bits | Breite |
|---|---|---|
| prefix | `[35:34]` | 2 |
| control | `[33:31]` | 3 |
| subopcode | `[30:26]` | 5 |
| rd | `[25:21]` | 5 |
| rs1 | `[20:16]` | 5 |
| operand | `[15:0]` | 16 |

Liegt im Operand-Feld eine Registernummer (rs2 bei der ALU, Zielregister beim indirekten Jump), steht sie immer in **`[4:0]`**; `[15:5]` sind dann reserviert.

### Regel für reservierte Bits

**Reservierte Bits müssen 0 sein.** Der Assembler setzt sie immer auf 0. Nur so können sie später rückwärtskompatibel belegt werden.

### Prefixe

| Prefix | Kategorie |
|---|---|
| `00` | ALU |
| `01` | Memory |
| `10` | Jump |
| `11` | frei / reserviert |

### Übersicht: Feldnutzung pro Prefix

| Feld | ALU `00` | Memory `01` | Jump `10` |
|---|---|---|---|
| control `[33]` | imm | reserviert | link |
| control `[32]` | suppress | reserviert | ind |
| control `[31]` | reserviert | reserviert | reserviert |
| subopcode `[30:26]` | Operation | Operation (`[27:26]`) | cond (`[28:26]`) |
| rd `[25:21]` | rd | rd / rsrc | reserviert |
| rs1 `[20:16]` | rs1 | rbase | reserviert |
| operand `[15:0]` | rs2 `[4:0]` oder imm16 | offset / imm16 | address `[11:0]` oder reg `[4:0]` |

---

## 3. ALU (Prefix `00`)

```
[35:34] prefix   = 00
[33]    imm      — 0: op2 = rs2,  1: op2 = imm16
[32]    suppress — 1: kein Writeback, nur Flags
[31]    reserviert
[30:26] subopcode
[25:21] rd
[20:16] rs1

  imm = 0:  [15:5] reserviert, [4:0] rs2
  imm = 1:  [15:0] imm16
```

### Subopcodes

| subopcode | Register-Form | I-Form (imm = 1) | Semantik |
|---|---|---|---|
| `00000` | NOP | — | keine Wirkung |
| `00001` | ADD | ADDI | `rd = rs1 + op2` |
| `00010` | SUB | SUBI | `rd = rs1 - op2` |
| `00011` | AND | ANDI | `rd = rs1 & op2` |
| `00100` | OR | ORI | `rd = rs1 \| op2` |
| `00101` | XOR | XORI | `rd = rs1 ^ op2` |
| `00110` | NOT | — | `rd = ~rs1` |
| `00111` | SHL | SHLI | `rd = rs1 << op2[3:0]` |
| `01000` | SHR | SHRI | `rd = rs1 >> op2[3:0]` (logisch) |
| `01001`–`11110` | frei | | Platz für SAR, ROL, ROR, MUL, ADC, SBC … |
| `11111` | HALT | — | CPU bleibt stehen, PC wird nicht mehr erhöht |

- Register- und I-Form teilen sich den Subopcode. Der Assembler setzt `imm` anhand des **Mnemonics**, nicht des Operanden. `ADD R1, R1, 5` und `ADDI R1, R1, R2` sind Syntaxfehler.
- imm16 ist volle 16 bit, keine Sign-Extension. Der Assembler akzeptiert −32768 bis 65535.
- **Shifts:** Die Hardware benutzt nur `op2[3:0]`. In der I-Form lehnt der Assembler Werte > 15 ab.
- `subopcode = 00000` als NOP: ein nullgefüllter Instruction Memory ist harmlos.
- NOT, NOP, HALT: nicht benutzte Felder sind reserviert.

### suppress und Vergleichs-Aliase

Mit `suppress = 1` wird das Ergebnis nicht geschrieben, nur die Flags. `rd` ist dann reserviert (0).

| Alias | entspricht | Zweck |
|---|---|---|
| `CMP rs1, rs2` | SUB mit suppress | Vergleich für bedingte Sprünge |
| `CMPI rs1, imm` | SUBI mit suppress | Vergleich mit Konstante |
| `TEST rs1, rs2` | AND mit suppress | Bits prüfen, ohne Register zu verändern |
| `TESTI rs1, imm` | ANDI mit suppress | z. B. Statusbit abfragen |

---

## 4. Memory (Prefix `01`)

```
[35:34] prefix  = 01
[33:31] control   — reserviert
[30:26] subopcode — nur [27:26] genutzt, [30:28] = 0
[25:21] rd / rsrc
[20:16] rbase   — bei LI reserviert
[15:0]  offset (signed) / imm16 bei LI
```

| subopcode | Mnemonic | Semantik | Felder |
|---|---|---|---|
| `00000` | LOAD | `rd = MEM[rbase + offset]` | rd, rbase, offset |
| `00001` | STORE | `MEM[rbase + offset] = rsrc` | rsrc, rbase, offset |
| `00010` | LI | `rd = imm16` | rd, imm16 |
| `00011` | reserviert | — | — |

- Effektive Adresse wird in 16 bit berechnet, nur `[11:0]` adressieren den Speicher.
- Es gibt genau **eine** Adressierungsart: `rbase + offset`.

| Adressierung | Schreibweise |
|---|---|
| direkt / absolut | `LOAD R1, 0x123(R0)` |
| register-indirekt | `LOAD R1, 0(R5)` |
| register-indirekt mit Offset | `LOAD R1, 4(R5)` |
| Konstante speichern | `LI R2, 42` + `STORE R2, 0xF00(R0)` |

- LOAD, STORE, LI verändern **keine** Flags.

---

## 5. Jump (Prefix `10`)

```
[35:34] prefix  = 10
[33]    link    — 1: Rücksprungadresse (PC + 1) nach R31
[32]    ind     — 0: Ziel = Literal,  1: Ziel = Registerinhalt (register-indirekt)
[31]    reserviert
[30:26] cond    — nur [28:26] genutzt, [30:29] = 0
[25:16] reserviert

  ind = 0:  [15:12] reserviert, [11:0] address
  ind = 1:  [15:5]  reserviert, [4:0]  reg
```

### Keine eigenen Subopcodes

Beim Jump enthält das Subopcode-Feld `[30:26]` die **Bedingung (cond)**. Es gibt keine separaten Operationscodes: Welche Instruktion gemeint ist, ergibt sich vollständig aus `cond` + `link` + `ind`. Die drei sind orthogonal, jede Kombination ist gültig.

### Mnemonics

| Mnemonic | control `[33:31]` (link, ind, –) | subopcode `[30:26]` (cond) | Operand |
|---|---|---|---|
| `JMP addr` | `000` | `00000` ALWAYS | address |
| `JEQ addr` | `000` | `00001` EQ | address |
| `JNE addr` | `000` | `00010` NE | address |
| `JLT addr` | `000` | `00011` LT | address |
| `JLE addr` | `000` | `00100` LE | address |
| `JGT addr` | `000` | `00101` GT | address |
| `JGE addr` | `000` | `00110` GE | address |
| `CALL addr` | `100` | `00000` ALWAYS | address |
| `JMPR rx` | `010` | `00000` ALWAYS | reg = rx |
| `CALLR rx` | `110` | `00000` ALWAYS | reg = rx |
| `RET` | `010` | `00000` ALWAYS | reg = R31 |

- Jedes Mnemonic hat genau eine Operandenform. Wie bei der ALU (`ADD`/`ADDI`) entscheidet das **Mnemonic** über `ind`, nicht der Operand: `JMP`/`CALL` nehmen eine Adresse, `JMPR`/`CALLR` ein Register. `JMP R5` und `JMPR loop` sind Syntaxfehler.

### Condition-Codes

| cond | Name | Bedingung |
|---|---|---|
| `000` | ALWAYS | immer wahr |
| `001` | EQ | `Z == 1` |
| `010` | NE | `Z == 0` |
| `011` | LT | `N ⊕ V == 1` |
| `100` | LE | `Z == 1` oder `N ⊕ V == 1` |
| `101` | GT | `Z == 0` und `N ⊕ V == 0` |
| `110` | GE | `N ⊕ V == 0` |
| `111` | reserviert | — |

- Bedingungen gelten für signed Vergleiche.
- `link = 1`: R31 wird **nur** geschrieben, wenn tatsächlich gesprungen wird. Der 12-bit PC wird zero-extended abgelegt.
- `ind = 1`: vom Registerinhalt zählen nur `[11:0]`.
- Jumps verändern keine Flags.

---

## 6. Kodierungsbeispiele

| Instruktion | Hex |
|---|---|
| `LI R1, 0` | `408200000` |
| `ADD R1, R1, R2` | `004210002` |
| `ADDI R1, R1, 1` | `204210001` |
| `CMP R1, R2` | `108010002` |
| `STORE R1, 0xF00(R0)` | `404200F00` |
| `JMP 0x002` | `800000002` |
| `JEQ 0x002` | `804000002` |
| `RET` | `90000001F` |

---

## 7. Hinweise für den Decoder

- `rd` und `rs1` gehen fest an die Registerfile-Adressen.
- Der **zweite Leseport** braucht einen Mux für seine Adresse: `operand[4:0]` (ALU-rs2, Jump-reg) oder `rd`-Feld (STORE-rsrc).
- PC-Next-Mux: `PC + 1` · `address[11:0]` · Registerinhalt `[11:0]`.
- Bitpositionen gehören an genau eine Stelle. Für Assembler und Simulator ist das [`isa.h`](isa.h); ein späterer Verilog-Decoder sollte seine Konstanten ebenfalls daraus ableiten.

---

## 8. Offene Punkte

- **Flag-Verhalten pro Operation:** Welche ALU-Ops setzen Z/N/V? Insbesondere: Was passiert mit V bei AND/OR/XOR/NOT/Shifts (unverändert oder gelöscht)? Wird in einer folgenden Ausbaustufe festgelegt, zusammen mit den Flags.
- **`control[31]`**, Kandidaten:
  - Jump: `rel` — PC-relative Sprünge (Operand als signed Offset)
  - Memory: Auto-Inkrement/-Dekrement von rbase (PUSH/POP in einer Instruktion)
  - ALU: Carry-in für ADC/SBC
- **Memory `control[33:32]`:** frei.
- **Bedingte Calls und bedingte indirekte Sprünge** (z. B. `CALL` nur bei EQ, bedingtes RET) sind kodierbar und von der Hardware abgedeckt, haben aber noch keine Mnemonics. Namen unten sind Vorschläge.

  | Beispiel | control `[33:31]` | subopcode `[30:26]` | Operand | Hex |
  |---|---|---|---|---|
  | `CALLEQ 0x010` | `100` | `00001` EQ | address = `0x010` | `A04000010` |
  | `RETNE` | `010` | `00010` NE | reg = R31 | `90800001F` |

  **Bedingter Call** — Funktion nur aufrufen, wenn R1 == 0:
  ```asm
          CMP    R1, R0
          CALLEQ handle_zero      ; springt nur bei Z = 1, nur dann wird R31 geschrieben
          ...                     ; hierher geht es in beiden Fällen weiter

  ; heute ohne Mnemonic:
          CMP    R1, R0
          JNE    skip
          CALL   handle_zero
  skip:   ...
  ```

  **Bedingter Return** — früh aus einer Funktion zurückkehren, wenn R2 ≠ 5:
  ```asm
  func:   CMPI   R2, 5
          RETNE                   ; zurück, wenn R2 != 5
          ...                     ; nur bei R2 == 5
          RET

  ; heute ohne Mnemonic:
  func:   CMPI   R2, 5
          JNE    out
          ...
  out:    RET
  ```
