#include "symp.h"
#include <stdio.h>
#include <stdlib.h>


void cg_start()
{
  printf("\t.global _start\n");
  printf("\t.text\n");
  printf("_start:\n");
  printf("\tpush %%rbp\n");
  printf("\tmov  %%rsp, %%rbp\n");
}


void cg_exit(int exit_code)
{
  printf("\tpop %%rbp\n");
  printf("\tmov $60,  %%rax\n");
  printf("\tmov $%d, %%rdi\n", exit_code);
  printf("\tsyscall\n");
} 

