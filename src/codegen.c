// Truth be told: this whole file is a mess, no register, alignment, sizing, types, etc control
// whatsoever, plus the very unoptmizied generated code. This will stay fucked up until I get
// to the point to write a proper codegen file per-os setup. After math and bool expressions,
// control-flow logic, and basic interfacing with the OS is properly implemented in the
// language.

#include <nemo.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


#define EXIT_ADDR "__EXIT__"

static void cg_start(FILE* f);

static bool cg_asm_for_func(FILE* f, struct ast_node func, int *label_count);
static void cg_asm_for_scope(FILE* f, struct ast_node scope, int *stack_offset, int *label_count);
static void cg_asm_for_var_decl(FILE* f, struct ast_node func, int *stack_offset);
static void cg_asm_for_assignment(FILE* f, struct ast_node assign);
static void cg_asm_for_exit(FILE* f, struct ast_node exit);
static void cg_asm_for_expr(FILE* f, struct ast_node expr);
static void cg_asm_for_if(FILE* f, struct ast_node if_exp, int *stack_offset, int *label_count);

static void cg_asm_for_expr_math(FILE* f, struct ast_node expr);
static void cg_asm_for_math_add(FILE* f, struct ast_node add);
static void cg_asm_for_math_mul(FILE* f, struct ast_node mul);
static void cg_asm_for_expr_bool(FILE* f, struct ast_node expr);

static void asm_load_operand_to_reg(FILE* f, struct ast_node op, char* reg);

