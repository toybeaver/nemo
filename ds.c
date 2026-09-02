#include "nemo.h"
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


// ----------------------------
//            HASHMAP
// ----------------------------
static size_t hm_hash(struct hmap *h, const char* key);
static void   hm_rehash(struct hmap* h);


// I have no idea what the optimal stuff is, I'll just use 69
#define HM_INIT_CAP 69
#define HM_GROW_RATE 2
void hm_init(struct hmap *h)
{
  h->len = 0;
  h->cap = HM_INIT_CAP;
  h->entries = calloc(HM_INIT_CAP, sizeof(struct hmap_entry)); 
}


// this free func sucks but I don't really care tbh
void hm_free(struct hmap *h)
{
  for (int i = 0; i < h->cap; i++) {
    if (h->entries[i].key) {
      // TODO: enable when this matters
      // free(h->entries[i].data);
    }
  }
  free(h->entries);
  free(h);
}


void* hm_get(struct hmap *h, const char* key)
{
  size_t idx = hm_hash(h, key);
  struct hmap_entry *entry = &h->entries[idx];
  while (entry->key && strcmp(entry->key, key) != 0) {
    idx = (idx + 1) % h->cap;
    entry = &h->entries[idx];   
  }

  if (!entry->key) return NULL;

  return entry->data;
}


#define HM_HI_LOAD_FACTOR 0.69
void hm_put(struct hmap *h, char* key, void* value)
{
  if (h->len / h->cap > HM_HI_LOAD_FACTOR) hm_rehash(h);

  size_t idx = hm_hash(h, key);
  struct hmap_entry *entry = &h->entries[idx];
  while (entry->key && strcmp(entry->key, key) != 0) {
    idx = (idx + 1) % h->cap;
    entry = &h->entries[idx];
  }

  entry->data = value;
  entry->key = key;
  h->len += 1;
}


// FNV-1a - https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function#FNV-1a_hash
#define FNV_OFFSET_BASIS 0xcbf29ce484222325
#define FNV_PRIME        0x100000001b3
static size_t hm_hash(struct hmap *h, const char* key)
{
  long hash = FNV_OFFSET_BASIS;
  for (const char* c = key; *c != '\0'; c++) {
    hash = (hash * FNV_PRIME) ^ *c;
  }
  return (size_t)(hash % h->cap);
}


static void hm_rehash(struct hmap* h)
{
  struct hmap* h1 = calloc(1, sizeof(struct hmap));
  h1->len = h->len;
  h1->cap = h->cap * HM_GROW_RATE;
  h1->entries = calloc(h1->cap, sizeof(struct hmap_entry)); 

  for (int i = 0; i < h->cap; i++) {
    struct hmap_entry* curr = &h->entries[i];
    if (!curr->key) continue;
    hm_put(h1, curr->key, curr->data);
  }
  free(h->entries);

  *h = *h1;
  free(h1);
}
