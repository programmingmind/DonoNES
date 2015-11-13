#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#include "cpu.h"
#include "memory.h"

#define NUM_INDEX_MODES 9

typedef struct {
   const char *name;
   uint8_t opcode[NUM_INDEX_MODES];
   uint8_t cycles[NUM_INDEX_MODES];
   uint8_t extraCycles[NUM_INDEX_MODES];
   int (*execute)(uint8_t, uint16_t);
} instruction_t;

typedef struct {
   uint8_t carry            : 1;
   uint8_t zero             : 1;
   uint8_t interruptDisable : 1;
   uint8_t decimalMode      : 1;
   uint8_t breakCmd         : 1;
   uint8_t unused           : 1;
   uint8_t overflow         : 1;
   uint8_t negative         : 1;
} status_t;

typedef struct {
   uint8_t  A;
   uint8_t  X;
   uint8_t  Y;
   status_t P;
   uint8_t  SP;
   uint16_t PC;
} registers_t;

static registers_t registers;

int ADC(uint8_t val, uint16_t addr);
int SBC(uint8_t val, uint16_t addr);

int ASL(uint8_t val, uint16_t addr);
int LSR(uint8_t val, uint16_t addr);
int ROL(uint8_t val, uint16_t addr);
int ROR(uint8_t val, uint16_t addr);

int AND(uint8_t val, uint16_t addr);
int EOR(uint8_t val, uint16_t addr);
int ORA(uint8_t val, uint16_t addr);

int BCC(uint8_t val, uint16_t addr);
int BCS(uint8_t val, uint16_t addr);
int BEQ(uint8_t val, uint16_t addr);
int BMI(uint8_t val, uint16_t addr);
int BNE(uint8_t val, uint16_t addr);
int BPL(uint8_t val, uint16_t addr);
int BVC(uint8_t val, uint16_t addr);
int BVS(uint8_t val, uint16_t addr);

int CLC(uint8_t val, uint16_t addr);
int CLD(uint8_t val, uint16_t addr);
int CLI(uint8_t val, uint16_t addr);
int CLV(uint8_t val, uint16_t addr);

int SEC(uint8_t val, uint16_t addr);
int SED(uint8_t val, uint16_t addr);
int SEI(uint8_t val, uint16_t addr);

int CMP(uint8_t val, uint16_t addr);
int CPX(uint8_t val, uint16_t addr);
int CPY(uint8_t val, uint16_t addr);

int DEC(uint8_t val, uint16_t addr);
int DEX(uint8_t val, uint16_t addr);
int DEY(uint8_t val, uint16_t addr);

int INC(uint8_t val, uint16_t addr);
int INX(uint8_t val, uint16_t addr);
int INY(uint8_t val, uint16_t addr);

int JMP_ABS(uint8_t val, uint16_t addr);
int JMP_IND(uint8_t val, uint16_t addr);

int BRK(uint8_t val, uint16_t addr);
int JSR(uint8_t val, uint16_t addr);

int RTI(uint8_t val, uint16_t addr);
int RTS(uint8_t val, uint16_t addr);

int LDA(uint8_t val, uint16_t addr);
int LDX(uint8_t val, uint16_t addr);
int LDX_ZPY(uint8_t val, uint16_t addr);
int LDY(uint8_t val, uint16_t addr);

int STA(uint8_t val, uint16_t addr);
int STX(uint8_t val, uint16_t addr);
int STX_ZPY(uint8_t val, uint16_t addr);
int STY(uint8_t val, uint16_t addr);

int PHA(uint8_t val, uint16_t addr);
int PHP(uint8_t val, uint16_t addr);
int PLA(uint8_t val, uint16_t addr);
int PLP(uint8_t val, uint16_t addr);

int TAX(uint8_t val, uint16_t addr);
int TAY(uint8_t val, uint16_t addr);
int TSX(uint8_t val, uint16_t addr);

int TXA(uint8_t val, uint16_t addr);
int TYA(uint8_t val, uint16_t addr);
int TXS(uint8_t val, uint16_t addr);

int BIT(uint8_t val, uint16_t addr);

int NOP(uint8_t val, uint16_t addr);

// undocumented

int AAC(uint8_t val, uint16_t addr);

int XAA(uint8_t val, uint16_t addr);

int AAX(uint8_t val, uint16_t addr);
int AAX_ZPY(uint8_t val, uint16_t addr);
int LAX(uint8_t val, uint16_t addr);
int LAX_ZPY(uint8_t val, uint16_t addr);

int ARR(uint8_t val, uint16_t addr);
int ASR(uint8_t val, uint16_t addr);
int ATX(uint8_t val, uint16_t addr);
int AXS(uint8_t val, uint16_t addr);

int AXA(uint8_t val, uint16_t addr);
int SXA(uint8_t val, uint16_t addr);
int SYA(uint8_t val, uint16_t addr);

