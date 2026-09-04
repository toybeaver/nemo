#include "nemo.h"
#include <stdlib.h>


struct hmap* g_keywords = NULL;
struct hmap* g_datatypes = NULL;


static void init_kws();
static void init_datatypes();


void init_globals()
{
  init_kws();
  init_datatypes();
}


void deinit_globals()
{
  hm_free(g_keywords);
  hm_free(g_datatypes);
}


bool is_keyword(const char* w)
{
  return hm_get(g_keywords, w) != NULL;
}


struct data_type_definition* get_datatype_definition(const char* dt)
{
  return (struct data_type_definition*)hm_get(g_datatypes, dt);
}


static void init_kws()
{
  g_keywords = calloc(1, sizeof(struct hmap));
  hm_init(g_keywords);

  hm_put(g_keywords, "exit",  "");
  hm_put(g_keywords, "func",  "");
  hm_put(g_keywords, "var",   "");
  hm_put(g_keywords, "int32", "");
}


static void init_datatypes()
{
  g_datatypes = calloc(1, sizeof(struct hmap));
  hm_init(g_datatypes);

  struct data_type_definition *int32 = calloc(1, sizeof(struct data_type_definition));
  int32->size = 4; int32->alignment = 4;
  hm_put(g_datatypes, "int32", int32);
}
