#include "nemo.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


#define EXIT_ADDR "__EXIT__"

static void cg_start();
static bool cg_asm_for_func(struct ast_node func);
static void cg_asm_for_var_decl(struct ast_node func, int *stack_offset);
static void cg_asm_for_assignment(struct ast_node assign);
static void cg_asm_for_exit(struct ast_node exit);

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

        if (is_main) main_found = is_main;
        break;
      default: {
        fprintf(stderr, "error: UNREACHABLE: UNKNOWN NODE FOR CG\n");
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
  printf(EXIT_ADDR ":\n");

  printf("\tmov  %%rax, %%rdi\n");
  printf("\tmov  $60, %%rax\n");
  printf("\tsyscall\n");
}


static bool cg_asm_for_func(struct ast_node func) {
  struct symbol* sym = func.ref_in_parent->data;

  printf("%s:\n", sym->name);
  printf("\tpush %%rbp\n");
  printf("\tmov  %%rsp, %%rbp\n");
  
  struct ast_node scope = func.children[0];
  int stack_offset = 0;
  for (int i = 0; i < scope.children_len; i++) {
    struct ast_node cur = scope.children[i];
    switch (cur.type) {
      case AST_VAR_DECL:
        cg_asm_for_var_decl(cur, &stack_offset);
        break;
      case AST_ASSIGNMENT:
        cg_asm_for_assignment(cur);
        break;
      case AST_EXIT:
        cg_asm_for_exit(cur);
        break;
      default:
        fprintf(stderr, "error: UNREACHABLE func scope cg\n");
        exit(1);
    }
  }

  printf("\tpop  %%rbp\n");
  if (sym->is_main_func) {
    // always return 0 for now
    printf("\txor  %%rax, %%rax\n");
  }
  printf("\tret\n");
  return sym->is_main_func;
}


static void cg_asm_for_var_decl(struct ast_node var, int *stack_offset)
{
  struct symbol* sym = var.ref_in_parent->data;
  assert(sym != NULL);

  struct data_type_definition *dt = sym->data_type;
  assert(dt != NULL); 

  // TODO: handle alignment
  *stack_offset -= dt->size;
  sym->stack_offset = *stack_offset;

  printf("\tadd  $%d, %%rbp\n", *stack_offset);
  printf("\tmovl $0,  %d(%%rbp)\n", *stack_offset);
}


static void cg_asm_for_assignment(struct ast_node assign)
{
  assert(assign.children_len == 2);

  struct ast_node lhs = assign.children[0];
  struct ast_node rhs = assign.children[1];

  int stack_offset = ((struct symbol*)lhs.ref_in_parent->data)->stack_offset;

  printf("\tmovl $%d, %d(%%rbp)\n", rhs.literal, stack_offset);
}


static void cg_asm_for_exit(struct ast_node exit)
{
  struct ast_node rhs = exit.children[0];

	printf("\tmov  $%d, %rax\n", rhs.literal);
	printf("\tjmp  " EXIT_ADDR "\n");
}