void gen_asm_to_file(FILE *asm_output, struct ast_node ast)
{
  cg_start(asm_output);

  bool main_found = false;
  int label_count = 0;

  for (int i = 0; i < ast.children_len; i++) {
    switch(ast.children[i].type) {
      case AST_FUNCTION:
        bool is_main = cg_asm_for_func(asm_output, ast.children[i], &label_count);
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


static void cg_start(FILE* f)
{
  fprintf(f, "\t.global _start\n");
  fprintf(f, "\t.text\n");
  fprintf(f, "_start:\n");
  fprintf(f, "\txor  %rbp, %rbp\n");
  fprintf(f, "\tand  $0xfffffffffffffff0, %%rsp\n");

  fprintf(f, "\tcall main\n");
  fprintf(f, EXIT_ADDR ":\n");

  fprintf(f, "\tmov  %%rax, %%rdi\n");
  fprintf(f, "\tmov  $60, %%rax\n");
  fprintf(f, "\tsyscall\n");
}


static bool cg_asm_for_func(FILE* f, struct ast_node func, int *label_count)
{
  struct symbol* sym = func.sym_ref;

  fprintf(f, "%s:\n", sym->name);
  fprintf(f, "\tpush %%rbp \t\t# INIT STACK\n");
  fprintf(f, "\tmov  %%rsp, %%rbp\n\n");
  
  struct ast_node scope = func.children[0];
  int stack_offset = 0;
  cg_asm_for_scope(f, scope, &stack_offset, label_count);

  fprintf(f, "\tpop  %%rbp\n");
  if (sym->is_main_func) {
    // always return 0 for now
    fprintf(f, "\txor  %%rax, %%rax\n");
  }
  fprintf(f, "\tret\n");
  return sym->is_main_func;
}

static void cg_asm_for_scope(FILE* f, struct ast_node scope, int *stack_offset, int *label_count)
{
  for (int i = 0; i < scope.children_len; i++) {
    struct ast_node cur = scope.children[i];
    switch (cur.type) {
      case AST_VAR_DECL:
        cg_asm_for_var_decl(f, cur, stack_offset);
        break;
      case AST_ASSIGNMENT:
        cg_asm_for_assignment(f, cur);
        break;
      case AST_IF:
        cg_asm_for_if(f, cur, stack_offset, label_count);
        break;
      case AST_EXIT:
        cg_asm_for_exit(f, cur);
        break;
      default:
        fprintf(stderr, "error: UNREACHABLE func scope cg\n");
        exit(1);
    }
  }
}


static void cg_asm_for_var_decl(FILE* f, struct ast_node var, int *stack_offset)
{
  struct symbol* sym = var.sym_ref;
  assert(sym);

  struct data_type_definition *dt = sym->data_type;
  assert(dt != NULL); 

  // TODO: handle alignment
  *stack_offset -= dt->size;
  sym->stack_offset = *stack_offset;

  fprintf(f, "\tadd  $%d, %%rbp \t\t# VAR DECL \"%s\"\n", *stack_offset, sym->name);
  fprintf(f, "\tmovl $0,  %d(%%rbp)\n\n", *stack_offset);
}


static void cg_asm_for_assignment(FILE* f, struct ast_node assign)
{
  assert(assign.children_len == 2);

  struct ast_node lhs = assign.children[0];
  int lhs_stack_offset = lhs.sym_ref->stack_offset;

  struct ast_node rhs = assign.children[1];
  cg_asm_for_expr(f, rhs);

  if (rhs.children[0].type == AST_EXPR_BOOL) {
    fprintf(f, "\tmovb  %%cl, %d(%%rbp) \t\t# VAR EXP ASSIGN \"%s\"\n\n", lhs_stack_offset, lhs.sym_ref->name);    
  } else {
    fprintf(f, "\tmovl  %%ecx, %d(%%rbp) \t\t# VAR EXP ASSIGN \"%s\"\n\n", lhs_stack_offset, lhs.sym_ref->name);
  }
}


static void cg_asm_for_exit(FILE* f, struct ast_node exit)
{
  struct ast_node rhs = exit.children[0];
 
  cg_asm_for_expr(f, rhs);
  fprintf(f, "\tmovl  %%ecx, %%eax \t\t# EXIT CALL \n");
	fprintf(f, "\tjmp  " EXIT_ADDR "\n\n");
}


static void cg_asm_for_expr(FILE* f, struct ast_node expr)
{
  assert(expr.type == AST_EXPR);

  struct ast_node exp = expr.children[0];
  if      (exp.type == AST_EXPR_MATH) cg_asm_for_expr_math(f, exp);     
  else if (exp.type == AST_EXPR_BOOL) cg_asm_for_expr_bool(f, exp);
}


static void cg_asm_for_expr_math(FILE* f, struct ast_node expr)
{
  assert(expr.type == AST_EXPR_MATH);

  struct ast_node exp = expr.children[0];
  if (exp.type == AST_MATH_ADD) cg_asm_for_math_add(f, exp);     
  else                          asm_load_operand_to_reg(f, exp, "ecx");
}


static void cg_asm_for_math_add(FILE* f, struct ast_node add)
{
  cg_asm_for_math_mul(f, add.children[0]);
  fprintf(f, "\tmovl  %%edx, %%ecx\n");
  for (int i = 1; i < add.children_len; i++) {
    cg_asm_for_math_mul(f, add.children[i]);
    if (add.children[i].prev_token == TOKEN_HYPHEN) {
      fprintf(f, "\tsubl %%edx, %%ecx\n");      
    } else {
      fprintf(f, "\taddl %%edx, %%ecx\n");      
    }
  }
}


static void cg_asm_for_math_mul(FILE* f, struct ast_node mul)
{
  asm_load_operand_to_reg(f, mul.children[0], "edx");
  for (int i = 1; i < mul.children_len; i++) {
    if (mul.children[i].prev_token == TOKEN_SLASH) {
      asm_load_operand_to_reg(f, mul.children[i], "ebx");
      fprintf(f, "\tmovl %%edx, %%eax\n");
      fprintf(f, "\tcltd\n");
      fprintf(f, "\tidivl %%ebx\n");
      fprintf(f, "\tmovl %%eax, %%edx\n");
    } else {
      asm_load_operand_to_reg(f, mul.children[i], "eax");
      fprintf(f, "\timull %%eax, %%edx\n");
      
    }
  }
}


static void cg_asm_for_expr_bool(FILE* f, struct ast_node expr)
{
  assert(expr.type == AST_EXPR_BOOL);

  struct ast_node exp = expr.children[0];
  fprintf(f, "\txor %%ecx, %%ecx\n");
  asm_load_operand_to_reg(f, exp, "cl");
}


static void asm_load_operand_to_reg(FILE* f, struct ast_node op, char* reg)
{
  switch (op.type) {
    case AST_LITERAL:
      if (op.literal_type == DT_BOOL) {
        fprintf(f, "\tmovb $%s, %%%s\n", op.literal._bool == true ? "0xFF" : "0x00", reg);
      } else {
        fprintf(f, "\tmovl  $%d, %%%s\n", op.literal, reg);
      }
      break;

    case AST_SYMBOL:
      int stack_offset = op.sym_ref->stack_offset;
      switch (op.sym_ref->data_type->type) {
      case DT_BOOL:
        fprintf(f, "\tmovb  %d(%%rbp), %%%s \t\t# LOAD VAR \"%s\"\n", stack_offset, reg, op.sym_ref->name);
        break;
      case DT_INT32:
        fprintf(f, "\tmovl  %d(%%rbp), %%%s \t\t# LOAD VAR \"%s\"\n", stack_offset, reg, op.sym_ref->name);
        break;
      }

      break;
    default:
      fprintf(stderr, "error: UNREACHABLE: EXPECTED OPERAND: found %d\n", op.type);
      exit(1);
  }  
}


static void cg_asm_for_if(FILE* f, struct ast_node if_exp, int *stack_offset, int *label_count)
{
  assert(if_exp.type == AST_IF);
  *label_count += 1;
  int lab_id = *label_count;

  struct ast_node condition = if_exp.children[0];  
  cg_asm_for_expr_bool(f, condition);

  fprintf(f, "\tcmpb $0x00, %%cl \t\t # IF CONDITION\n");
  fprintf(f, "\tjz LAB%d\n", lab_id);

  struct ast_node body = if_exp.children[1]; 
  cg_asm_for_scope(f, body, stack_offset, label_count);
  fprintf(f, "LAB%d:\n", lab_id);
}
