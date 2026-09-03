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
static void cg_asm_for_expr(struct ast_node expr);
static void cg_asm_for_add(struct ast_node add);

static void asm_load_operand_to_reg(struct ast_node op, char* reg);

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
  struct symbol* sym = func.sym_ref;

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
  struct symbol* sym = var.sym_ref;
  assert(sym);

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
  int lhs_stack_offset = lhs.sym_ref->stack_offset;

  struct ast_node rhs = assign.children[1];
  switch (rhs.type) {
    case AST_LITERAL: printf("\tmovl  $%d, %d(%%rbp)\n", rhs.literal, lhs_stack_offset); break;
    case AST_SYMBOL:
      int rhs_stack_offset = rhs.sym_ref->stack_offset;
      printf("\tmovl  %d(%%rbp), %%r8d\n", rhs_stack_offset);
      printf("\tmovl  %%r8d, %d(%%rbp)\n", lhs_stack_offset);
      break;
    case AST_EXPR:
      cg_asm_for_expr(rhs);
      printf("\tmovl  %%edx, %d(%%rbp)\n", lhs_stack_offset);
      break;
  }
}


static void cg_asm_for_exit(struct ast_node exit)
{
  struct ast_node rhs = exit.children[0];

  switch (rhs.type) {
    case AST_LITERAL: printf("\tmov  $%d, %rax\n", rhs.literal); break;
    case AST_SYMBOL:
      int stack_offset = rhs.sym_ref->stack_offset;
      printf("\tmovl  %d(%%rbp), %%eax\n", stack_offset);
      break;
    case AST_EXPR:
      cg_asm_for_expr(rhs);
      printf("\tmovl  %%edx, %%eax\n");
      break;
  }

	printf("\tjmp  " EXIT_ADDR "\n");
}


static void cg_asm_for_expr(struct ast_node expr)
{
  assert(expr.type == AST_EXPR);

  struct ast_node exp = expr.children[0];
  if (exp.type == AST_MATH_ADD) cg_asm_for_add(exp);     
  else                          asm_load_operand_to_reg(exp, "eax");
}


static void cg_asm_for_add(struct ast_node add)
{
  asm_load_operand_to_reg(add.children[0], "edx");
  for (int i = 1; i < add.children_len; i++) {
    asm_load_operand_to_reg(add.children[i], "eax");
    if (add.children[i].prev_token == TOKEN_HYPHEN) {
      printf("\tsubl %%eax, %%edx\n");      
    } else {
      printf("\taddl %%eax, %%edx\n");      
    }
  }
}


static void asm_load_operand_to_reg(struct ast_node op, char* reg)
{
  switch (op.type) {
    case AST_LITERAL:
      printf("\tmovl  $%d, %%%s\n", op.literal, reg);
      break;

    case AST_SYMBOL:
      int stack_offset = op.sym_ref->stack_offset;
      printf("\tmovl  %d(%%rbp), %%%s\n", stack_offset, reg);
      break;
    default:
      fprintf(stderr, "error: UNREACHABLE: EXPECTED OPERAND: found %d\n", op.type);
      exit(1);
  }  
}
