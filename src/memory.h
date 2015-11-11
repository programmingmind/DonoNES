#ifndef MEMORY_H
#define MEMORY_H

#include <inttypes.h>

void initMemory(void *rom, int fileSize);

void cleanMemory();

uint8_t fetch(uint16_t addr);

uint16_t fetch16(uint16_t addr);

void store(uint16_t addr, uint8_t value);

#endif
