#include <stdio.h>
#include <string.h>

#include "memory.h"

static uint8_t memory[65536];

// $0000 $800  2KB of work RAM
// $0800 $800  Mirror of $000-$7FF
// $1000 $800  Mirror of $000-$7FF
// $1800 $800  Mirror of $000-$7FF
// $2000 8  PPU Ctrl Registers
// $2008 $1FF8 *Mirror of $2000-$2007
// $4000 $20   Registers (Mostly APU)
// $4020 $1FDF Cartridge Expansion ROM
// $6000 $2000 SRAM
// $8000 $4000 PRG-ROM
// $C000 $4000 PRG-ROM

void initMemory(void *rom, int fileSize) {
   memcpy(memory + 0xC000, ((uint8_t*)rom)+16, 16*1024);
   // memcpy(memory + 0x8000, rom, fileSize);
}

uint8_t fetch(uint16_t addr) {
   fprintf(stderr, "Fetching 0x%04X\n", addr);
   if (addr < 0x2000) {
      return memory[addr & 0x7FF];
   } else if (addr < 0x4000) {
      // ppu register[addr & 0x7];
      return addr & 0x7;
   } else if (addr < 0x4020) {
      // registers
      return addr - 0x4000;
   } else {
      return memory[addr];
   }
}

uint16_t fetchZP16(uint16_t addr) {
   fprintf(stderr, "Fetching 0x%04X\n", addr);
   return fetch(addr & 0x00FF) | (fetch((addr+1) & 0x00FF) << 8);
}

uint16_t fetch16(uint16_t addr) {
   fprintf(stderr, "Fetching 0x%04X\n", addr);
   return fetch(addr) | (fetch((addr+1)) << 8);
}

void store(uint16_t addr, uint8_t value) {
   fprintf(stderr, "Storing 0x%02X into 0x%04X\n", value, addr);
   if (addr < 0x2000) {
      memory[addr & 0x7FF] = value;
   } else if (addr < 0x4000) {
      // ppu register[addr & 0x7];
   } else if (addr < 0x4020) {
      // registers
   } else {
      memory[addr] = value;
   }
}
