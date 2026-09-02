#include "nemo.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


static struct ast_node* define_node(ASTNodeType type, size_t children_cap, bool with_symtable, struct ast_node* parent); // used for all nodes except root
static struct symbol*   define_symbol(struct ast_node* target, SymbolType type, char* name, struct ast_node* parent);

static bool is_func(const char* src, struct lex_token* cur);
static bool is_var(const char* src, struct lex_token* cur);
static bool is_exit(const char* src, struct lex_token* cur);

static void                         consume_single_token(const char* src, struct lex_token **_cur, TokenType type, char expected);
static char*                        consume_ident(const char* src, struct lex_token **_cur, struct ast_node* parent, bool uniq);
static char*                        consume_literal(const char* src, struct lex_token **_cur, struct ast_node* parent);
static struct data_type_definition* consume_type(const char* src, struct lex_token **_cur, struct ast_node* parent); 

static void parse_func(const char* src, struct lex_token **_cur, struct ast_node* parent);
static void parse_scope(const char* src, struct lex_token **_cur, struct ast_node* parent);
static void parse_var(const char* src, struct lex_token **_cur, struct ast_node* parent);
static void parse_exit(const char* src, struct lex_token **_cur, struct ast_node* parent);
static void parse_ident(const char* src, struct lex_token **_cur, struct ast_node* parent);


#define MAX_NODES /* for now */ 128
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


static struct ast_node* define_node(ASTNodeType type, size_t children_cap, bool with_symtable, struct ast_node* parent)
{
  struct ast_node* node = &parent->children[parent->children_len];
  node->type = type;

  if (children_cap > 0) node->children = calloc(children_cap, sizeof(struct ast_node));   
  else                  node->children = NULL;

  if (with_symtable) node->sym_table = init_sym_table(&parent->sym_table);

  parent->children_len += 1;

  return node;
}


static struct symbol* define_symbol(struct ast_node* target, SymbolType type, char* name, struct ast_node* parent)
{
  struct symbol *sym = calloc(1, sizeof(struct symbol));
  sym->name = name;
  sym->type = type;
  hm_put(parent->sym_table.table, name, sym);
  target->ref_in_parent = hm_get_ref(parent->sym_table.table, name);
  return sym;
}


static bool is_func(const char* src, struct lex_token* cur)
{
  return cur->type == TOKEN_IDENT && strncmp(&src[cur->pos], "func", cur->len) == 0;
}


static bool is_var(const char* src, struct lex_token* cur)
{
  return cur->type == TOKEN_IDENT && strncmp(&src[cur->pos], "var", cur->len) == 0;
}


static bool is_exit(const char* src, struct lex_token* cur)
{
  return cur->type == TOKEN_IDENT && strncmp(&src[cur->pos], "exit", cur->len) == 0;  
}


static void consume_single_token(const char* src, struct lex_token **_cur, TokenType type, char expected)
{
  struct lex_token* cur = *_cur;
  if (cur->type != type) {
    char* bad_token = get_tokens_content(src, cur);
    fprintf(stderr, "error: Expected '%c' found \"%s\" at pos %d\n", expected, bad_token, cur->pos);
    exit(1);
  }
  *_cur = cur+1;
}


static char* consume_ident(const char* src, struct lex_token** _cur, struct ast_node* parent, bool uniq)
{
  struct lex_token* cur = *_cur;
  if (cur->type != TOKEN_IDENT) {
    fprintf(stderr, "error: Unexpected token found at pos %zu: expected indentifier\n", cur->pos);
    exit(1);
  }

  char* ident_name = get_tokens_content(src, cur);
  if (is_keyword(ident_name)) {
    fprintf(stderr, "error: Unexpected keyword found: \"%s\" at pos %zu\n", ident_name, cur->pos);
    exit(1);
  }

  if (uniq && hm_get(parent->sym_table.table, ident_name) != NULL) {
    fprintf(stderr, "error: Unique symbol expected, but second declaration found: \"%s\" at pos %d\n", ident_name, cur->pos);
    exit(1);
  }
  
  *_cur = cur+1;
  
  return ident_name;
}


static char* consume_literal(const char* src, struct lex_token **_cur, struct ast_node* parent)
{
  struct lex_token* cur = *_cur;

  char* lit = get_tokens_content(src, cur);
  if (cur->type != TOKEN_LITERAL) {
    fprintf(stderr, "error: Unexpected token \"%s\" at pos %d\n", lit, cur->pos);
    exit(1);      
  }  

  *_cur = cur+1;

  return lit;
}


static struct data_type_definition* consume_type(const char* src, struct lex_token **_cur, struct ast_node* parent)
{
    struct lex_token* cur = *_cur;

    if (cur->type != TOKEN_IDENT) {
      char* bad_token = get_tokens_content(src, cur);
      fprintf(stderr, "error: Expected type, found \"%s\" at pos %d\n", bad_token, cur->pos);
      exit(1);
    }

