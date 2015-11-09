#include "memory.h"

static uint8_t memory[65536];

void initMemory(void *rom) {

}

uint8_t fetch(uint16_t addr) {

}

uint16_t fetch16(uint16_t addr) {
   return fetch(addr) | (fetch(addr+1) << 8);
}


void store(uint16_t addr, uint8_t value) {

}
