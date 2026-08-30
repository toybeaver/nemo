#include "symp.h"
#include <stdio.h>
#include <stdlib.h>


int main()
{
  char* src = read_file("./test.symp");
  struct lex_token* toks = lex_scan(src);


  // struct hmap* h = calloc(1, sizeof(struct hmap)); hm_init(h);

  // char* k1 = "Jansen";
  // char* k2 = "Ana";
  // char* k3 = "Ed";

  // hm_put(h, k1, "Saunier");
  // hm_put(h, k2, "Paula");

  // printf("Key: '%s' -- Value: %s\n", k1, (char*)hm_get(h, k1));
  // printf("Key: '%s' -- Value: %s\n", k2, (char*)hm_get(h, k2)); 
  // printf("Key: '%s' -- Value: %s\n", k3, (char*)hm_get(h, k3)); 

  // hm_free(h);
  free(toks);
  free(src);
  return 0;
}
