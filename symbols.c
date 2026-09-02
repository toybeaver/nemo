#include "nemo.h"
#include <stdlib.h>


struct sym_table* init_sym_table(struct sym_table* parent)
{
  struct sym_table *st = calloc(1, sizeof(struct sym_table));
  st->parent = parent;
  st->table = calloc(1, sizeof(struct hmap));
  hm_init(st->table);
  return st;
}


struct symbol* get_symbol(struct sym_table *table, char *key)
{
  while (table->table != NULL) {
    struct symbol* sym = hm_get(table->table, key);
    if (sym != NULL) {
      return sym;
    }
    table = table->parent;
  }
  return NULL;
}