    char* type_name = get_tokens_content(src, cur);
    struct data_type_definition* dt = get_datatype_definition(type_name);
    if (dt == NULL) {
      fprintf(stderr, "error: \"%s\" at pos %d is not a valid type\n", type_name, cur->pos);
      exit(1);
    }
    
    *_cur = cur + 1;

    return dt;
}


static void parse_func(const char* src, struct lex_token** _cur, struct ast_node* parent)
{ 
  struct lex_token* cur = *_cur;

  struct ast_node* func = define_node(AST_FUNCTION, 1, true, parent);

  cur += 1;

  char* func_name = consume_ident(src, &cur, parent, true);

  struct symbol* sym = define_symbol(func, SYM_FUNC, func_name, parent);  
  sym->is_main_func = strncmp(func_name, "main", cur->len) == 0;

  consume_single_token(src, &cur, TOKEN_LPAREN, '(');
  consume_single_token(src, &cur, TOKEN_RPAREN, ')');

  parse_scope(src, &cur, func);

  *_cur = cur;
}


#define MAX_SCOPE_STMTS /* for now */ 1024
static void parse_scope(const char* src, struct lex_token** _cur, struct ast_node* parent)
{
  struct lex_token* cur = *_cur;

  struct ast_node* scope = define_node(AST_SCOPE, MAX_SCOPE_STMTS, true, parent);

  consume_single_token(src, &cur, TOKEN_LBRACKET, '{');

  while(true) {
    bool should_quit = false;

    if      (is_var(src, cur))         parse_var(src, &cur, scope); 
    else if (is_exit(src, cur))        parse_exit(src, &cur, scope); 
    else if (cur->type == TOKEN_IDENT) parse_ident(src, &cur, scope);

    switch (cur->type) {
      case TOKEN_EOF: 
        fprintf(stderr, "error: Unexpected EOF in func at pos %d\n", cur->pos);
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
  struct lex_token* cur = *_cur;

  // TODO: this should have a symbol node as a child and in the future maybe an assignment op
  struct ast_node* var_decl = define_node(AST_VAR_DECL, 0, false, parent);

  cur += 1;

  char* var_name = consume_ident(src, &cur, parent, true);
  struct symbol* sym = define_symbol(var_decl, SYM_VAR, var_name, parent);

  consume_single_token(src, &cur, TOKEN_COLON, ':');

  struct data_type_definition* dt = consume_type(src, &cur, parent); 
  sym->data_type = dt;

  consume_single_token(src, &cur, TOKEN_SEMICOL, ';');

  *_cur = cur;
}


static void parse_exit(const char* src, struct lex_token** _cur, struct ast_node* parent)
{
  struct lex_token* cur = *_cur;

  struct ast_node* exitt = define_node(AST_EXIT, 1, false, parent);

  cur += 1;
    
  char* lit = consume_literal(src, &cur, parent);

  struct ast_node* rhs = define_node(AST_LITERAL, 0, false, exitt);
  rhs->literal = atoi(lit);

  consume_single_token(src, &cur, TOKEN_SEMICOL, ';');

  *_cur = cur;
}



static void parse_ident(const char* src, struct lex_token** _cur, struct ast_node* parent)
{
  struct lex_token* cur = *_cur;

  char* var_name = consume_ident(src, &cur, parent, false);
 
  struct hmap_entry *sym_entry = hm_get_ref(parent->sym_table.table, var_name);
  if (sym_entry == NULL) {
    fprintf(stderr, "error: Undeclared variable \"%s\"at pos %d\n", var_name, cur->pos);
    fprintf(stderr, "tip:   try first declaring it with \"var %s: <type>\"\n", var_name);
    exit(1);
  }
 
  switch (cur->type) {
    case TOKEN_ASSIGN: {
        cur += 1;

        struct ast_node* assign = define_node(AST_ASSIGNMENT, 2, false, parent);
        
        struct ast_node* lhs = define_node(AST_SYMBOL, 0, false, assign);
        lhs->ref_in_parent = sym_entry;
        
        char* lit = consume_literal(src, &cur, parent);

        struct ast_node* rhs = define_node(AST_LITERAL, 0, false, assign);
        rhs->literal = atoi(lit);

        consume_single_token(src, &cur, TOKEN_SEMICOL, ';');

        break;
    }
    case TOKEN_EOF: {
      fprintf(stderr, "error: Unexpected EOF at pos %d\n", cur->pos);
      exit(1);
    }
    default: {
      char* bad_token = get_tokens_content(src, cur);
      fprintf(stderr, "error: Unexpected token \"%s\" at pos %d\n", bad_token, cur->pos);
      exit(1);      
    }
  }

  *_cur = cur;
}
