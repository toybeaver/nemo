#include <nemo.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


/*
  S := (FUNC)*
  FUNC := 'func' IDENT '(' ')' SCOPE

  SCOPE := '{' STMT;* '}'

  STMT := VAR_DECL | EXIT | ASSIGN

  VAR_DECL := 'var' IDENT ':' TYPE
  EXIT     := 'exit' EXPR
  ASSIGN   := IDENT '=' EXPR

  EXPR      := EXPR_MATH
  EXPR_MATH := MATH_ADD
  EXPR_BOOL := BOOL_AND

  MATH_ADD := MATH_MUL '+' MATH_ADD | MATH_MUL '-' MATH_ADD | MATH_MUL
  MATH_MUL := OPERAND '*' MATH_MUL | OPERAND '/' MATH_MUL | OPERAND
  OPERAND := LITERAL | IDENT

  BOOL_AND     := BOOL_OPERAND && BOOL_AND | BOOL_OPERAND
  BOOL_OPERAND := BOOL_LITERAL | IDENT  

  TYPE  := 'int32'

  BOOL_LITERAL := 'true' | 'false'
  LITERAL := [0-9]+ | BOOL_LITERAL
  IDENT := [aA-zZ_]+[aA-zZ0-9]*
*/


static struct ast_node* define_node(ASTNodeType type, size_t children_cap, bool with_symtable, struct ast_node* parent); // used for all nodes except root
static struct symbol*   define_symbol(struct ast_node* target, SymbolType type, char* name, struct ast_node* parent);

static bool is_func(const char* src, struct lex_token* cur);
static bool is_var(const char* src, struct lex_token* cur);
static bool is_if(const char* src, struct lex_token* cur);
static bool is_exit(const char* src, struct lex_token* cur);
static bool is_bool_literal(const char* src, struct lex_token* cur);

static void                         consume_single_token(const char* src, struct lex_token **_cur, TokenType type, char expected);
static void                         consume_semi(const char* src, struct lex_token **_cur);
static char*                        consume_ident(const char* src, struct lex_token **_cur, struct ast_node* parent, bool uniq);
static char*                        consume_literal(const char* src, struct lex_token **_cur, struct ast_node* parent);
static struct data_type_definition* consume_type(const char* src, struct lex_token **_cur, struct ast_node* parent); 

static void parse_func(const char *src, struct lex_token **_cur, struct ast_node *parent);
static void parse_scope(const char *src, struct lex_token **_cur, struct ast_node *parent);
static void parse_var(const char *src, struct lex_token **_cur, struct ast_node *parent);
static void parse_exit(const char *src, struct lex_token **_cur, struct ast_node *parent);
static void parse_assign(const char *src, struct lex_token **_cur, struct ast_node *parent);
static void parse_if(const char *src, struct lex_token **_cur, struct ast_node *parent);

static void parse_expr(const char* src, struct lex_token **_cur, struct ast_node *parent);

static bool is_math_expr(const char* src, struct lex_token **_cur, struct ast_node *parent);
static void parse_expr_math(const char* src, struct lex_token **_cur, struct ast_node *parent);
static void parse_math_add(const char* src, struct lex_token **_cur, struct ast_node *parent);
static void parse_math_mul(const char* src, struct lex_token **_cur, struct ast_node *parent);
static void parse_operand(const char *src, struct lex_token **_cur, struct ast_node *parent);

static bool is_bool_expr(const char* src, struct lex_token **_cur, struct ast_node *parent);
static void parse_expr_bool(const char* src, struct lex_token **_cur, struct ast_node *parent);
static void parse_bool_and(const char *src, struct lex_token **_cur, struct ast_node *parent);
static void parse_bool_operand(const char *src, struct lex_token **_cur, struct ast_node *parent);


#define MAX_NODES /* for now */ 128
struct ast_node parse_ast(const char* src, struct lex_token* tokens)
{
  struct ast_node root = {
    .type = AST_ROOT,
    .children = calloc(MAX_NODES, sizeof(struct ast_node)),
    .children_len = 0,
    .sym_table = init_sym_table(NULL),
  };

