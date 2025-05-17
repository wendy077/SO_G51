#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cache.h"

typedef struct CacheEntry {
    IndexEntry data;
    struct CacheEntry *prev;
    struct CacheEntry *next;
} CacheEntry;

typedef struct {
    CacheEntry *head;
    CacheEntry *tail;
    int count;
    int max;
} Cache;

static Cache cache;

int cache_init(int max_entries) {
    cache.head = NULL;
    cache.tail = NULL;
    cache.count = 0;
    cache.max = max_entries;
    return 0;
}

void cache_move_to_front(CacheEntry *entry) {
    if (entry == cache.head) return;

    if (entry->prev) entry->prev->next = entry->next;
    if (entry->next) entry->next->prev = entry->prev;

    if (entry == cache.tail) cache.tail = entry->prev;

    entry->prev = NULL;
    entry->next = cache.head;
    if (cache.head) cache.head->prev = entry;
    cache.head = entry;
    if (!cache.tail) cache.tail = entry;
}

int cache_add(const IndexEntry *entry) {
    if (!entry || entry->year == -1) return -1;

    // Check if already exists
    CacheEntry *curr = cache.head;
    while (curr) {
        if (curr->data.id == entry->id) {
            printf("Cache: Acesso a ID=%d (mover para frente)\n", entry->id);
            cache_move_to_front(curr);
            return 0;
        }
        curr = curr->next;
    }

    // Create new entry
    CacheEntry *new_entry = malloc(sizeof(CacheEntry));
    if (!new_entry) return -1;
    new_entry->data = *entry;
    new_entry->prev = NULL;
    new_entry->next = NULL;

    if (cache.count >= cache.max) {
        // Remove least recently used (tail)
        CacheEntry *to_remove = cache.tail;
        if (to_remove->prev)
            to_remove->prev->next = NULL;
        cache.tail = to_remove->prev;
        if (to_remove == cache.head) cache.head = NULL;
        printf("Cache: Removido LRU ID=%d\n", to_remove->data.id);
        free(to_remove);
        cache.count--;
    }

    // Add to front
    new_entry->next = cache.head;
    if (cache.head) cache.head->prev = new_entry;
    cache.head = new_entry;
    if (!cache.tail) cache.tail = new_entry;
    cache.count++;
    printf("Cache: Adicionado ID=%d (nova entrada)\n", entry->id);
    return 0;
}

const IndexEntry *cache_get_by_id(int id) {
    CacheEntry *curr = cache.head;
    while (curr) {
        if (curr->data.id == id) {
            cache_move_to_front(curr);
            return &curr->data;
        }
        curr = curr->next;
    }
    return NULL;
}

const IndexEntry *cache_get_by_index(int index) {
    CacheEntry *curr = cache.head;
    int i = 0;
    while (curr && i < index) {
        curr = curr->next;
        i++;
    }
    return curr ? &curr->data : NULL;
}

void cache_remove_by_id(int id) {
    CacheEntry *curr = cache.head;
    while (curr) {
        if (curr->data.id == id) {
            if (curr->prev) curr->prev->next = curr->next;
            if (curr->next) curr->next->prev = curr->prev;
            if (curr == cache.head) cache.head = curr->next;
            if (curr == cache.tail) cache.tail = curr->prev;
            free(curr);
            cache.count--;
            return;
        }
        curr = curr->next;
    }
}

void cache_touch(int id) {
    cache_get_by_id(id);  // Move to front if exists
}

void cache_free() {
    CacheEntry *curr = cache.head;
    while (curr) {
        CacheEntry *next = curr->next;
        free(curr);
        curr = next;
    }
    cache.head = cache.tail = NULL;
    cache.count = 0;
}

int cache_size() {
    return cache.count;
}

CacheEntry *cache_head() {
    return cache.head;
}
