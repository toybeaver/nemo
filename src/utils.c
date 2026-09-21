#include <nemo.h>
#include <stdio.h>
#include <stdlib.h>


char* read_file(const char* file_path)
{
  FILE *f = fopen(file_path, "r");
  if (f == NULL) {
    fprintf(stderr, "error: File \"%s\" not found\n", file_path);
    exit(1);
  }

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  char* src = malloc(size+1);
  fread(src, sizeof(char), size, f);
  src[size] = '\0';

  fclose(f);
  return src;
 }

