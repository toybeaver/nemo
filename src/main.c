#include <nemo.h>
#include <stdio.h>
#include <stdlib.h>


int main()
{
  init_globals();
    
  char* src = read_file("./test.nemo");

  struct lex_token* toks = lex_scan(src);
  struct ast_node   ast  = parse_ast(src, toks); 
  free(toks);

  gen_asm_to_stdout(ast);

  free(src);
  deinit_globals();
  return 0;
}
