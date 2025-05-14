#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cache.h"

// TODO: Ver se a cache depois passa a usar LRU
typedef struct {
    IndexEntry *entries;
    int max;
    int count;
    int start;  // Para FIFO
} Cache;

static Cache cache;

int cache_size() {
    return cache.count;
}

int cache_init(int max_entries) {
    cache.entries = malloc(sizeof(IndexEntry) * max_entries);
    if (!cache.entries) return -1;
    cache.max = max_entries;
    cache.count = 0;
    cache.start = 0;
    return 0;
}

int cache_add(const IndexEntry *entry) {
    if (!entry) return -1;

    int pos;
    if (cache.count < cache.max) {
        pos = cache.count;
        cache.count++;
    } else {
        pos = cache.start;
        cache.start = (cache.start + 1) % cache.max;
    }

    cache.entries[pos] = *entry;  // copia por valor
    return 0;
}

const IndexEntry *cache_get_by_id(int id) {
    for (int i = 0; i < cache.count; i++) {
        int index = (cache.start + i) % cache.max;
        if (cache.entries[index].id == id) {
            return &cache.entries[index];
        }
    }
    return NULL;
}

const IndexEntry *cache_get_by_index(int i) {
    if (i >= cache.count) return NULL;
    int index = (cache.start + i) % cache.max;
    return &cache.entries[index];
}

void cache_free() {
    free(cache.entries);
}