int DCP(uint8_t val, uint16_t addr);
int ISC(uint8_t val, uint16_t addr);
int RLA(uint8_t val, uint16_t addr);
int RRA(uint8_t val, uint16_t addr);
int SLO(uint8_t val, uint16_t addr);
int SRE(uint8_t val, uint16_t addr);

int LAR(uint8_t val, uint16_t addr);
int XAS(uint8_t val, uint16_t addr);

int DOP(uint8_t val, uint16_t addr);
int TOP(uint8_t val, uint16_t addr);
int KIL(uint8_t val, uint16_t addr);

static instruction_t InstructionTable[] = {
   //       Other
   //       Impl, Rel
   //        Acc, Imm,  ZP,   ZPX,  Abs,  AbsX, AbsY, IndX, IndY
   {"ADC", {0xFF, 0x69, 0x65, 0x75, 0x6D, 0x7D, 0x79, 0x61, 0x71}, {0, 2, 3, 4, 4, 4, 4, 6, 5}, {0, 0, 0, 0, 0, 1, 1, 0, 1}, ADC},
   {"SBC", {0xFF, 0xE9, 0xE5, 0xF5, 0xED, 0xFD, 0xF9, 0xE1, 0xF1}, {0, 2, 3, 4, 4, 4, 4, 6, 5}, {0, 0, 0, 0, 0, 1, 1, 0, 1}, SBC},

   {"ASL", {0x0A, 0xFF, 0x06, 0x16, 0x0E, 0x1E, 0xFF, 0xFF, 0xFF}, {2, 0, 5, 6, 6, 7, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, ASL},
   {"LSR", {0x4A, 0xFF, 0x46, 0x56, 0x4E, 0x5E, 0xFF, 0xFF, 0xFF}, {2, 0, 5, 6, 6, 7, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, LSR},
   {"ROL", {0x2A, 0xFF, 0x26, 0x36, 0x2E, 0x3E, 0xFF, 0xFF, 0xFF}, {2, 0, 5, 6, 6, 7, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, ROL},
   {"ROR", {0x6A, 0xFF, 0x66, 0x76, 0x6E, 0x7E, 0xFF, 0xFF, 0xFF}, {2, 0, 5, 6, 6, 7, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, ROR},

   {"AND", {0xFF, 0x29, 0x25, 0x35, 0x2D, 0x3D, 0x39, 0x21, 0x31}, {0, 2, 2, 3, 4, 4, 4, 6, 5}, {0, 0, 0, 0, 0, 1, 1, 0, 1}, AND},
   {"EOR", {0xFF, 0x49, 0x45, 0x55, 0x4D, 0x5D, 0x59, 0x41, 0x51}, {0, 2, 3, 4, 4, 4, 4, 6, 5}, {0, 0, 0, 0, 0, 1, 1, 0, 1}, EOR},
   {"ORA", {0xFF, 0x09, 0x05, 0x15, 0x0D, 0x1D, 0x19, 0x01, 0x11}, {0, 2, 2, 3, 4, 4, 4, 6, 5}, {0, 0, 0, 0, 0, 1, 1, 0, 1}, ORA},

   {"BCC", {0xFF, 0x90, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, BCC},
   {"BCS", {0xFF, 0xB0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, BCS},
   {"BEQ", {0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, BEQ},
   {"BMI", {0xFF, 0x30, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, BMI},
   {"BNE", {0xFF, 0xD0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, BNE},
   {"BPL", {0xFF, 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, BPL},
   {"BVC", {0xFF, 0x50, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, BVC},
   {"BVS", {0xFF, 0x70, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, BVS},

   {"CLC", {0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, CLC},
   {"CLD", {0xD8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, CLD},
   {"CLI", {0x58, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, CLI},
   {"CLV", {0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, CLV},

   {"SEC", {0x38, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, SEC},
   {"SED", {0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, SED},
   {"SEI", {0x78, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, SEI},

   {"CMP", {0xFF, 0xC9, 0xC5, 0xD5, 0xCD, 0xDD, 0xD9, 0xC1, 0xD1}, {0, 2, 3, 4, 4, 4, 4, 6, 5}, {0, 0, 0, 0, 0, 1, 1, 0, 1}, CMP},
   {"CPX", {0xFF, 0xE0, 0xE4, 0xFF, 0xEC, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 3, 0, 4, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, CPX},
   {"CPY", {0xFF, 0xC0, 0xC4, 0xFF, 0xCC, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 3, 0, 4, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, CPY},

   {"DEC", {0xFF, 0xFF, 0xC6, 0xD6, 0xCE, 0xDE, 0xFF, 0xFF, 0xFF}, {0, 0, 5, 6, 6, 7, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, DEC},
   {"DEX", {0xCA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, DEX},
   {"DEY", {0x88, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, DEY},

   {"INC", {0xFF, 0xFF, 0xE6, 0xF6, 0xEE, 0xFE, 0xFF, 0xFF, 0xFF}, {0, 0, 5, 6, 6, 7, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, INC},
   {"INX", {0xE8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, INX},
   {"INY", {0xC8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, INY},

   {"JMP", {0x4C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {3, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, JMP_ABS},
   {"JMP", {0x6C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {5, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, JMP_IND},

   {"BRK", {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {7, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, BRK},
   {"JSR", {0x20, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {6, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, JSR},

   {"RTI", {0x40, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {6, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, RTI},
   {"RTS", {0x60, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {6, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, RTS},

   {"LDA", {0xFF, 0xA9, 0xA5, 0xB5, 0xAD, 0xBD, 0xB9, 0xA1, 0xB1}, {0, 2, 3, 4, 4, 4, 4, 6, 5}, {0, 0, 0, 0, 0, 1, 1, 0, 1}, LDA},
   {"LDX", {0xFF, 0xA2, 0xA6, 0xFF, 0xAE, 0xFF, 0xBE, 0xFF, 0xFF}, {0, 2, 3, 0, 4, 0, 4, 0, 0}, {0, 0, 0, 0, 0, 0, 1, 0, 0}, LDX},
   {"LDX", {0xB6, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {4, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, LDX_ZPY},
   {"LDY", {0xFF, 0xA0, 0xA4, 0xB4, 0xAC, 0xBC, 0xFF, 0xFF, 0xFF}, {0, 2, 3, 4, 4, 4, 0, 0, 0}, {0, 0, 0, 0, 0, 1, 0, 0, 0}, LDY},

   {"STA", {0xFF, 0xFF, 0x85, 0x95, 0x8D, 0x9D, 0x99, 0x81, 0x91}, {0, 0, 3, 4, 4, 5, 5, 6, 6}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, STA},
   {"STX", {0xFF, 0xFF, 0x86, 0xFF, 0x8E, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 3, 0, 4, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, STX},
   {"STX", {0x96, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {4, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, STX_ZPY},
   {"STY", {0xFF, 0xFF, 0x84, 0x94, 0x8C, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 3, 4, 4, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, STY},

   {"PHA", {0x48, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {3, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, PHA},
   {"PHP", {0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {3, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, PHP},
   {"PLA", {0x68, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {4, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, PLA},
   {"PLP", {0x28, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {4, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, PLP},

   {"TAX", {0xAA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, TAX},
   {"TAY", {0xA8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, TAY},
   {"TSX", {0xBA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, TSX},

   {"TXA", {0x8A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, TXA},
   {"TYA", {0x98, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, TYA},
   {"TXS", {0x9A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, TXS},

   {"BIT", {0xFF, 0xFF, 0x24, 0xFF, 0x2C, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 3, 0, 4, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, BIT},
   
   {"NOP", {0xEA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, NOP},

   // Undocumented
   //       Other
   //       Impl, Rel
   //        Acc, Imm,  ZP,   ZPX,  Abs,  AbsX, AbsY, IndX, IndY

   {"SBC", {0xFF, 0xEB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, SBC},

   {"AAC", {0xFF, 0x0B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, AAC},
   {"AAC", {0xFF, 0x2B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, AAC},

   {"XAA", {0xFF, 0x8B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, XAA},

   {"AAX", {0xFF, 0xFF, 0x87, 0xFF, 0x8F, 0xFF, 0xFF, 0x83, 0xFF}, {0, 0, 3, 0, 4, 0, 0, 6, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, AAX},
   {"AAX", {0x97, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {4, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, AAX_ZPY},
   {"LAX", {0xFF, 0xFF, 0xA7, 0xFF, 0xAF, 0xFF, 0xBF, 0xA3, 0xB3}, {0, 0, 3, 0, 4, 0, 4, 6, 5}, {0, 0, 0, 0, 0, 0, 1, 0, 1}, LAX},
   {"LAX", {0xB7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {4, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, LAX_ZPY},

   {"ARR", {0xFF, 0x6B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, ARR},
   {"ASR", {0xFF, 0x4B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, ASR},
   {"ATX", {0xFF, 0xAB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, ATX},
   {"AXS", {0xFF, 0xCB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, AXS},

   {"AXA", {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF, 0x93}, {0, 0, 0, 0, 0, 0, 5, 0, 6}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, AXA},
   {"SXA", {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x9E, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 0, 5, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, SXA},
   {"SYA", {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x9C, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 5, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, SYA},

   {"DCP", {0xFF, 0xFF, 0xC7, 0xD7, 0xCF, 0xDF, 0xDB, 0xC3, 0xD3}, {0, 0, 5, 6, 6, 7, 7, 8, 8}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, DCP},
   {"ISC", {0xFF, 0xFF, 0xE7, 0xF7, 0xEF, 0xFF, 0xFB, 0xE3, 0xF3}, {0, 0, 5, 6, 6, 7, 7, 8, 8}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, ISC}, // SPECIAL
   {"RLA", {0xFF, 0xFF, 0x27, 0x37, 0x2F, 0x3F, 0x3B, 0x23, 0x33}, {0, 0, 5, 6, 6, 7, 7, 8, 8}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, RLA},
   {"RRA", {0xFF, 0xFF, 0x67, 0x77, 0x6F, 0x7F, 0x7B, 0x63, 0x73}, {0, 0, 5, 6, 6, 7, 7, 8, 8}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, RRA},
   {"SLO", {0xFF, 0xFF, 0x07, 0x17, 0x0F, 0x1F, 0x1B, 0x03, 0x13}, {0, 0, 5, 6, 6, 7, 7, 8, 8}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, SLO},
   {"SRE", {0xFF, 0xFF, 0x47, 0x57, 0x4F, 0x5F, 0x5B, 0x43, 0x53}, {0, 0, 5, 6, 6, 7, 7, 8, 8}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, SRE},

   {"LAR", {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBB, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 0, 4, 0, 0}, {0, 0, 0, 0, 0, 0, 1, 0, 0}, LAR},
   {"XAS", {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x9B, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 0, 5, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, XAS},

   {"NOP", {0x1A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, NOP},
   {"NOP", {0x3A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, NOP},
   {"NOP", {0x5A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, NOP},
   {"NOP", {0x7A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, NOP},
   {"NOP", {0xDA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, NOP},
   {"NOP", {0xFA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {2, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, NOP},

   {"DOP", {0xFF, 0x80, 0x04, 0x14, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 3, 4, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, DOP},
   {"DOP", {0xFF, 0x82, 0x44, 0x34, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 3, 4, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, DOP},
   {"DOP", {0xFF, 0x89, 0x64, 0x54, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 3, 4, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, DOP},
   {"DOP", {0xFF, 0xC2, 0xFF, 0x74, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 4, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, DOP},
   {"DOP", {0xFF, 0xE2, 0xFF, 0xD4, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 4, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, DOP},
   {"DOP", {0xFF, 0xFF, 0xFF, 0xF4, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 4, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, DOP},

   {"TOP", {0xFF, 0xFF, 0xFF, 0xFF, 0x0C, 0x1C, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 4, 4, 0, 0, 0}, {0, 0, 0, 0, 0, 1, 0, 0, 0}, TOP},
   {"TOP", {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3C, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 4, 0, 0, 0}, {0, 0, 0, 0, 0, 1, 0, 0, 0}, TOP},
   {"TOP", {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5C, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 4, 0, 0, 0}, {0, 0, 0, 0, 0, 1, 0, 0, 0}, TOP},
   {"TOP", {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7C, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 4, 0, 0, 0}, {0, 0, 0, 0, 0, 1, 0, 0, 0}, TOP},
   {"TOP", {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xDC, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 4, 0, 0, 0}, {0, 0, 0, 0, 0, 1, 0, 0, 0}, TOP},
   {"TOP", {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 4, 0, 0, 0}, {0, 0, 0, 0, 0, 1, 0, 0, 0}, TOP},

   {"KIL", {0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, KIL},
   {"KIL", {0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, KIL},
   {"KIL", {0x22, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, KIL},
   {"KIL", {0x32, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, KIL},
   {"KIL", {0x42, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, KIL},
   {"KIL", {0x52, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, KIL},
   {"KIL", {0x62, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, KIL},
   {"KIL", {0x72, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, KIL},
   {"KIL", {0x92, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, KIL},
   {"KIL", {0xB2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, KIL},
   {"KIL", {0xD2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, KIL},
   {"KIL", {0xF2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, KIL},

   {"???", {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, NULL}
};

uint8_t fetchPC();
uint16_t fetchPC16();

uint8_t registerFlags();

uint8_t Imm (uint8_t *pageBoundary, uint16_t *address);
uint8_t ZP  (uint8_t *pageBoundary, uint16_t *address);
uint8_t ZPX (uint8_t *pageBoundary, uint16_t *address);
uint8_t ZPY (uint8_t *pageBoundary, uint16_t *address);
uint8_t Abs (uint8_t *pageBoundary, uint16_t *address);
uint8_t AbsX(uint8_t *pageBoundary, uint16_t *address);
uint8_t AbsY(uint8_t *pageBoundary, uint16_t *address);
uint8_t IndX(uint8_t *pageBoundary, uint16_t *address);
uint8_t IndY(uint8_t *pageBoundary, uint16_t *address);

int runInstruction(instruction_t *inst, int ndx);

void initCPU() {
   registers.A  = 0;
   registers.X  = 0;
   registers.Y  = 0;
   registers.P  = {0,0,1,0,0,1,0,0};
   registers.SP = 0xFD;
   registers.PC = 0xC000;
}

void cleanCPU() {

}

int step() {
   uint16_t pc = registers.PC;
   uint8_t opcode = fetchPC();
   printf("0x%04X: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n", pc, opcode, registers.A, registers.X, registers.Y, registerFlags(), registers.SP);
   fflush(stdout);

   if (opcode != 0xFF) {
      int instNdx = 0;

      while (InstructionTable[instNdx].execute) {
         for (int ndx = 0; ndx < NUM_INDEX_MODES; ndx++) {
            if (InstructionTable[instNdx].opcode[ndx] == opcode) {
               return runInstruction(InstructionTable + instNdx, ndx);
            }
         }

         ++instNdx;
      }
   } else {
      int instNdx = 0;

      while (InstructionTable[instNdx].execute != ISC) {
         instNdx++;
      }

      return runInstruction(InstructionTable + instNdx, 5);
   }

   fprintf(stderr, "Invalid opcode 0x%02X\n", opcode);
   return 1;
}

int runInstruction(instruction_t *inst, int ndx) {
   uint8_t val;
   uint8_t pageBoundary;
   uint16_t address = 0;

   switch (ndx) {
      case 0:
         val = registers.A;
         pageBoundary = 0;
         break;
      case 1:
         val = Imm(&pageBoundary, &address);
         break;
      case 2:
         val = ZP(&pageBoundary, &address);
         break;
      case 3:
         val = ZPX(&pageBoundary, &address);
         break;
      case 4:
         val = Abs(&pageBoundary, &address);
         break;
      case 5:
         val = AbsX(&pageBoundary, &address);
         break;
      case 6:
         val = AbsY(&pageBoundary, &address);
         break;
      case 7:
         val = IndX(&pageBoundary, &address);
         break;
      case 8:
         val = IndY(&pageBoundary, &address);
         break;
      default:
         fprintf(stderr, "Wrong ndx %d\n", ndx);
         return 1;
   }

   return inst->execute(val, address) + inst->cycles[ndx] + (pageBoundary ? inst->extraCycles[ndx] : 0);
}

uint8_t fetchPC() {
   return fetch(registers.PC++);
}

uint16_t fetchPC16() {
   uint16_t ret = fetch16(registers.PC);
   registers.PC += 2;
   return ret;
}

uint8_t registerFlags() {
   return
      (registers.P.carry            ? 1<<0 : 0) |
      (registers.P.zero             ? 1<<1 : 0) |
      (registers.P.interruptDisable ? 1<<2 : 0) |
      (registers.P.decimalMode      ? 1<<3 : 0) |
      (registers.P.breakCmd         ? 1<<4 : 0) |
      (registers.P.unused           ? 1<<5 : 0) |
      (registers.P.overflow         ? 1<<6 : 0) |
      (registers.P.negative         ? 1<<7 : 0);
}

status_t flagsToRegister(uint8_t f) {
   status_t P = {
      .carry            = (uint8_t)((f & 1<<0) ? 1 : 0),
      .zero             = (uint8_t)((f & 1<<1) ? 1 : 0),
      .interruptDisable = (uint8_t)((f & 1<<2) ? 1 : 0),
      .decimalMode      = (uint8_t)((f & 1<<3) ? 1 : 0),
      .breakCmd         = (uint8_t)((f & 1<<4) ? 1 : 0),
      .unused           = (uint8_t)((f & 1<<5) ? 1 : 0),
      .overflow         = (uint8_t)((f & 1<<6) ? 1 : 0),
      .negative         = (uint8_t)((f & 1<<7) ? 1 : 0)};
   return P;
}

void push(uint8_t val) {
   store(0x100 | registers.SP--, val);
}

uint8_t pop() {
   return fetch(0x100 | ++registers.SP);
}

int8_t asSigned(uint8_t v) {
   return (int8_t)v;
}

uint8_t MSB(uint16_t v) {
   return (v >> 7) & 1;
}

uint8_t MSB2(uint16_t v) {
   return (v >> 6) & 1;
}

uint8_t Imm(uint8_t *pageBoundary, uint16_t *address) {
   *pageBoundary = 0;
   return fetchPC();
}

uint8_t ZP(uint8_t *pageBoundary, uint16_t *address) {
   *pageBoundary = 0;
   *address = fetchPC();
   return fetch(*address);
}

uint8_t ZPX(uint8_t *pageBoundary, uint16_t *address) {
   *pageBoundary = 0;
   *address = (fetchPC() + registers.X) & 0xFF;
   return fetch(*address);
}

uint8_t ZPY(uint8_t *pageBoundary, uint16_t *address) {
   *pageBoundary = 0;
   *address = (fetchPC() + registers.Y) & 0xFF;
   return fetch(*address);
}

uint8_t Abs(uint8_t *pageBoundary, uint16_t *address) {
   *pageBoundary = 0;
   *address = fetchPC16();
   return fetch(*address);
}

uint8_t AbsX(uint8_t *pageBoundary, uint16_t *address) {
   *pageBoundary = 0;
   *address = fetchPC16() + registers.X;
   return fetch(*address);
}

uint8_t AbsY(uint8_t *pageBoundary, uint16_t *address) {
   *pageBoundary = 0;
   *address = fetchPC16() + registers.Y;
   return fetch(*address);
}

uint8_t IndX(uint8_t *pageBoundary, uint16_t *address) {
   *pageBoundary = 0;
   *address = fetchZP16((fetchPC() + registers.X) & 0xFF);
   return fetch(*address);
}

uint8_t IndY(uint8_t *pageBoundary, uint16_t *address) {
   *pageBoundary = 0;
   *address = fetchZP16(fetchPC()) + registers.Y;
   return fetch(*address);
}

int ADC(uint8_t val, uint16_t addr) {
   uint16_t res = registers.A + val + registers.P.carry;
   
   registers.P.overflow = (MSB(registers.A) != MSB(res) && MSB(val) != MSB(res)) ? 1 : 0;
   registers.P.negative = MSB(res)  ? 1 : 0;
   registers.P.zero     = (res & 0xFF) == 0  ? 1 : 0;
   registers.P.carry    = res > 255 ? 1 : 0;

   registers.A = res & 0xFF;
   return 0;
}

int SBC(uint8_t val, uint16_t addr) {
   return ADC(~val, addr);
   // int16_t re = registers.A - val + (registers.P.carry ? 0x100 : 0);
   // uint16_t res = registers.A - val - (registers.P.carry ? 0 : 1);

   // fprintf(stderr, "%d, %d\n", res, int8_t(re));

   // registers.P.overflow = ((res & 0x100) == 0 && int16_t(res) < -128) ? 1 : 0;
   // registers.P.negative = MSB(res)  ? 1 : 0;
   // registers.P.zero     = (res & 0xFF) == 0  ? 1 : 0;
   // registers.P.carry    = (int8_t(res) >= 0) ? 1 : 0;

   // registers.A = res & 0xFF;
   // return 0;
}

int ASL(uint8_t val, uint16_t addr) {
   registers.P.carry = MSB(val) ? 1 : 0;

   val = (val << 1) & 0xFE;

   registers.P.negative = MSB(val) ? 1 : 0;
   registers.P.zero     = val == 0 ? 1 : 0;

   if (addr) {
      store(addr, val);
   } else {
      registers.A = val;
   }

   return 0;
}

int LSR(uint8_t val, uint16_t addr) {
   fprintf(stderr, "LSR %04X %02X\n", addr, val);
   registers.P.negative = 0;
   registers.P.carry = (val & 1) ? 1 : 0;

   val = (val >> 1) & 0x7F;
   registers.P.zero = val == 0 ? 1 : 0;

   if (addr) {
      store(addr, val);
   } else {
      registers.A = val;
   }

   return 0;
}

int ROL(uint8_t val, uint16_t addr) {
   uint8_t t = MSB(val) ? 1 : 0;

   val = ((val << 1) & 0xFE) | (registers.P.carry ? 1 : 0);

   registers.P.carry    = t;
   registers.P.negative = MSB(val) ? 1 : 0;
   registers.P.zero     = val == 0 ? 1 : 0;

   if (addr) {
      store(addr, val);
   } else {
      registers.A = val;
   }

   return 0;
}

int ROR(uint8_t val, uint16_t addr) {
   uint8_t t = (val & 1) ? 1 : 0;

   val = ((val >> 1) & 0x7F) | (registers.P.carry ? 0x80 : 0x00);

   registers.P.carry    = t;
   registers.P.negative = MSB(val) ? 1 : 0;
   registers.P.zero     = val == 0 ? 1 : 0;

   if (addr) {
      store(addr, val);
   } else {
      registers.A = val;
   }

   return 0;
}

int AND(uint8_t val, uint16_t addr) {
   registers.A &= val;
   registers.P.negative = MSB(registers.A) ? 1 : 0;
   registers.P.zero     = registers.A == 0 ? 1 : 0;
   return 0;
}

int EOR(uint8_t val, uint16_t addr) {
   registers.A ^= val;
   registers.P.negative = MSB(registers.A) ? 1 : 0;
   registers.P.zero     = registers.A == 0 ? 1 : 0;
   return 0;
}

int ORA(uint8_t val, uint16_t addr) {
   registers.A |= val;
   registers.P.negative = MSB(registers.A) ? 1 : 0;
   registers.P.zero     = registers.A == 0 ? 1 : 0;
   return 0;
}

int Branch(uint8_t cond, uint8_t val) {
   if (cond) {
      uint16_t page = registers.PC & 0xFF00;
      registers.PC += asSigned(val);
      return (registers.PC & 0xFF00) == page ? 1 : 2;
   }
   return 0;
}

int BCC(uint8_t val, uint16_t addr) {
   return Branch(registers.P.carry == 0, val);
}

int BCS(uint8_t val, uint16_t addr) {
   return Branch(registers.P.carry == 1, val);
}

int BEQ(uint8_t val, uint16_t addr) {
   return Branch(registers.P.zero == 1, val);
}

int BMI(uint8_t val, uint16_t addr) {
   return Branch(registers.P.negative == 1, val);
}

int BNE(uint8_t val, uint16_t addr) {
   return Branch(registers.P.zero == 0, val);
}

int BPL(uint8_t val, uint16_t addr) {
   return Branch(registers.P.negative == 0, val);
}

int BVC(uint8_t val, uint16_t addr) {
   return Branch(registers.P.overflow == 0, val);
}

int BVS(uint8_t val, uint16_t addr) {
   return Branch(registers.P.overflow == 1, val);
}

int CLC(uint8_t val, uint16_t addr) {
   registers.P.carry = 0;
   return 0;
}

int CLD(uint8_t val, uint16_t addr) {
   registers.P.decimalMode = 0;
   return 0;
}

int CLI(uint8_t val, uint16_t addr) {
   registers.P.interruptDisable = 0;
   return 0;
}

int CLV(uint8_t val, uint16_t addr) {
   registers.P.overflow = 0;
   return 0;
}

int SEC(uint8_t val, uint16_t addr) {
   registers.P.carry = 1;
   return 0;
}

int SED(uint8_t val, uint16_t addr) {
   registers.P.decimalMode = 1;
   return 0;
}

int SEI(uint8_t val, uint16_t addr) {
   registers.P.interruptDisable = 1;
   return 0;
}

int Compare(uint8_t val, uint8_t mem) {
   fprintf(stderr, "%d, %d\n", (val), (mem));
   registers.P.negative = MSB(val - mem) ? 1 : 0;
   registers.P.carry    = val >= mem ? 1 : 0;
   registers.P.zero     = val == mem ? 1 : 0;
   return 0;
}

int CMP(uint8_t val, uint16_t addr) {
   return Compare(registers.A, val);
}

int CPX(uint8_t val, uint16_t addr) {
   return Compare(registers.X, val);
}

int CPY(uint8_t val, uint16_t addr) {
   return Compare(registers.Y, val);
}

int DEC(uint8_t val, uint16_t addr) {
   store(addr, --val);
   registers.P.negative = MSB(val) ? 1 : 0;
   registers.P.zero     = val == 0 ? 1 : 0;
   return 0;
}

int DEX(uint8_t val, uint16_t addr) {
   registers.X--;
   registers.P.negative = MSB(registers.X) ? 1 : 0;
   registers.P.zero     = registers.X == 0 ? 1 : 0;
   return 0;
}

int DEY(uint8_t val, uint16_t addr) {
   registers.Y--;
   registers.P.negative = MSB(registers.Y) ? 1 : 0;
   registers.P.zero     = registers.Y == 0 ? 1 : 0;
   return 0;
}

int INC(uint8_t val, uint16_t addr) {
   store(addr, ++val);
   registers.P.negative = MSB(val) ? 1 : 0;
   registers.P.zero     = val == 0 ? 1 : 0;
   return 0;
}

int INX(uint8_t val, uint16_t addr) {
   registers.X++;
   registers.P.negative = MSB(registers.X) ? 1 : 0;
   registers.P.zero     = registers.X == 0 ? 1 : 0;
   return 0;
}

int INY(uint8_t val, uint16_t addr) {
   registers.Y++;
   registers.P.negative = MSB(registers.Y) ? 1 : 0;
   registers.P.zero     = registers.Y == 0 ? 1 : 0;
   return 0;
}

int JMP_ABS(uint8_t val, uint16_t addr) {
   registers.PC = fetchPC16();
   return 0;
}

int JMP_IND(uint8_t val, uint16_t addr) {
   uint16_t first = fetchPC16();
   registers.PC = fetch(first) | (fetch((first & 0xFF00) | ((first+1) & 0xFF)) << 8);
   return 0;
}

int BRK(uint8_t val, uint16_t addr) {
   push((registers.PC >> 8) & 0xFF);
   push((registers.PC     ) & 0xFF);
   push(registerFlags()     | 0x10);
   registers.PC = fetch16(0xFFFE);
   return 0;
}

int JSR(uint8_t val, uint16_t addr) {
   uint16_t nextPC = fetchPC16();
   registers.PC--;
   push((registers.PC >> 8) & 0xFF);
   push((registers.PC     ) & 0xFF);
   registers.PC = nextPC;
   return 0;
}

int RTI(uint8_t val, uint16_t addr) {
   registers.P  = flagsToRegister(pop() | 0x20);
   registers.PC = pop() | (pop() << 8);
   return 0;
}

int RTS(uint8_t val, uint16_t addr) {
   registers.PC = (pop() | (pop() << 8)) + 1;
   return 0;
}

int LDA(uint8_t val, uint16_t addr) {
   registers.A = val;
   registers.P.negative = MSB(registers.A) ? 1 : 0;
   registers.P.zero     = registers.A == 0 ? 1 : 0;
   return 0;
}

int LDX(uint8_t val, uint16_t addr) {
   registers.X = val;
   registers.P.negative = MSB(registers.X) ? 1 : 0;
   registers.P.zero     = registers.X == 0 ? 1 : 0;
   return 0;
}

int LDX_ZPY(uint8_t val, uint16_t addr) {
   uint8_t pageBoundary = 0;
   return LDX(ZPY(&pageBoundary, &addr), addr);
}

int LDY(uint8_t val, uint16_t addr) {
   registers.Y = val;
   registers.P.negative = MSB(registers.Y) ? 1 : 0;
   registers.P.zero     = registers.Y == 0 ? 1 : 0;
   return 0;
}

int STA(uint8_t val, uint16_t addr) {
   store(addr, registers.A);
   return 0;
}

int STX(uint8_t val, uint16_t addr) {
   store(addr, registers.X);
   return 0;
}

int STX_ZPY(uint8_t val, uint16_t addr) {
   uint8_t pageBoundary = 0;
   return STX(ZPY(&pageBoundary, &addr), addr);
}

int STY(uint8_t val, uint16_t addr) {
   store(addr, registers.Y);
   return 0;
}

int PHA(uint8_t val, uint16_t addr) {
   push(registers.A);
   return 0;
}

int PHP(uint8_t val, uint16_t addr) {
   push(registerFlags() | 0x10);
   return 0;
}

int PLA(uint8_t val, uint16_t addr) {
   registers.A = pop();
   registers.P.negative = MSB(registers.A) ? 1 : 0;
   registers.P.zero     = registers.A == 0 ? 1 : 0;
   return 0;
}

int PLP(uint8_t val, uint16_t addr) {
   registers.P = flagsToRegister((pop() & 0xEF) | 0x20);
   return 0;
}

int TAX(uint8_t val, uint16_t addr) {
   registers.X = registers.A;
   registers.P.negative = MSB(registers.X) ? 1 : 0;
   registers.P.zero     = registers.X == 0 ? 1 : 0;
   return 0;
}

int TAY(uint8_t val, uint16_t addr) {
   registers.Y = registers.A;
   registers.P.negative = MSB(registers.Y) ? 1 : 0;
   registers.P.zero     = registers.Y == 0 ? 1 : 0;
   return 0;
}

int TSX(uint8_t val, uint16_t addr) {
   registers.X = registers.SP;
   registers.P.negative = MSB(registers.X) ? 1 : 0;
   registers.P.zero     = registers.X == 0 ? 1 : 0;
   return 0;
}

int TXA(uint8_t val, uint16_t addr) {
   registers.A = registers.X;
   registers.P.negative = MSB(registers.A) ? 1 : 0;
   registers.P.zero     = registers.A == 0 ? 1 : 0;
   return 0;
}

int TYA(uint8_t val, uint16_t addr) {
   registers.A = registers.Y;
   registers.P.negative = MSB(registers.A) ? 1 : 0;
   registers.P.zero     = registers.A == 0 ? 1 : 0;
   return 0;
}

int TXS(uint8_t val, uint16_t addr) {
   registers.SP = registers.X;
   return 0;
}

int BIT(uint8_t val, uint16_t addr) {
   uint8_t t = registers.A & val;
   registers.P.negative = MSB(val)  ? 1 : 0;
   registers.P.overflow = MSB2(val) ? 1 : 0;
   registers.P.zero     = t == 0    ? 1 : 0;
   return 0;
}

int NOP(uint8_t val, uint16_t addr) {
   return 0;
}

// undocumented

int AAC(uint8_t val, uint16_t addr) {
   return 0;
}

int XAA(uint8_t val, uint16_t addr) {
   return 0;
}

int AAX(uint8_t val, uint16_t addr) {
   return 0;
}

int AAX_ZPY(uint8_t val, uint16_t addr) {
   return 0;
}

int LAX(uint8_t val, uint16_t addr) {
   return 0;
}

int LAX_ZPY(uint8_t val, uint16_t addr) {
   return 0;
}

int ARR(uint8_t val, uint16_t addr) {
   return 0;
}

int ASR(uint8_t val, uint16_t addr) {
   return 0;
}

int ATX(uint8_t val, uint16_t addr) {
   return 0;
}

int AXS(uint8_t val, uint16_t addr) {
   return 0;
}

int AXA(uint8_t val, uint16_t addr) {
   return 0;
}

int SXA(uint8_t val, uint16_t addr) {
   return 0;
}

int SYA(uint8_t val, uint16_t addr) {
   return 0;
}

int DCP(uint8_t val, uint16_t addr) {
   return 0;
}

int ISC(uint8_t val, uint16_t addr) {
   return 0;
}

int RLA(uint8_t val, uint16_t addr) {
   return 0;
}

int RRA(uint8_t val, uint16_t addr) {
   return 0;
}

int SLO(uint8_t val, uint16_t addr) {
   return 0;
}

int SRE(uint8_t val, uint16_t addr) {
   return 0;
}

int LAR(uint8_t val, uint16_t addr) {
   return 0;
}

int XAS(uint8_t val, uint16_t addr) {
   return 0;
}

int DOP(uint8_t val, uint16_t addr) {
   return 0;
}

int TOP(uint8_t val, uint16_t addr) {
   return 0;
}

int KIL(uint8_t val, uint16_t addr) {
   exit(0);
}
