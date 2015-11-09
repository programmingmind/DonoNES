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
   int (*execute)(uint8_t);
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

int ADC(uint8_t val);
int AND(uint8_t val);
int ASL(uint8_t val);
int BCC(uint8_t val);
int BCS(uint8_t val);
int BEQ(uint8_t val);
int BMI(uint8_t val);
int BNE(uint8_t val);
int BPL(uint8_t val);
int BIT(uint8_t val);

static instruction_t InstructionTable[] = {
   //       Impl, Rel
   //        Acc, Imm,  ZP,   ZPX,  Abs,  AbsX, AbsY, IndX, IndY
   {"ADC", {0xFF, 0x69, 0x65, 0x75, 0x6D, 0x7D, 0x79, 0x61, 0x71}, {0, 2, 3, 4, 4, 4, 4, 6, 5}, {0, 0, 0, 0, 0, 1, 1, 0, 1}, ADC},
   {"AND", {0xFF, 0x29, 0x25, 0x35, 0x2D, 0x3D, 0x39, 0x21, 0x31}, {0, 2, 2, 3, 4, 4, 4, 6, 5}, {0, 0, 0, 0, 0, 1, 1, 0, 1}, AND},
   {"ASL", {0x0A, 0xFF, 0x06, 0x16, 0x0E, 0x1E, 0xFF, 0xFF, 0xFF}, {2, 0, 5, 6, 6, 7, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, ASL},

   {"BCC", {0xFF, 0x90, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0 ,0 ,0 ,0 ,0}, {0, 0, 0, 0, 0 ,0 ,0 ,0 ,0}, BCC},
   {"BCS", {0xFF, 0xB0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0 ,0 ,0 ,0 ,0}, {0, 0, 0, 0, 0 ,0 ,0 ,0 ,0}, BCS},
   {"BEQ", {0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0 ,0 ,0 ,0 ,0}, {0, 0, 0, 0, 0 ,0 ,0 ,0 ,0}, BEQ},
   {"BMI", {0xFF, 0x30, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0 ,0 ,0 ,0 ,0}, {0, 0, 0, 0, 0 ,0 ,0 ,0 ,0}, BMI},
   {"BNE", {0xFF, 0xD0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0 ,0 ,0 ,0 ,0}, {0, 0, 0, 0, 0 ,0 ,0 ,0 ,0}, BNE},
   {"BPL", {0xFF, 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 2, 0, 0, 0 ,0 ,0 ,0 ,0}, {0, 0, 0, 0, 0 ,0 ,0 ,0 ,0}, BPL},

   {"BIT", {0xFF, 0xFF, 0x24, 0xFF, 0x2C, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 3, 0, 4 ,0 ,0 ,0 ,0}, {0, 0, 0, 0, 0 ,0 ,0 ,0 ,0}, BIT},

   {"???", {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0, 0 ,0 ,0 ,0 ,0}, {0, 0, 0, 0, 0 ,0 ,0 ,0 ,0}, NULL}
};

uint8_t fetchPC();
uint16_t fetchPC16();

uint8_t Imm (uint8_t *pageBoundary);
uint8_t ZP  (uint8_t *pageBoundary);
uint8_t ZPX (uint8_t *pageBoundary);
uint8_t Abs (uint8_t *pageBoundary);
uint8_t AbsX(uint8_t *pageBoundary);
uint8_t AbsY(uint8_t *pageBoundary);
uint8_t IndX(uint8_t *pageBoundary);
uint8_t IndY(uint8_t *pageBoundary);

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
   uint8_t opcode = fetchPC();

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

   switch (ndx) {
      case 0:
         val = pageBoundary = 0;
         break;
      case 1:
         val = Imm(&pageBoundary);
         break;
      case 2:
         val = ZP(&pageBoundary);
         break;
      case 3:
         val = ZPX(&pageBoundary);
         break;
      case 4:
         val = Abs(&pageBoundary);
         break;
      case 5:
         val = AbsX(&pageBoundary);
         break;
      case 6:
         val = AbsY(&pageBoundary);
         break;
      case 7:
         val = IndX(&pageBoundary);
         break;
      case 8:
         val = IndY(&pageBoundary);
         break;
      default:
         fprintf(stderr, "Wrong ndx %d\n", ndx);
         return 1;
   }

   return inst->execute(val) + inst->cycles[ndx] + (pageBoundary ? inst->extraCycles[ndx] : 0);
}

uint8_t fetchPC() {
   return fetch(registers.PC++);
}

uint16_t fetchPC16() {
   uint16_t ret = fetch16(registers.PC);
   registers.PC += 2;
   return ret;
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

uint8_t Imm(uint8_t *pageBoundary) {
   *pageBoundary = 0;
   return fetchPC();
}

uint8_t ZP(uint8_t *pageBoundary) {
   *pageBoundary = 0;
   return fetch(fetchPC());
}

uint8_t ZPX(uint8_t *pageBoundary) {
   *pageBoundary = 0;
   return fetch((fetchPC() + registers.X) & 0xFF);
}

uint8_t Abs(uint8_t *pageBoundary) {
   *pageBoundary = 0;
   return fetch(fetchPC16());
}

uint8_t AbsX(uint8_t *pageBoundary) {
   *pageBoundary = 0;
   return fetch(fetchPC16() + registers.X);
}

uint8_t AbsY(uint8_t *pageBoundary) {
   *pageBoundary = 0;
   return fetch(fetchPC16() + registers.Y);
}

uint8_t IndX(uint8_t *pageBoundary) {
   *pageBoundary = 0;
   return fetch(fetch16((fetchPC() + registers.X) & 0xFF));
}

uint8_t IndY(uint8_t *pageBoundary) {
   *pageBoundary = 0;
   return fetch((fetch16(fetchPC()) + registers.Y));
}

int ADC(uint8_t val) {
   uint16_t res = registers.A + val + registers.P.carry;
   
   registers.P.overflow = MSB(registers.A) != MSB(res) ? 1 : 0;
   registers.P.negative = MSB(res)  ? 1 : 0;
   registers.P.zero     = res == 0  ? 1 : 0;
   registers.P.carry    = res > 255 ? 1 : 0;

   registers.A = res & 0xFF;
   return 0;
}

int AND(uint8_t val) {
   registers.A &= val;
   registers.P.negative = MSB(registers.A)  ? 1 : 0;
   registers.P.zero     = registers.A == 0  ? 1 : 0;
   return 0;
}

int ASL(uint8_t val) {
   registers.P.carry = MSB(val) ? 1 : 0;

   registers.A = (val << 1) & 0xFE;

   registers.P.negative = MSB(registers.A)  ? 1 : 0;
   registers.P.zero     = registers.A == 0  ? 1 : 0;

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

int BCC(uint8_t val) {
   return Branch(registers.P.carry == 0, val);
}

int BCS(uint8_t val) {
   return Branch(registers.P.carry == 1, val);
}

int BEQ(uint8_t val) {
   return Branch(registers.P.zero == 1, val);
}

int BMI(uint8_t val) {
   return Branch(registers.P.negative == 1, val);
}

int BNE(uint8_t val) {
   return Branch(registers.P.zero == 0, val);
}

int BPL(uint8_t val) {
   return Branch(registers.P.negative == 0, val);
}

int BIT(uint8_t val) {
   uint8_t t = registers.A & val;
   registers.P.negative = MSB(t)  ? 1 : 0;
   registers.P.overflow = MSB2(t) ? 1 : 0;
   registers.P.zero     = t == 0  ? 1 : 0;
   return 0;
}
