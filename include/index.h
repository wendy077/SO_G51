#ifndef INDEX_H
#define INDEX_H

#define MAX_TITLE 200
#define MAX_AUTHORS 200
#define MAX_PATH 64

typedef struct {
    int id;                         
    char title[MAX_TITLE];          
    char authors[MAX_AUTHORS];     
    int year;                       
    char path[MAX_PATH];             
} IndexEntry;

// Gera um novo ID (incremental)
int generate_new_id();

// Faz parsing de uma string de operação "ADD" e retorna uma estrutura IndexEntry
IndexEntry *parse_add_command(const char *operation_str);

#endif
