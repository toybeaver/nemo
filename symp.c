#include "symp.h"
#include <stdlib.h>

struct hmap* g_keywords = NULL;

void init_globals()
{
  g_keywords = calloc(1, sizeof(struct hmap));
  hm_init(g_keywords);

  hm_put(g_keywords, "func", "");
}


void deinit_globals()
{
  hm_free(g_keywords);
}


bool is_keyword(const char* w)
{
  return hm_get(g_keywords, w) != NULL;
}
