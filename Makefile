PROJ   = symp
CC     = gcc
CFLAGS = -pedantic -Wall -Werror

$(PROJ): main.o ds.o
	gcc $(CFLAGS) -o $@ $^
