#ifndef SYMP_H
#define SYMP_H

#include <stddef.h>


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


// ============================
//       CODEGEN
// ============================
void cg_start();
void cg_exit(int exit_code);


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
void* hm_get(struct hmap *h, char* key);
void  hm_put(struct hmap *h, char* key, void* value);


// ============================
//       UTILS
// ============================
char* read_file(const char* file_path);


#endif
