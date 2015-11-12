#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "memory.h"

void loadFile(const char *fileName, void **contents, int *fileSize);

int main(int argc, char *argv[]) {
   if (argc < 2) {
      fprintf(stderr, "Usage: %s rom.nes\n", argv[0]);
      return 1;
   }

   void *rom = NULL;
   int fileSize = 0;

   loadFile(argv[1], &rom, &fileSize);

   initMemory(rom, fileSize);
   initCPU();

   free(rom);

   while (1) {
      step();
      getchar();
   }

   cleanMemory();
   cleanCPU();

   return 0;
}

void loadFile(const char *fileName, void **contents, int *fileSize) {
   FILE *fp = NULL;

   if ((fp = fopen(fileName, "rb"))) {
      fseek(fp, 0, SEEK_END);
      *fileSize = ftell(fp);
      fseek(fp, 0, SEEK_SET);

      if ((*contents = malloc(*fileSize))) {
         if (fread(*contents, 1, *fileSize, fp) != *fileSize) {
            fprintf(stderr, "File read error\n");
            exit(1);
         }
         fclose(fp);
      } else {
         fprintf(stderr, "Could not allocate memory\n");
         exit(1);
      }
   } else {
      fprintf(stderr, "Could not load file %s\n", fileName);
      exit(1);
   }
}

