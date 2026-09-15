PROJ   = nemo
CC     = gcc
CFLAGS = -pedantic -Werror

.PHONY: all, clean, asm
all: $(PROJ)
	./nemo > out.s && gcc -c out.s && ld out.o
asm: out.s
	gcc -c out.s && ld out.o && ./a.out

clean:
	rm -Rf *.o
	rm -Rf *.out
	rm -Rf $(PROJ)
	rm -Rf out.s

$(PROJ): main.o ds.o codegen.o lexer.o utils.o ast.o nemo.o symbols.o
	gcc $(CFLAGS) -o $@ $^
