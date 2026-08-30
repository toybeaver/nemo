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

struct lex_token {
  TokenType type;  
  size_t pos;
  int    len;
};

struct lex_token* lex_scan(const char* src);
char*             get_tokens_content(const char* src, struct lex_token* tok);


// ============================
//       SYMBOLS
// ============================
typedef int SymbolType;
#define SYM_FUNC 0

struct symbol {
  char*      name;
  SymbolType type;
  bool       is_main_func;
};

struct sym_table {
  struct hmap*      table;
  struct sym_table* parent;
};

 
// ============================
//       AST
// ============================
typedef int ASTNodeType;
#define AST_ROOT     0
#define AST_FUNCTION 1

struct ast_node {
  ASTNodeType        type;
  struct ast_node*   children;
  size_t             children_len;
  struct sym_table   sym_table;
  struct hmap_entry* ref_in_parent;
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
void*              hm_get(struct hmap *h, const char* key);
struct hmap_entry* hm_get_ref(struct hmap *h, const char* key);
void               hm_put(struct hmap *h, char* key, void* value);


// ============================
//       UTILS
// ============================
char* read_file(const char* file_path);


#endif