  struct lex_token* cur = tokens;
  while (cur->type != TOKEN_EOF) {
    if (is_func(src, cur)) parse_func(src, &cur, &root);
    else {
      char* s = get_tokens_content(src, cur);
      fprintf(stderr, "error: [0]Unexpected token found on root scope: \"%s\" at pos %d\n", s, cur->pos);
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

  if (with_symtable) node->sym_table = init_sym_table(parent->sym_table);
  else               node->sym_table = parent->sym_table; // helps with scope propagation when nodes don't
                                                          // need a dedicated sym_table

  parent->children_len += 1;

  return node;
}


static struct symbol* define_symbol(struct ast_node* target, SymbolType type, char* name, struct ast_node* parent)
{
  struct symbol *sym = calloc(1, sizeof(struct symbol));
  sym->name = name;
  sym->type = type;

  hm_put(parent->sym_table->table, name, sym);
  target->sym_ref = sym;

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


static bool is_if(const char* src, struct lex_token* cur)
{
  return cur->type == TOKEN_IF && strncmp(&src[cur->pos], "if", cur->len) == 0;
}


static bool is_exit(const char* src, struct lex_token* cur)
{
  return cur->type == TOKEN_IDENT && strncmp(&src[cur->pos], "exit", cur->len) == 0;  
}


static bool is_bool_literal(const char* src, struct lex_token* cur)
{
  return cur->type == TOKEN_LITERAL &&
         (strncmp(&src[cur->pos], "true", cur->len) == 0 ||
          strncmp(&src[cur->pos], "false", cur->len) == 0);
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


static void consume_semi(const char* src, struct lex_token **_cur)
{
  consume_single_token(src, _cur, TOKEN_SEMICOL, ';');
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

  if (uniq && hm_get(parent->sym_table->table, ident_name) != NULL) {
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
    fprintf(stderr, "error: [2]Unexpected token \"%s\" at pos %d\n", lit, cur->pos);
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

    if      (is_var(src, cur))         { parse_var(src, &cur, scope); consume_semi(src, &cur); }
    else if (is_exit(src, cur))        { parse_exit(src, &cur, scope); consume_semi(src, &cur); }
    else if (is_if(src, cur))            parse_if(src, &cur, scope);
    else if (cur->type == TOKEN_IDENT) { parse_assign(src, &cur, scope); consume_semi(src, &cur); }

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

  *_cur = cur;
}


static void parse_exit(const char* src, struct lex_token** _cur, struct ast_node* parent)
{
  struct lex_token* cur = *_cur;
  struct ast_node* exitt = define_node(AST_EXIT, 1, false, parent);

  cur += 1;

  parse_expr(src, &cur, exitt);

  *_cur = cur;
}



static void parse_assign(const char* src, struct lex_token** _cur, struct ast_node* parent)
{
  struct lex_token* cur = *_cur;

  char* var_name = consume_ident(src, &cur, parent, false);
 
  struct symbol *sym = get_symbol(parent->sym_table, var_name);
  if (sym == NULL) {
    fprintf(stderr, "error: Undeclared variable \"%s\"at pos %d\n", var_name, cur->pos);
    fprintf(stderr, "tip:   try first declaring it with \"var %s: <type>\"\n", var_name);
    exit(1);
  }
 
  switch (cur->type) {
    case TOKEN_ASSIGN: {
        cur += 1;

        struct ast_node* assign = define_node(AST_ASSIGNMENT, 2, false, parent);
        
        struct ast_node* lhs = define_node(AST_SYMBOL, 0, false, assign);
        lhs->sym_ref = sym;
        
        parse_expr(src, &cur, assign);

        break;
    }
    case TOKEN_EOF: {
      fprintf(stderr, "error: Unexpected EOF at pos %d\n", cur->pos);
      exit(1);
    }
    default: {
      char* bad_token = get_tokens_content(src, cur);
      fprintf(stderr, "error: [3]Unexpected token \"%s\" at pos %d\n", bad_token, cur->pos);
      exit(1);      
    }
  }

  *_cur = cur;
}


static void parse_if(const char* src, struct lex_token** _cur, struct ast_node* parent)
{
  struct lex_token* cur = *_cur;

  cur += 1;

  struct ast_node* if_exp = define_node(AST_IF, 2, false, parent);
  parse_expr_bool(src, &cur, if_exp);
  parse_scope(src, &cur, if_exp);

  *_cur = cur;
}


static void parse_expr(const char* src, struct lex_token **_cur, struct ast_node *parent)
{
  struct lex_token* cur = *_cur;

  struct ast_node* expr = define_node(AST_EXPR, 1, false, parent);

  if      (is_math_expr(src, &cur, parent)) parse_expr_math(src, &cur, expr);
  else if (is_bool_expr(src, &cur, parent)) parse_expr_bool(src, &cur, expr);

  *_cur = cur;
}


static void parse_expr_math(const char* src, struct lex_token **_cur, struct ast_node *parent)
{
  struct lex_token* cur = *_cur;

  struct ast_node* expr = define_node(AST_EXPR_MATH, 1, false, parent);

  parse_math_add(src, &cur, expr);

  *_cur = cur;
}


// TODO: handle parenthesis
static bool is_math_expr(const char* src, struct lex_token **_cur, struct ast_node *parent)
{
  struct lex_token* cur = *_cur;

  switch ((*_cur)->type) {
  case TOKEN_LITERAL:
      char* lit = get_tokens_content(src, cur);
      for (char *c = lit; *c != '\0'; c++) {
        if (*c < '0' || *c > '9') return false;
      }
      return true;

  case TOKEN_IDENT:
      char* var_name = get_tokens_content(src, cur);

      struct symbol *sym = get_symbol(parent->sym_table, var_name);
      if (sym == NULL) {
        fprintf(stderr, "error: Undeclared variable \"%s\" at pos %d\n", var_name, cur->pos);
        fprintf(stderr, "tip:   try first declaring it with \"var %s: <type>\"\n", var_name);
        exit(1);
      }

      if (sym->data_type->type == DT_INT32) return true;
  
      break;
  }
  return false;
}


#define MAX_EXPR_SIZE /* for now */ 128
static void parse_math_add(const char* src, struct lex_token **_cur, struct ast_node *parent)
{
  struct lex_token* cur = *_cur;
  struct ast_node* add = define_node(AST_MATH_ADD, MAX_EXPR_SIZE, false, parent);
  
  parse_math_mul(src, &cur, add);

  while (cur->type == TOKEN_SUM || cur->type == TOKEN_HYPHEN) {
    if (cur->type == TOKEN_SUM) {
      consume_single_token(src, &cur, TOKEN_SUM, '+'); 
      parse_math_mul(src, &cur, add);
    }
    else {
      consume_single_token(src, &cur, TOKEN_HYPHEN, '-'); 
      parse_math_mul(src, &cur, add);
      add->children[add->children_len-1].prev_token = TOKEN_HYPHEN;
    }
  }

  *_cur = cur;
}


static void parse_math_mul(const char* src, struct lex_token **_cur, struct ast_node *parent)
{
  struct lex_token* cur = *_cur;
  struct ast_node* mul = define_node(AST_MATH_MUL, MAX_EXPR_SIZE, false, parent);
  
  parse_operand(src, &cur, mul);
  while (cur->type == TOKEN_STAR || cur->type == TOKEN_SLASH) {
    if (cur->type == TOKEN_STAR) {
      consume_single_token(src, &cur, TOKEN_STAR, '*'); 
      parse_operand(src, &cur, mul);
    }
    else {
      consume_single_token(src, &cur, TOKEN_SLASH, '/'); 
      parse_operand(src, &cur, mul);
      mul->children[mul->children_len-1].prev_token = TOKEN_SLASH;
    }
  }
  *_cur = cur;
}


static void parse_operand(const char *src, struct lex_token **_cur, struct ast_node *parent)
{
  struct lex_token* cur = *_cur;
    
  struct ast_node* expr = NULL;
  switch (cur->type) {
    case TOKEN_LITERAL:
      // TODO: validate if it's in fact a numeric literal
      char* lit = consume_literal(src, &cur, parent);
      expr = define_node(AST_LITERAL, 0, false, parent);
      expr->literal._int32 = atoi(lit);
      expr->literal_type = DT_INT32;
      break;

    case TOKEN_IDENT:
      char* var_name = consume_ident(src, &cur, parent, false); 

      struct symbol *sym = get_symbol(parent->sym_table, var_name);
      if (sym == NULL) {
        fprintf(stderr, "error: Undeclared variable \"%s\" at pos %d\n", var_name, cur->pos);
        fprintf(stderr, "tip:   try first declaring it with \"var %s: <type>\"\n", var_name);
        exit(1);
      }

      expr = define_node(AST_SYMBOL, 0, false, parent);
      expr->sym_ref = sym;
      break;

    default:
      char* bad_token = consume_literal(src, &cur, parent);
      fprintf(stderr, "error: [5]Unexpected token \"%s\" at pos %d\n", bad_token, cur->pos);
      exit(1);      
  }

  *_cur = cur;  
}


static bool is_bool_expr(const char* src, struct lex_token **_cur, struct ast_node *parent)
{
  struct lex_token* cur = *_cur;

  switch (cur->type) {
  case TOKEN_LITERAL:
      return is_bool_literal(src, cur);

  case TOKEN_IDENT:
      char* var_name = get_tokens_content(src, cur);

      struct symbol *sym = get_symbol(parent->sym_table, var_name);
      if (sym == NULL) {
        fprintf(stderr, "error: Undeclared variable \"%s\" at pos %d\n", var_name, cur->pos);
        fprintf(stderr, "tip:   try first declaring it with \"var %s: <type>\"\n", var_name);
        exit(1);
      }

      if (sym->data_type->type == DT_BOOL) return true;
  
      break;
  }
  return false;  
}


static void parse_expr_bool(const char* src, struct lex_token **_cur, struct ast_node *parent)
{
  struct lex_token* cur = *_cur;

  struct ast_node* expr = define_node(AST_EXPR_BOOL, 1, false, parent);

  parse_bool_and(src, &cur, expr);  

  *_cur = cur;  
}


static void parse_bool_and(const char *src, struct lex_token **_cur, struct ast_node *parent)
{
  struct lex_token* cur = *_cur;
  struct ast_node* and = define_node(AST_AND_BOOL, MAX_EXPR_SIZE, false, parent);
  
  parse_bool_operand(src, &cur, and);

  while (cur->type == TOKEN_AND) {
    consume_single_token(src, &cur, TOKEN_AND, '&'); 
    parse_bool_operand(src, &cur, and);
  }

  *_cur = cur;
}


static void parse_bool_operand(const char *src, struct lex_token **_cur, struct ast_node *parent)
{
  struct lex_token* cur = *_cur;
    
  struct ast_node* expr = NULL;
  switch (cur->type) {
    case TOKEN_LITERAL:
      if (!is_bool_literal(src, cur)) {
        char* bad_token = consume_literal(src, &cur, parent);
        fprintf(stderr, "error: Unexpected non-bool literal \"%s\" in boolean expression at pos %d\n", bad_token, cur->pos);
        exit(1); 
      }
      
      char* lit = consume_literal(src, &cur, parent);
      expr = define_node(AST_LITERAL, 0, false, parent);
      expr->literal._bool = strncmp(lit, "true", 4) == 0;
      expr->literal_type = DT_BOOL;
      break;

    case TOKEN_IDENT:
      char* var_name = consume_ident(src, &cur, parent, false); 
      struct symbol *sym = get_symbol(parent->sym_table, var_name);
      if (sym == NULL) {
        fprintf(stderr, "error: Undeclared variable \"%s\" at pos %d\n", var_name, cur->pos);
        fprintf(stderr, "tip:   try first declaring it with \"var %s: <type>\"\n", var_name);
        exit(1);
      }

      expr = define_node(AST_SYMBOL, 0, false, parent);
      expr->sym_ref = sym;
      break;

    default:
      char* bad_token = consume_literal(src, &cur, parent);
      fprintf(stderr, "error: [5]Unexpected token \"%s\" at pos %d\n", bad_token, cur->pos);
      exit(1);      
  }

  *_cur = cur;  
}
