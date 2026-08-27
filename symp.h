#ifndef SYMP_H
#define SYMP_H

#include <stddef.h>

// ============================
//       DATA STRUCTURES
// ============================

// ----------------------------
//            HASHMAP
// ----------------------------
struct hmap_entry {
  char* key;
  void* data;
};

struct hmap {
  size_t len;
  size_t cap;
  struct hmap_entry *entries; 
};

void  hm_init(struct hmap *h);
void  hm_free(struct hmap *h);
// For now support only str keys, changing later is easy
void* hm_get(struct hmap *h, char* key);
void  hm_put(struct hmap *h, char* key, void* value);

#endif
