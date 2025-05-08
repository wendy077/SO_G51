#ifndef STORAGE_H
#define STORAGE_H

#include "index.h"

#define INDEX_FILE "data/index.dat"  // caminho relativo ao projeto

int storage_append_index(const IndexEntry *entry);
int storage_init();  // cria ficheiro se não existir
// TODO: funções para carregar todos os índices, apagar, etc.

#endif
