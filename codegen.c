#include "symp.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


static void cg_start();
static bool cg_asm_for_func(struct ast_node func);

void gen_asm_to_stdout(struct ast_node ast)
{
  cg_start();

  bool main_found = false;

  for (int i = 0; i < ast.children_len; i++) {
    switch(ast.children[i].type) {
      case AST_FUNCTION:
        bool is_main = cg_asm_for_func(ast.children[i]);
        if (is_main && main_found) {
          fprintf(stderr, "error: There could be only one main but two or more found\n");
          exit(1);
        }
        main_found = is_main;
        break;
      default: {
        fprintf(stderr, "error: UNREACHABLE\n");
        exit(1);
      }
    }
  }

  if (!main_found) {
    fprintf(stderr, "error: main function not found.\n");
    fprintf(stderr, "error: you need to have one \"func main(){}\" somewhere in your code.\n");
    exit(1);
  }
}


static void cg_start()
{
  printf("\t.global _start\n");
  printf("\t.text\n");
  printf("_start:\n");
  printf("\txor  %rbp, %rbp\n");
  printf("\tand  $0xfffffffffffffff0, %%rsp\n");

  printf("\tcall main\n");

  printf("\tmov  %%rax, %%rdi\n");
  printf("\tmov  $60, %%rax\n");
  printf("\tsyscall\n");
}


static bool cg_asm_for_func(struct ast_node func) {
  struct symbol* identifier = (void*)func.ref_in_parent->data;

  printf("%s:\n", identifier->name);
  printf("\tpush %%rbp\n");
  printf("\tmov  %%rsp, %%rbp\n");
  
  // TODO: parse body and args here

  printf("\tpop %%rbp\n");
  if (identifier->is_main_func) {
    // always return 0 for now
    printf("\txor  %%rax, %%rax\n");
  }
  printf("\tret\n");
  return identifier->is_main_func;
}

