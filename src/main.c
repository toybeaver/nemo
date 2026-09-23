#include <nemo.h>
#include <stdio.h>
#include <stdlib.h>


static char* get_file_from_args(int argc, char **argv);

static void compile_from_ast(struct ast_node ast);


int main(int argc, char **argv)
{
  init_globals();

  char* src = get_file_from_args(argc, argv);

  struct lex_token* toks = lex_scan(src);
  struct ast_node   ast  = parse_ast(src, toks);
  free(toks);

  debug_ast(ast);
  compile_from_ast(ast);

  free(src);
  deinit_globals();
  return 0;
}


static char* get_file_from_args(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "error: You need to provide one nemo source file\n");
    fprintf(stderr, "usage: nemo <file>.nemo\n");
    exit(1);
  }
  return read_file(argv[1]);
}


static void compile_from_ast(struct ast_node ast)
{
  FILE *asm_output = fopen("out.s", "w");
  if (asm_output == NULL) {
    fprintf(stderr, "error: COULD NOT WRITE FILE, CHECK DISK\n");
    exit(1);
  }
  gen_asm_to_file(asm_output, ast);
  fclose(asm_output);

  if(system("gcc -c out.s") != 0) {
    fprintf(stderr, "error: You need gcc to run nemo because I'm too lazy and dumb\n");
    fprintf(stderr, "error: to write my own assembler. Maybe someday.");
    exit(1);
  }
  if(system("ld out.o") != 0) {
    fprintf(stderr, "error: You need ld to run nemo because I'm too lazy and dumb\n");
    fprintf(stderr, "error: to write my own linker. Maybe someday.");
    exit(1);
  }
}
