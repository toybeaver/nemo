#ifndef SYMP_H
#define SYMP_H

#include <stddef.h>
#include <stdbool.h>

 
// ============================
//       GLOBALS
// ============================
void init_globals();
void deinit_globals();
bool is_keyword(const char* w);

struct data_type_definition* get_datatype_definition(const char* dt); 


// ============================
//       LEXER
// ============================
typedef int TokenType;
#define TOKEN_ERR      -1
#define TOKEN_EOF      0
#define TOKEN_IDENT    1
#define TOKEN_LPAREN   2
#define TOKEN_RPAREN   3
#define TOKEN_LBRACKET 4
#define TOKEN_RBRACKET 5
#define TOKEN_COLON    6
#define TOKEN_SEMICOL  8
#define TOKEN_ASSIGN    9
#define TOKEN_LITERAL  10

struct lex_token {
  TokenType type;  
  size_t    pos;
  int       len;
};

struct lex_token* lex_scan(const char* src);
char*             get_tokens_content(const char* src, struct lex_token* tok);


// ============================
//       SYMBOLS
// ============================
typedef int SymbolType;
#define SYM_FUNC 0
#define SYM_VAR 1

struct data_type_definition {
  size_t size;
  size_t alignment;
};

struct symbol {
  char*                       name;
  SymbolType                  type;
  bool                        is_main_func;
  int                         stack_offset;
  struct data_type_definition *data_type;
};

struct sym_table {
  struct hmap*      table;
  struct sym_table* parent;
};

struct sym_table* init_sym_table(struct sym_table *parent);
struct symbol*    get_symbol(struct sym_table *table, char *key);

 
// ============================
//       AST
// ============================
typedef int ASTNodeType;
#define AST_ROOT       0
#define AST_FUNCTION   1
#define AST_SCOPE      2
#define AST_VAR_DECL   3
#define AST_ASSIGNMENT 4
#define AST_LITERAL    5
#define AST_EXIT       6
#define AST_NUMBER     7
#define AST_SYMBOL     8

struct ast_node {
  ASTNodeType      type;
  struct ast_node* children;
  size_t           children_len;

  struct sym_table* sym_table;
  struct symbol*    sym_ref;

  int literal;
};

struct ast_node parse_ast(const char* src, struct lex_token* tokens);


// ============================
//       CODEGEN
// ============================
// later I'll probably add a middle layer for SSA before codegen but for
// now this is easier ig
void gen_asm_to_stdout(struct ast_node ast);


// ============================
//       DATA STRUCTURES
// ============================

// ----------------------------
//            HASHMAP
// ----------------------------
struct hmap_entry {
  char* key;
  void* data;
};

struct hmap {
  size_t len;
  size_t cap;
  struct hmap_entry *entries; 
};

void  hm_init(struct hmap *h);
void  hm_free(struct hmap *h);
// For now support only str keys, changing later is easy
void* hm_get(struct hmap *h, const char* key);
void  hm_put(struct hmap *h, char* key, void* value);


// ============================
//       UTILS
// ============================
char* read_file(const char* file_path);


#endif
