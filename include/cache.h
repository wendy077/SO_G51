// === cache.h ===

#ifndef CACHE_H
#define CACHE_H

#include "index.h"

typedef struct CacheEntry CacheEntry;

CacheEntry *cache_head();  
int cache_init(int max_entries);
int cache_add(const IndexEntry *entry);
const IndexEntry *cache_get_by_id(int id);
const IndexEntry *cache_get_by_index(int index);
void cache_remove_by_id(int id);
void cache_touch(int id);
void cache_free();
int cache_size();

#endif // CACHE_H
