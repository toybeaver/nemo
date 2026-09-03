#include "nemo.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>


static struct lex_token next_token(const char* src, int initial_pos);

static bool try_parse_ident(const char* src, size_t cur, struct lex_token *tok);
static bool try_parse_literal(const char* src, size_t cur, struct lex_token *tok);

static void dbg_token(const char* src, struct lex_token t);


#define MAX_TOKENS /* for now */ 1024
struct lex_token* lex_scan(const char* src)
{
  struct lex_token *t = malloc(sizeof(struct lex_token) * MAX_TOKENS);
  size_t i = 0;

  t[i] = next_token(src, 0);
  while (t[i].type != TOKEN_ERR && t[i].type != TOKEN_EOF && i < MAX_TOKENS) {
    i += 1;
    t[i] = next_token(src, t[i-1].len + t[i-1].pos);
  }
 
  if (t[i].type == TOKEN_ERR) {
    fprintf(stderr, "error: Unexpected token found: '%c'(ascii:%d) at pos %zu\n", src[t[i].pos], src[t[i].pos], t[i].pos);
    exit(1);
  }

  if (t[i].type != TOKEN_EOF) {
    fprintf(stderr, "error: Expected EOF, but found: '%c' at pos %zu\n", src[t[i].pos], t[i].pos);
    exit(1);
  }
 
  return t;
}


char* get_tokens_content(const char* src, struct lex_token* tok)
{
  char* st = malloc(tok->len+1);
  strncpy(st, &src[tok->pos], tok->len);
  st[tok->len] = '\0';
  return st;
}


static struct lex_token next_token(const char* src, int initial_pos)
{
  struct lex_token tok = { .len = 1 };
  size_t cur = initial_pos;

  while (isspace(src[cur]) && src[cur] != '\0') cur++; 

  if (src[cur] == '\0' || src[cur] == EOF) {
    tok.type = TOKEN_EOF;
    tok.pos = cur;
    return tok;
  }

  int pre = cur;

  if (try_parse_ident(src, cur, &tok)) return tok;
  if (try_parse_literal(src, cur, &tok)) return tok;

  tok.pos = cur;

  switch(src[cur]) {
    case '(': tok.type = TOKEN_LPAREN;   return tok; 
    case ')': tok.type = TOKEN_RPAREN;   return tok; 
    case '{': tok.type = TOKEN_LBRACKET; return tok; 
    case '}': tok.type = TOKEN_RBRACKET; return tok; 
    case ':': tok.type = TOKEN_COLON;    return tok;
    case ';': tok.type = TOKEN_SEMICOL;  return tok;
    case '=': tok.type = TOKEN_ASSIGN;   return tok;
    case '+': tok.type = TOKEN_SUM;      return tok;
  }

  tok.type = TOKEN_ERR;
  tok.pos = cur;
  return tok;
}


static bool try_parse_ident(const char* src, size_t cur, struct lex_token *tok)
{
  if (!isalpha(src[cur]) && src[cur] != '_') return false;
  
  size_t init = cur;
  while (isalnum(src[cur]) || src[cur] == '_') cur += 1;
  tok->type = TOKEN_IDENT;
  tok->pos = init;
  tok->len = cur - init;
  return true;
}


static bool try_parse_literal(const char* src, size_t cur, struct lex_token *tok)
{
  if (!isdigit(src[cur])) return false;
  
  size_t init = cur;
  while (isdigit(src[cur])) cur += 1;
  tok->type = TOKEN_LITERAL;
  tok->pos = init;
  tok->len = cur - init;
  return true;
}


static void dbg_token(const char* src, struct lex_token t)
{
  printf("Token: %c - %zu - %d - %d\n", src[t.pos], t.pos, t.len, t.type);
}
