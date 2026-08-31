PROJ   = symp
CC     = gcc
CFLAGS = -pedantic -Werror

.PHONY: all, clean
all: $(PROJ)
	./symp > out.s && gcc -c out.s && ld out.o

clean:
	rm -Rf *.o
	rm -Rf *.out
	rm -Rf $(PROJ)
	rm -Rf out.s

$(PROJ): main.o ds.o codegen.o lexer.o utils.o ast.o symp.o symbols.o
	gcc $(CFLAGS) -o $@ $^
