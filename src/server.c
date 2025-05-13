#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>

#include "ipc.h"
#include "index.h"
#include "storage.h"
#include "cache.h"

#define CACHE_SIZE 100  // definir tamanho máximo da cache

void cleanup() {
    unlink(SERVER_FIFO);
    cache_free();  // libertar memória da cache
    printf("\nServidor: FIFO e cache removidos.\n");
}

int main() {
    if (create_server_fifo() != 0) return 1;
    if (storage_init() != 0) return 1;
    if (cache_init(CACHE_SIZE) != 0) {
        fprintf(stderr, "Erro ao inicializar a cache\n");
        return 1;
    }

    signal(SIGINT, (void *)cleanup);  // Ctrl+C cleanup

    IndexEntry *indices = NULL;
    int total = 0;

    if (storage_load_all(&indices, &total) == 0) {
        printf("Servidor: %d índices carregados do disco.\n", total);
        for (int i = 0; i < total; i++) {
            cache_add(&indices[i]);  // adicionar cada índice à cache
            printf("  [%d] %s (%d) - %s [%s]\n",
                   indices[i].id, indices[i].title, indices[i].year,
                   indices[i].authors, indices[i].path);
        }
        free(indices);
    }

    printf("Servidor: à escuta em %s\n", SERVER_FIFO);

    while (1) {
        Message msg;
        if (receive_message_from_client(&msg) == 0) {
    
            // Verifica se é uma operação ADD
            if (strncmp(msg.operation, "ADD|", 4) == 0) {
                IndexEntry *entry = parse_add_command(msg.operation);
                if (entry == NULL) {
                    fprintf(stderr, "Erro ao interpretar a operação ADD.\n");
                    continue;
                }
    
                printf("Servidor: a indexar documento -> ID: %d | Título: %s | Autores: %s | Ano: %d | Caminho: %s\n",
                       entry->id, entry->title, entry->authors, entry->year, entry->path);
    
                if (storage_append_index(entry) != 0) {
                    fprintf(stderr, "Erro ao guardar o índice no disco.\n");
                } else {
                    cache_add(entry);  // guardar na cache também
                }
    
                free(entry);
            }
    
            // 🔍 NOVO BLOCO: Pesquisa por campo
            else if (strncmp(msg.operation, "SEARCH|", 7) == 0) {
                char *query = msg.operation + 7;
                char *field = strtok(query, "=");
                char *value = strtok(NULL, "=");
            
                if (!field || !value) {
                    fprintf(stderr, "Erro: pesquisa mal formada.\n");
                    continue;
                }
            
                printf("Servidor: a procurar por %s = %s\n", field, value);
            
                char result[1024] = "";
                for (int i = 0; i < CACHE_SIZE; i++) {
                    const IndexEntry *entry = cache_get_by_index(i);
                    if (!entry) break;
            
                    int match = 0;
                    if (strcmp(field, "title") == 0 && strstr(entry->title, value)) match = 1;
                    else if (strcmp(field, "author") == 0 && strstr(entry->authors, value)) match = 1;
                    else if (strcmp(field, "year") == 0 && entry->year == atoi(value)) match = 1;
            
                    if (match) {
                        char line[256];
                        snprintf(line, sizeof(line), "[%d] %s (%d) - %s [%s]\n",
                                 entry->id, entry->title, entry->year, entry->authors, entry->path);
                        strncat(result, line, sizeof(result) - strlen(result) - 1);
                    }
                }
            
                if (strlen(result) == 0) {
                    snprintf(result, sizeof(result), "Sem resultados.\n");
                }
            
                if (send_response_to_client(msg.client_pid, result) != 0) {
                    fprintf(stderr, "Erro ao enviar resposta ao cliente %d\n", msg.client_pid);
                }
            } else if (strncmp(msg.operation, "SEARCH_CONTENT|", 15) == 0) {
                char *keyword = msg.operation + 15;
                char result[1024] = "";
            
                for (int i = 0; i < CACHE_SIZE; i++) {
                    const IndexEntry *entry = cache_get_by_index(i);
                    if (!entry) break;
            
                    int fd = open(entry->path, O_RDONLY);
                    if (fd == -1) continue;
            
                    char content[1024];
                    ssize_t bytes = read(fd, content, sizeof(content) - 1);
                    close(fd);
            
                    if (bytes <= 0) continue;
                    content[bytes] = '\0';
            
                    if (strstr(content, keyword)) {
                        char line[256];
                        snprintf(line, sizeof(line), "[%d] %s (%d) - %s [%s]\n",
                                 entry->id, entry->title, entry->year,
                                 entry->authors, entry->path);
                        strncat(result, line, sizeof(result) - strlen(result) - 1);
                    }
                }
            
                if (send_response_to_client(msg.client_pid, result) != 0) {
                    fprintf(stderr, "Erro ao enviar resposta ao cliente %d\n", msg.client_pid);
                }

            } else if (strncmp(msg.operation, "GET_META|", 9) == 0) {
                int id = atoi(msg.operation + 9);
                IndexEntry *entry = NULL;
                IndexEntry *entries = NULL;
                int total = 0;
                int from_disk = 0;

                entry = cache_get_by_id(id);
            
                // Se não estiver na cache, tenta carregar do disco
                if (!entry) {
                    if (storage_load_all(&entries, &total) == 0) {
                        for (int i = 0; i < total; i++) {
                            if (entries[i].id == id) {
                                entry = &entries[i];
                                break;
                            }
                        }
                    }
                    // nota: não free(entries) ainda, pois entry pode estar a apontar para lá
                }
            
                if (entry) {
                    char response[256];
                    snprintf(response, sizeof(response), "[%d] %s (%d) - %s [%s]",
                             entry->id, entry->title, entry->year,
                             entry->authors, entry->path);
                    send_response_to_client(msg.client_pid, response);
                } else {
                    send_response_to_client(msg.client_pid, "Documento não encontrado.");
                }
            
                if (from_disk && entries != NULL) {
                    free(entries);
                }

            } else if (strncmp(msg.operation, "COUNT_LINES|", 12) == 0) {
                char *copy = strdup(msg.operation + 12);
                char *id_str = strtok(copy, "|");
                char *word = strtok(NULL, "|");
            
                if (!id_str || !word) {
                    send_response_to_client(msg.client_pid, "Erro: parâmetros inválidos.");
                    free(copy);
                    return 1;
                }
            
                int id = atoi(id_str);
                const IndexEntry *entry = cache_get_by_id(id);
                IndexEntry *from_disk = NULL;
            
                if (!entry) {
                    IndexEntry *entries = NULL;
                    int total = 0;
                    if (storage_load_all(&entries, &total) == 0) {
                        for (int i = 0; i < total; i++) {
                            if (entries[i].id == id) {
                                from_disk = &entries[i];
                                break;
                            }
                        }
                    }
                    entry = from_disk;
                }
            
                if (!entry || entry->year == -1) {
                    send_response_to_client(msg.client_pid, "Documento não encontrado.");
                    free(copy);
                    return 1;
                }
            
                // Abrir e contar linhas com a palavra
                int fd = open(entry->path, O_RDONLY);
                if (fd == -1) {
                    send_response_to_client(msg.client_pid, "Erro ao abrir ficheiro.");
                    free(copy);
                    return 1;
                }
            
                char buf[1024];
                ssize_t len;
                int line_count = 0;
                int match_count = 0;
                char line[1024];
                int pos = 0;
            
                while ((len = read(fd, buf, sizeof(buf))) > 0) {
                    for (ssize_t i = 0; i < len; i++) {
                        if (buf[i] == '\n' || pos >= 1023) {
                            line[pos] = '\0';
                            line_count++;
                            if (strstr(line, word)) match_count++;
                            pos = 0;
                        } else {
                            line[pos++] = buf[i];
                        }
                    }
                }
            
                close(fd);
                char response[128];
                snprintf(response, sizeof(response), "Palavra '%s' encontrada em %d linha(s).", word, match_count);
                send_response_to_client(msg.client_pid, response);
            
                free(copy);
                if (from_disk) free(from_disk);

            } else if (strcmp(msg.operation, "SHUTDOWN") == 0) {
                
                printf("Servidor: encerramento solicitado pelo cliente %d\n", msg.client_pid);
                cleanup();  // remove FIFO e liberta recursos
                exit(0);

            } else {
                printf("Servidor: operação não reconhecida: %s\n", msg.operation);
            }
        }
    }
  

    return 0;
}
