#include "nemo.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


static bool is_func(const char* src, struct lex_token* cur);
static bool is_var(const char* src, struct lex_token* cur);

static void parse_func(const char* src, struct lex_token** cur, struct ast_node* parent);
static void parse_scope(const char* src, struct lex_token** cur, struct ast_node* parent);
static void parse_var(const char* src, struct lex_token** cur, struct ast_node* parent);

#define MAX_NODES /* for now */ 1024
struct ast_node parse_ast(const char* src, struct lex_token* tokens)
{
  struct ast_node root = {
    .type = AST_ROOT,
    .children = malloc(sizeof(struct ast_node) * MAX_NODES),
    .children_len = 0,
    .sym_table = init_sym_table(NULL),
  };

  struct lex_token* cur = tokens;
  while (cur->type != TOKEN_EOF) {
    if (is_func(src, cur)) parse_func(src, &cur, &root);
    else {
      char* s = get_tokens_content(src, cur);
      fprintf(stderr, "error: Unexpected token found on root scope: \"%s\" at pos %d\n", s, cur->pos);
      exit(1);
    }
  }

  return root;
}


static bool is_func(const char* src, struct lex_token* cur)
{
  return cur->type == TOKEN_IDENT && strncmp(&src[cur->pos], "func", cur->len) == 0;
}


static bool is_var(const char* src, struct lex_token* cur)
{
  return cur->type == TOKEN_IDENT && strncmp(&src[cur->pos], "var", cur->len) == 0;
}


static void parse_func(const char* src, struct lex_token** _cur, struct ast_node* parent)
{ 
  struct ast_node* func = &parent->children[parent->children_len];
  func->type = AST_FUNCTION;
  func->children = malloc(sizeof(struct ast_node));
  func->children_len = 1; // there's always 1 scope, even when empty 
  func->sym_table = init_sym_table(&parent->sym_table);
  parent->children_len += 1;

  struct lex_token* cur = *_cur;

  cur += 1;
  if (cur->type != TOKEN_IDENT) {
    fprintf(stderr, "error: Function without a name found at at pos %zu\n", (cur-1)->pos);
    exit(1);
  }
    
  char* func_name = get_tokens_content(src, cur);
  if (is_keyword(func_name)) {
    fprintf(stderr, "error: Unexpected keyword as function name: \"%s\" at pos %zu\n", func_name, cur->pos);
    exit(1);
  }

  if(hm_get(parent->sym_table.table, func_name) != NULL) {
    fprintf(stderr, "error: Two functions with the same name in the same scope found:  \"%s\" at pos %d\n", func_name, cur->pos);
    exit(1);
  }
  struct symbol *sym = calloc(1, sizeof(struct symbol));
  sym->name         = func_name;
  sym->type         = SYM_FUNC;
  sym->is_main_func = strncmp(func_name, "main", cur->len) == 0;

  hm_put(parent->sym_table.table, func_name, sym);
  func->ref_in_parent = hm_get_ref(parent->sym_table.table, func_name);

  cur += 1;
  if (cur->type != TOKEN_LPAREN || (cur+1)->type != TOKEN_RPAREN) {
    fprintf(stderr, "error: Function declaration without arg list found: \"%s\" at pos %d\n", func_name, cur->pos);
    exit(1);
  }
  cur += 2;

  parse_scope(src, &cur, func);

  *_cur = cur;
}


#define MAX_SCOPE_STMTS /* for now */ 1024
static void parse_scope(const char* src, struct lex_token** _cur, struct ast_node* parent)
{
  struct ast_node* scope = &parent->children[0];
  scope->type = AST_SCOPE;
  scope->children = malloc(sizeof(struct ast_node) * MAX_SCOPE_STMTS);
  scope->children_len = 0;
  scope->sym_table = init_sym_table(&parent->sym_table);

  struct lex_token* cur = *_cur;

  struct symbol* func_id = (void*)parent->ref_in_parent->data;
  char* func_name = func_id->name;

  if (cur->type != TOKEN_LBRACKET) {
    char* bad_token = get_tokens_content(src, cur);
    fprintf(stderr, "error: Expected '{' found \"%s\" in func \"%s\" at pos %d\n", bad_token, func_name, cur->pos);
    exit(1);
  }
  cur += 1;


  while(true) {
    bool should_quit = false;

    if (is_var(src, cur)) parse_var(src, &cur, scope); 

    switch (cur->type) {
      case TOKEN_EOF: 
        fprintf(stderr, "error: Unexpected EOF in func \"%s\" at pos %d\n", func_name, cur->pos);
        exit(1);

      case TOKEN_RBRACKET:
        cur += 1;
        should_quit = true;
        break;
    }

    if (should_quit) break;
  }


  *_cur = cur;
}


static void parse_var(const char* src, struct lex_token** _cur, struct ast_node* parent)
{
  struct ast_node* var_decl = &parent->children[parent->children_len];
  var_decl->type = AST_VAR_DECL;
  parent->children_len += 1;

  struct lex_token* cur = *_cur;

  cur += 1;
  if (cur->type != TOKEN_IDENT) {
    char* bad_token = get_tokens_content(src, cur);
    fprintf(stderr, "error: Expected variable name, found \"%s\" at pos %d\n", bad_token, cur->pos);
    exit(1);
  }  

  char* var_name = get_tokens_content(src, cur); 
  if(hm_get(parent->sym_table.table, var_name) != NULL) {
    fprintf(stderr, "error: Two variables with the same name in the same scope found:  \"%s\" at pos %d\n", var_name, cur->pos);
    exit(1);
  }
  
  struct symbol *sym = calloc(1, sizeof(struct symbol));
  sym->name = var_name;
  sym->type = SYM_VAR;
  sym->stack_offset = -1;

  cur += 1;
  if (cur->type != TOKEN_COLON) {
    char* bad_token = get_tokens_content(src, cur);
    fprintf(stderr, "error: Expected ':', found \"%s\" at pos %d\n", bad_token, cur->pos);
    exit(1);
  }

  cur += 1;
  if (cur->type != TOKEN_IDENT) {
    char* bad_token = get_tokens_content(src, cur);
    fprintf(stderr, "error: Expected variable type, found \"%s\" at pos %d\n", bad_token, cur->pos);
    exit(1);
  }

  char* type_name = get_tokens_content(src, cur);
  struct data_type_definition* dt = get_datatype_definition(type_name);
  if (dt == NULL) {
    fprintf(stderr, "error: \"%s\" at pos %d is not a valid type\n", type_name, cur->pos);
    exit(1);
  }

  sym->data_type = dt;

  hm_put(parent->sym_table.table, var_name, sym);
  var_decl->ref_in_parent = hm_get_ref(parent->sym_table.table, var_name);  

  cur += 1;
  if (cur->type != TOKEN_SEMICOL) {
    char* bad_token = get_tokens_content(src, cur);
    fprintf(stderr, "error: Expected ';', found \"%s\" at pos %d\n", bad_token, cur->pos);
    exit(1);
  }

  *_cur = cur+1;
}
