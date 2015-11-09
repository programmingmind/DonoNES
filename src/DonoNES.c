#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "memory.h"

void *loadFile(const char *fileName);

int main(int argc, char *argv[]) {
   if (argc < 2) {
      fprintf(stderr, "Usage: %s rom.nes\n", argv[0]);
      return 1;
   }

   void *rom = loadFile(argv[1]);

   initMemory(rom);
   initCPU();

   free(rom);

   while (1) {
      step();
   }

   cleanMemory();
   cleanCPU();

   return 0;
}

void *loadFile(const char *fileName) {
   FILE *fp = NULL;
   void *contents = NULL;

   if ((fp = fopen(fileName, "rb"))) {
      fseek(fp, 0, SEEK_END);
      long fileSize = ftell(fp);
      fseek(fp, 0, SEEK_SET);

      if ((contents = malloc(fileSize))) {
         if (fread(contents, 1, fileSize, fp) != fileSize) {
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

   return contents;
}

