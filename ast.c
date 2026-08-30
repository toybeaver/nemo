#include "symp.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


static bool is_func(const char* src, struct lex_token* cur);

static void parse_func(const char* src, struct lex_token** cur, struct ast_node* parent);


#define MAX_NODES /* for now */ 1024
struct ast_node parse_ast(const char* src, struct lex_token* tokens)
{
  struct ast_node root = {
    .type = AST_ROOT,
    .children = malloc(sizeof(struct ast_node) * MAX_NODES),
    .children_len = 0,
  };
  root.sym_table.parent = NULL;
  root.sym_table.table = calloc(1, sizeof(struct hmap));
  hm_init(root.sym_table.table);

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


static void parse_func(const char* src, struct lex_token** _cur, struct ast_node* parent)
{ 
  struct ast_node* func = &parent->children[parent->children_len];
  parent->children_len += 1;
  func->type = AST_FUNCTION;

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

  struct symbol *sym = malloc(sizeof(struct symbol));
  sym->name         = func_name;
  sym->type         = SYM_FUNC;
  sym->is_main_func = strncmp(func_name, &src[cur->pos], cur->len) == 0;
  hm_put(parent->sym_table.table, func_name, sym);
  func->ref_in_parent = hm_get_ref(parent->sym_table.table, func_name);

  cur += 1;
  if (cur->type != TOKEN_LPAREN || (cur+1)->type != TOKEN_RPAREN) {
    fprintf(stderr, "error: Function declaration without arg list found: \"%s\" at pos %d\n", func_name, cur->pos);
    exit(1);
  }

  cur += 2;
  if (cur->type != TOKEN_LBRACKET || (cur+1)->type != TOKEN_RBRACKET) {
    fprintf(stderr, "error: Function declaration without body found: \"%s\" at pos %d\n", func_name, cur->pos);
    exit(1);
  }

  *_cur = cur+2;
}
