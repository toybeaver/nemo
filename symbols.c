#include "symp.h"
#include <stdlib.h>


struct sym_table init_sym_table(struct sym_table* parent)
{
  struct sym_table st = {
    .parent = parent,
    .table = calloc(1, sizeof(struct hmap)),
  };
  hm_init(st.table);
  return st;
}
