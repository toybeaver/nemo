#include "symp.h"
#include <stdio.h>
#include <stdlib.h>


int main()
{
  init_globals();
    
  char* src = read_file("./test.symp");
  struct lex_token* toks = lex_scan(src);
  struct ast_node   ast  = parse_ast(src, toks); 
  gen_asm_to_stdout(ast);

  free(toks);
  free(src);
  return 0;
}
