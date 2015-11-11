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

static instruction_t InstructionTable[] = {
   //       Other
   //       Impl, Rel
   //        Acc, Imm,  ZP,   ZPX,  Abs,  AbsX, AbsY, IndX, IndY
   {"ADC", {0xFF, 0x69, 0x65, 0x75, 0x6D, 0x7D, 0x79, 0x61, 0x71}, {0, 2, 3, 4, 4, 4, 4, 6, 5}, {0, 0, 0, 0, 0, 1, 1, 0, 1}, ADC},
   {"SBC", {0xFF, 0xE9, 0xE5, 0xF5, 0xED, 0xFD, 0xF9, 0xE1, 0xF1}, {0, 2, 3, 4, 4, 4, 4, 6, 5}, {0, 0, 0, 0, 0, 1, 1, 0, 1}, ADC},

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

   {"???", {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, NULL}
};

uint8_t fetchPC();
uint16_t fetchPC16();

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
   registers.P  = {0,0,0,0,0,0,0,0};
   registers.SP = 0xFF;
   registers.PC = 0xC000;
}

void cleanCPU() {

}

int step() {
   uint16_t pc = registers.PC;
   uint8_t opcode = fetchPC();
   printf("%d: 0x%02x\n", pc, opcode);

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
   }

   fprintf(stderr, "Invalid opcode 0x%02x\n", opcode);
   return 1;
}

int runInstruction(instruction_t *inst, int ndx) {
   uint8_t val;
   uint8_t pageBoundary;
   uint16_t address = 0;

   switch (ndx) {
      case 0:
         val = pageBoundary = 0;
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
   *address = fetch16((fetchPC() + registers.X) & 0xFF);
   return fetch(*address);
}

uint8_t IndY(uint8_t *pageBoundary, uint16_t *address) {
   *pageBoundary = 0;
   *address = fetch16(fetchPC()) + registers.Y;
   return fetch(*address);
}

int ADC(uint8_t val, uint16_t addr) {
   uint16_t res = registers.A + val + registers.P.carry;
   
   registers.P.overflow = MSB(registers.A) != MSB(res) ? 1 : 0;
   registers.P.negative = MSB(res)  ? 1 : 0;
   registers.P.zero     = res == 0  ? 1 : 0;
   registers.P.carry    = res > 255 ? 1 : 0;

   registers.A = res & 0xFF;
   return 0;
}

int SBC(uint8_t val, uint16_t addr) {
   int16_t res = registers.A + ~val + registers.P.carry;

   registers.P.overflow = MSB(registers.A) != MSB(res) ? 1 : 0;
   registers.P.negative = MSB(res)  ? 1 : 0;
   registers.P.zero     = res == 0  ? 1 : 0;
   registers.P.carry    = (res > 127 || res < -128) ? 1 : 0;

   registers.A = res & 0xFF;
   return 0;
}

int ASL(uint8_t val, uint16_t addr) {
   registers.P.carry = MSB(val) ? 1 : 0;

   registers.A = (val << 1) & 0xFE;

   registers.P.negative = MSB(registers.A) ? 1 : 0;
   registers.P.zero     = registers.A == 0 ? 1 : 0;

   return 0;
}

int LSR(uint8_t val, uint16_t addr) {
   registers.P.negative = 0;
   registers.P.carry = (val & 1) ? 1 : 0;

   registers.A = (val >> 1) & 0x7F;

   registers.P.zero = registers.A == 0 ? 1 : 0;
   return 0;
}

int ROL(uint8_t val, uint16_t addr) {
   uint8_t t = MSB(val) ? 1 : 0;

   registers.A = ((val << 1) & 0xFE) | (registers.P.carry ? 1 : 0);

   registers.P.carry    = t                ? 1 : 0;
   registers.P.negative = MSB(registers.A) ? 1 : 0;
   registers.P.zero     = registers.A == 0 ? 1 : 0;

   return 0;
}

int ROR(uint8_t val, uint16_t addr) {
   uint8_t t = (val & 1) ? 1 : 0;

   registers.A = ((val >> 1) & 0x7F) | (registers.P.carry ? 0x80 : 0x00);

   registers.P.carry    = t                ? 1 : 0;
   registers.P.negative = MSB(registers.A) ? 1 : 0;
   registers.P.zero     = registers.A == 0 ? 1 : 0;

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
   return 0;
}

int SEI(uint8_t val, uint16_t addr) {
   registers.P.interruptDisable = 1;
   return 0;
}

int Compare(uint8_t val, uint8_t mem) {
   registers.P.negative = val <= mem ? 1 : 0;
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
   registers.PC = fetch16(fetchPC16());
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
   registers.PC--;
   push((registers.PC >> 8) & 0xFF);
   push((registers.PC     ) & 0xFF);
   registers.PC = fetchPC16();
   return 0;
}

int RTI(uint8_t val, uint16_t addr) {
   registers.P  = flagsToRegister(pop());
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
   push(registerFlags());
   return 0;
}

int PLA(uint8_t val, uint16_t addr) {
   registers.A = pop();
   registers.P.negative = MSB(registers.A) ? 1 : 0;
   registers.P.zero     = registers.A == 0 ? 1 : 0;
   return 0;
}

int PLP(uint8_t val, uint16_t addr) {
   registers.P = flagsToRegister(pop());
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
   registers.P.negative = MSB(t)  ? 1 : 0;
   registers.P.overflow = MSB2(t) ? 1 : 0;
   registers.P.zero     = t == 0  ? 1 : 0;
   return 0;
}

int NOP(uint8_t val, uint16_t addr) {
   return 0;
}
