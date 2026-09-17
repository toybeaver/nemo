// Truth be told: this whole file is a mess, no register, alignment, sizing, types, etc control
// whatsoever, plus the very unoptmizied generated code. This will stay fucked up until I get
// to the point to write a proper codegen file per-os setup. After math and bool expressions,
// control-flow logic, and basic interfacing with the OS is properly implemented in the
// language.

#include "nemo.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


#define EXIT_ADDR "__EXIT__"

static void cg_start();
static bool cg_asm_for_func(struct ast_node func, int *label_count);
static void cg_asm_for_scope(struct ast_node scope, int *stack_offset, int *label_count);
static void cg_asm_for_var_decl(struct ast_node func, int *stack_offset);
static void cg_asm_for_assignment(struct ast_node assign);
static void cg_asm_for_exit(struct ast_node exit);
static void cg_asm_for_expr(struct ast_node expr);
static void cg_asm_for_if(struct ast_node if_exp, int *stack_offset, int *label_count);

static void cg_asm_for_expr_math(struct ast_node expr);
static void cg_asm_for_math_add(struct ast_node add);
static void cg_asm_for_math_mul(struct ast_node mul);

static void cg_asm_for_expr_bool(struct ast_node expr);

static void asm_load_operand_to_reg(struct ast_node op, char* reg);

