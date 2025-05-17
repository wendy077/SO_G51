#ifndef STORAGE_H
#define STORAGE_H

#include "index.h"

#define INDEX_FILE "data/index.dat"  // caminho relativo ao projeto

int storage_load_all(IndexEntry **entries, int *count);
int storage_append_index(const IndexEntry *entry);
int storage_init();  // cria ficheiro se não existir
int storage_get_max_id();
void storage_set_folder(const char *folder);
void storage_resolve_path(const char *relative, char *resolved, size_t size);

// TODO funções para carregar todos os índices, apagar, etc.

#endif
