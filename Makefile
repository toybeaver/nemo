PROJ   = nemo
CC     = gcc
CFLAGS = -pedantic -Werror -I./include

.PHONY: all, clean, asm
all: $(PROJ)
	./nemo > out.s && gcc -c out.s && ld out.o
asm: out.s
	gcc -c out.s && ld out.o && ./a.out

clean:
	rm -Rf **/*.o
	rm -Rf **/*.out
	rm -Rf $(PROJ)
	rm -Rf out.s

$(PROJ): src/main.o src/ds.o src/codegen.o src/lexer.o src/utils.o src/ast.o src/nemo.o src/symbols.o
	gcc $(CFLAGS) -o $@ $^
