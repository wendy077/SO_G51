#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "index.h"

static int current_id = -1;

int generate_new_id() {
    if (current_id == -1) {
        current_id = storage_get_max_id();  // inicia com o maior ID já usado
    }
    return ++current_id;
}

IndexEntry *parse_add_command(const char *operation_str) {
    if (!operation_str) return NULL;

    // Espera-se: ADD|title|authors|year|path
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

    // Campo: title
    token = strtok(NULL, "|");
    if (!token) { free(copy); free(entry); return NULL; }
    strncpy(entry->title, token, MAX_TITLE);

    // Campo: authors
    token = strtok(NULL, "|");
    if (!token) { free(copy); free(entry); return NULL; }
    strncpy(entry->authors, token, MAX_AUTHORS);

    // Campo: year
    token = strtok(NULL, "|");
    if (!token) { free(copy); free(entry); return NULL; }
    entry->year = atoi(token);

    // Campo: path
    token = strtok(NULL, "|");
    if (!token) { free(copy); free(entry); return NULL; }
    strncpy(entry->path, token, MAX_PATH);

    entry->id = generate_new_id();

    free(copy);
    return entry;
}
