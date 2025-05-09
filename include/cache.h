#ifndef CACHE_H
#define CACHE_H

#include "index.h"

int cache_init(int max_entries);
int cache_add(const IndexEntry *entry);
const IndexEntry *cache_get_by_id(int id);
void cache_free();
const IndexEntry *cache_get_by_index(int i);  // novo protótipo


#endif