void gen_asm_to_stdout(struct ast_node ast)
{
  cg_start();

  bool main_found = false;
  int label_count = 0;

  for (int i = 0; i < ast.children_len; i++) {
    switch(ast.children[i].type) {
      case AST_FUNCTION:
        bool is_main = cg_asm_for_func(ast.children[i], &label_count);
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


static bool cg_asm_for_func(struct ast_node func, int *label_count)
{
  struct symbol* sym = func.sym_ref;

  printf("%s:\n", sym->name);
  printf("\tpush %%rbp \t\t# INIT STACK\n");
  printf("\tmov  %%rsp, %%rbp\n\n");
  
  struct ast_node scope = func.children[0];
  int stack_offset = 0;
  cg_asm_for_scope(scope, &stack_offset, label_count);

  printf("\tpop  %%rbp\n");
  if (sym->is_main_func) {
    // always return 0 for now
    printf("\txor  %%rax, %%rax\n");
  }
  printf("\tret\n");
  return sym->is_main_func;
}

static void cg_asm_for_scope(struct ast_node scope, int *stack_offset, int *label_count)
{
  for (int i = 0; i < scope.children_len; i++) {
    struct ast_node cur = scope.children[i];
    switch (cur.type) {
      case AST_VAR_DECL:
        cg_asm_for_var_decl(cur, stack_offset);
        break;
      case AST_ASSIGNMENT:
        cg_asm_for_assignment(cur);
        break;
      case AST_IF:
        cg_asm_for_if(cur, stack_offset, label_count);
        break;
      case AST_EXIT:
        cg_asm_for_exit(cur);
        break;
      default:
        fprintf(stderr, "error: UNREACHABLE func scope cg\n");
        exit(1);
    }
  }
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

  printf("\tadd  $%d, %%rbp \t\t# VAR DECL \"%s\"\n", *stack_offset, sym->name);
  printf("\tmovl $0,  %d(%%rbp)\n\n", *stack_offset);
}


static void cg_asm_for_assignment(struct ast_node assign)
{
  assert(assign.children_len == 2);

  struct ast_node lhs = assign.children[0];
  int lhs_stack_offset = lhs.sym_ref->stack_offset;

  struct ast_node rhs = assign.children[1];
  cg_asm_for_expr(rhs);

  if (rhs.children[0].type == AST_EXPR_BOOL) {
    printf("\tmovb  %%cl, %d(%%rbp) \t\t# VAR EXP ASSIGN \"%s\"\n\n", lhs_stack_offset, lhs.sym_ref->name);    
  } else {
    printf("\tmovl  %%ecx, %d(%%rbp) \t\t# VAR EXP ASSIGN \"%s\"\n\n", lhs_stack_offset, lhs.sym_ref->name);
  }
}


static void cg_asm_for_exit(struct ast_node exit)
{
  struct ast_node rhs = exit.children[0];

 
  cg_asm_for_expr(rhs);
  printf("\tmovl  %%ecx, %%eax \t\t# EXIT CALL \n");
	printf("\tjmp  " EXIT_ADDR "\n\n");
}


static void cg_asm_for_expr(struct ast_node expr)
{
  assert(expr.type == AST_EXPR);

  struct ast_node exp = expr.children[0];
  if      (exp.type == AST_EXPR_MATH) cg_asm_for_expr_math(exp);     
  else if (exp.type == AST_EXPR_BOOL) cg_asm_for_expr_bool(exp);
}


static void cg_asm_for_expr_math(struct ast_node expr)
{
  assert(expr.type == AST_EXPR_MATH);

  struct ast_node exp = expr.children[0];
  if (exp.type == AST_MATH_ADD) cg_asm_for_math_add(exp);     
  else                          asm_load_operand_to_reg(exp, "ecx");
}


static void cg_asm_for_math_add(struct ast_node add)
{
  cg_asm_for_math_mul(add.children[0]);
  printf("\tmovl  %%edx, %%ecx\n");
  for (int i = 1; i < add.children_len; i++) {
    cg_asm_for_math_mul(add.children[i]);
    if (add.children[i].prev_token == TOKEN_HYPHEN) {
      printf("\tsubl %%edx, %%ecx\n");      
    } else {
      printf("\taddl %%edx, %%ecx\n");      
    }
  }
}


static void cg_asm_for_math_mul(struct ast_node mul)
{
  asm_load_operand_to_reg(mul.children[0], "edx");
  for (int i = 1; i < mul.children_len; i++) {
    if (mul.children[i].prev_token == TOKEN_SLASH) {
      asm_load_operand_to_reg(mul.children[i], "ebx");
      printf("\tmovl %%edx, %%eax\n");
      printf("\tcltd\n");
      printf("\tidivl %%ebx\n");
      printf("\tmovl %%eax, %%edx\n");
    } else {
      asm_load_operand_to_reg(mul.children[i], "eax");
      printf("\timull %%eax, %%edx\n");
      
    }
  }
}


static void cg_asm_for_expr_bool(struct ast_node expr)
{
  assert(expr.type == AST_EXPR_BOOL);

  struct ast_node exp = expr.children[0];
  printf("\txor %%ecx, %%ecx\n");
  asm_load_operand_to_reg(exp, "cl");
}


static void asm_load_operand_to_reg(struct ast_node op, char* reg)
{
  switch (op.type) {
    case AST_LITERAL:
      if (op.literal_type == DT_BOOL) {
        printf("\tmovb $%s, %%%s\n", op.literal._bool == true ? "0xFF" : "0x00", reg);
      } else {
        printf("\tmovl  $%d, %%%s\n", op.literal, reg);
      }
      break;

    case AST_SYMBOL:
      int stack_offset = op.sym_ref->stack_offset;
      switch (op.sym_ref->data_type->type) {
      case DT_BOOL:
        printf("\tmovb  %d(%%rbp), %%%s \t\t# LOAD VAR \"%s\"\n", stack_offset, reg, op.sym_ref->name);
        break;
      case DT_INT32:
        printf("\tmovl  %d(%%rbp), %%%s \t\t# LOAD VAR \"%s\"\n", stack_offset, reg, op.sym_ref->name);
        break;
      }

      break;
    default:
      fprintf(stderr, "error: UNREACHABLE: EXPECTED OPERAND: found %d\n", op.type);
      exit(1);
  }  
}


static void cg_asm_for_if(struct ast_node if_exp, int *stack_offset, int *label_count)
{
  assert(if_exp.type == AST_IF);
  *label_count += 1;
  int lab_id = *label_count;

  struct ast_node condition = if_exp.children[0];  
  cg_asm_for_expr_bool(condition);

  printf("\tcmpb $0x00, %%cl \t\t # IF CONDITION\n");
  printf("\tjz LAB%d\n", lab_id);

  struct ast_node body = if_exp.children[1]; 
  cg_asm_for_scope(body, stack_offset, label_count);
  printf("LAB%d:\n", lab_id);
}
