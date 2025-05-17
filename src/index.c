#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "index.h"

#define MAX_TITLE_LEN 200
#define MAX_AUTHORS_LEN 200
#define MAX_PATH_LEN 64

static int current_id = -1;

int generate_new_id() {
    if (current_id == -1) {
        current_id = storage_get_max_id();  // inicia com o maior ID já usado
    }
    return ++current_id;
}

IndexEntry *parse_add_command(const char *operation_str) {
    if (!operation_str) return NULL;

    char *copy = strdup(operation_str);
    if (!copy) return NULL;

    IndexEntry *entry = malloc(sizeof(IndexEntry));
    if (!entry) {
        free(copy);
        return NULL;
    }

    char *token = strtok(copy, "|");
    if (!token || strcmp(token, "ADD") != 0) {
        free(copy); free(entry);
        return NULL;
    }

    // Title
    token = strtok(NULL, "|");
    if (!token || strlen(token) >= MAX_TITLE_LEN) {
        free(copy); free(entry);
        fprintf(stderr, "Erro: Título demasiado longo ou ausente.\n");
        return NULL;
    }
    strncpy(entry->title, token, MAX_TITLE_LEN);

    // Authors
    token = strtok(NULL, "|");
    if (!token || strlen(token) >= MAX_AUTHORS_LEN) {
        free(copy); free(entry);
        fprintf(stderr, "Erro: Autores demasiado longos ou ausentes.\n");
        return NULL;
    }
    strncpy(entry->authors, token, MAX_AUTHORS_LEN);

    // Year (número)
    token = strtok(NULL, "|");
    if (!token) {
        free(copy); free(entry);
        fprintf(stderr, "Erro: Ano não fornecido.\n");
        return NULL;
    }
    for (size_t i = 0; token[i]; i++) {
        if (!isdigit(token[i])) {
            free(copy); free(entry);
            fprintf(stderr, "Erro: Ano inválido.\n");
            return NULL;
        }
    }
    entry->year = atoi(token);

    // Path
    token = strtok(NULL, "|");
    if (!token || strlen(token) >= MAX_PATH_LEN) {
        free(copy); free(entry);
        fprintf(stderr, "Erro: Caminho demasiado longo ou ausente.\n");
        return NULL;
    }
    strncpy(entry->path, token, MAX_PATH_LEN);

    free(copy);
    return entry;
}


