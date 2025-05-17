#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#include "ipc.h"
#include "index.h"
#include "storage.h"
#include "cache.h"
#include <time.h>

#define LOG_FILE "data/log.txt"

void log_operation(const char *op, const char *details) {
    int fd = open(LOG_FILE, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd == -1) return;

    time_t now = time(NULL);
    char buffer[512];
    int len = snprintf(buffer, sizeof(buffer), "%s | %s | %s\n", ctime(&now), op, details);

    if (len > 0) write(fd, buffer, len);
    close(fd);
}

void cleanup() {
    unlink(SERVER_FIFO);
    cache_free();  
    printf("\nServidor: FIFO e cache removidos.\n");
    exit(0);
}

void to_lowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}

void process_operation(Message *msg) {
    if (strncmp(msg->operation, "ADD|", 4) == 0) {
        IndexEntry *entry = parse_add_command(msg->operation);
        if (entry == NULL) {
            fprintf(stderr, "Erro ao interpretar a operação ADD.\n");
            send_response_to_client(msg->client_pid, "Erro ao interpretar a operação ADD.\n");
            exit(0);
        }

        int duplicado = 0;
        for (int i = 0; i < cache_size(); i++) {
            const IndexEntry *e = cache_get_by_index(i);
            if (e && e->year != -1 && strcmp(e->path, entry->path) == 0) {
                duplicado = 1;
                break;
            }
        }

        // Verifica no disco se não está na cache
        if (!duplicado) {
            IndexEntry *all = NULL;
            int count = 0;
            if (storage_load_all(&all, &count) == 0) {
                for (int i = 0; i < count; i++) {
                    if (strcmp(all[i].path, entry->path) == 0) {
                        duplicado = 1;
                        break;
                    }
                }
                free(all);
            }
        }
        
        if (duplicado) {
            printf("Servidor: Documento já indexado: %s\n", entry->path);
            send_response_to_client(msg->client_pid, "Documento já está indexado.\n");
            free(entry);
            exit(0);
        }

        entry->id = generate_new_id();

        
        printf("Servidor: a indexar documento -> ID: %d | Título: %s | Autores: %s | Ano: %d | Caminho: %s\n",
               entry->id, entry->title, entry->authors, entry->year, entry->path);
        
        char response[128];
        
        if (storage_append_index(entry) != 0) {
            fprintf(stderr, "Erro ao guardar o índice no disco.\n");
            snprintf(response, sizeof(response), "Erro ao guardar o documento.");
        } else {
            cache_add(entry);  
            snprintf(response, sizeof(response), "Document %d indexed", entry->id);
        }
        
        send_response_to_client(msg->client_pid, response);

        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "ID=%d | Título=%s | Caminho=%s", entry->id, entry->title, entry->path);
        log_operation("ADD", log_msg);

        free(entry);
      

    } else if (strncmp(msg->operation, "SEARCH_CONTENT|", 15) == 0) {
        char *keyword = msg->operation + 15;
        char keyword_lc[256];
        strncpy(keyword_lc, keyword, sizeof(keyword_lc));
        keyword_lc[sizeof(keyword_lc) - 1] = '\0';
        to_lowercase(keyword_lc);
        char result[1024] = "";
    
        int total = cache_size();
            for (int i = 0; i < total; i++) {
                const IndexEntry *entry = cache_get_by_index(i);

            if (!entry || entry->year == -1) {
                exit(0);
            }

            if (!entry->path || strlen(entry->path) == 0) exit(0);
            int fd = open(entry->path, O_RDONLY);
            if (fd == -1) {
                exit(0);
            }

            off_t size = lseek(fd, 0, SEEK_END);
                if (size <= 0) {
                    close(fd);
                    exit(0);
                }
                lseek(fd, 0, SEEK_SET);

                char *buf = malloc(size + 1);
                if (!buf) {
                    close(fd);
                    exit(0);
                }

                ssize_t len = read(fd, buf, size);

                close(fd);

                if (len <= 0) {
                    free(buf);
                    exit(0);
                }
                buf[len] = '\0';
                to_lowercase(buf);


                if (strstr(buf, keyword_lc)) {
                    char id_buf[16];
                    snprintf(id_buf, sizeof(id_buf), "%d,", entry->id);
                    strncat(result, id_buf, sizeof(result) - strlen(result) - 1);
                                        
                }

                free(buf);

        }

        size_t len = strlen(result);
        if (len > 0 && result[len - 1] == ',') {
            result[len - 1] = '\0'; 
        }

        char formatted[1024];
        if (strlen(result) == 0) {
            strcpy(formatted, "[]");
        } else {
            snprintf(formatted, sizeof(formatted), "[%s]", result);
        }

        send_response_to_client(msg->client_pid, formatted);

        char log_msg[128];
        snprintf(log_msg, sizeof(log_msg), "Keyword=%s | Resultado=%s", keyword, formatted);
        log_operation("SEARCH_CONTENT", log_msg);
        
        
    } else if (strncmp(msg->operation, "GET_META|", 9) == 0) {
        int id = atoi(msg->operation + 9);
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
                        static IndexEntry temp_entry;  
                        temp_entry = entries[i];      
                        entry = &temp_entry;
                        from_disk = 1;
                        break;
                    }
                    
                }
            }
        }
    
        if (entry && entry->year != -1) {
            char response[256];
            const char *filename = strrchr(entry->path, '/');
            filename = (filename != NULL) ? filename + 1 : entry->path;

            snprintf(response, sizeof(response), "Title: %s\nAuthors: %s\nYear: %d\nPath: %s",
                entry->title, entry->authors, entry->year, filename);
            send_response_to_client(msg->client_pid, response);
        } else {
            send_response_to_client(msg->client_pid, "Documento não encontrado.");
        }
    
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "ID=%d | Title=%s | Found=%s", id,
                (entry ? entry->title : "NULL"),
                (entry && entry->year != -1 ? "yes" : "no"));
        log_operation("GET_META", log_msg);

        if (from_disk && entries != NULL) {
            free(entries);
        }

    } else if (strncmp(msg->operation, "COUNT_LINES|", 12) == 0) {
        char *copy = strdup(msg->operation + 12);
        char *id_str = strtok(copy, "|");
        char *word = strtok(NULL, "|");
    
        if (!id_str || !word) {
            send_response_to_client(msg->client_pid, "Erro: parâmetros inválidos.");
            free(copy);
            exit(1);
        }
    
        int id = atoi(id_str);
        const IndexEntry *entry = cache_get_by_id(id);
        IndexEntry *from_disk = NULL;
        IndexEntry *entries = NULL;
    
        if (entry == NULL) {
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
            send_response_to_client(msg->client_pid, "Documento não encontrado.");
            free(copy);
            exit(1);
        }
    
        if (!entry->path || strlen(entry->path) == 0) exit(0);
        int fd = open(entry->path, O_RDONLY);
        if (fd == -1) {
            send_response_to_client(msg->client_pid, "Erro ao abrir ficheiro.");
            free(copy);
            exit(1);
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
                    char temp_line[1024];
                    char temp_word[128];
                    
                    strncpy(temp_line, line, sizeof(temp_line));
                    strncpy(temp_word, word, sizeof(temp_word));

                    to_lowercase(temp_line);
                    to_lowercase(temp_word);

                    if (strstr(temp_line, temp_word)) {
                        match_count++;
                    }
                    pos = 0;
                } else {
                    line[pos++] = buf[i];
                }
            }
        }
    
        close(fd);
        char response[128];
        snprintf(response, sizeof(response), "Palavra '%s' encontrada em %d linha(s).", word, match_count);
        send_response_to_client(msg->client_pid, response);
    
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "ID=%d | Palavra='%s' | Linhas com match=%d",
                id, word, match_count);
        log_operation("COUNT_LINES", log_msg);

        free(copy);
        
        if (entries != NULL) {
            free(entries);
        }

    } else if (strncmp(msg->operation, "DELETE|", 7) == 0) {
        int id = atoi(msg->operation + 7);
        IndexEntry *entries = NULL;
        int total = 0;
        int found = 0;
    
        if (storage_load_all(&entries, &total) == 0) {
            for (int i = 0; i < total; i++) {
                if (entries[i].id == id && entries[i].year != -1) {
                    entries[i].year = -1;
                    found = 1;
                    break;
                }
            }
        }
    
        if (!found) {
            send_response_to_client(msg->client_pid, "Documento não encontrado.");
            log_operation("DELETE", "Falha: documento não encontrado");
            free(entries);
        } else {
            int fd = open("data/index.dat", O_WRONLY | O_TRUNC);
            if (fd != -1) {
                write(fd, entries, total * sizeof(IndexEntry));
                close(fd);

            }
            cache_remove_by_id(id);
            char deleted_msg[64];
            snprintf(deleted_msg, sizeof(deleted_msg), "Index entry %d deleted.", id);
            send_response_to_client(msg->client_pid, deleted_msg);

            char log_msg[64];
            snprintf(log_msg, sizeof(log_msg), "ID=%d removido com sucesso", id);
            log_operation("DELETE", log_msg);
            free(entries);
        }

    } else if (strncmp(msg->operation, "SEARCH_PARALLEL|", 16) == 0) {
        char *copy = strdup(msg->operation + 16);
        char *word = strtok(copy, "|");
        char *n_str = strtok(NULL, "|");
    
        if (!word || !n_str) {
            send_response_to_client(msg->client_pid, "Erro: argumentos inválidos.");
            free(copy);
            exit(1);
        }
    
        int N = atoi(n_str);
        int total_entries = cache_size();
        if (N <= 0 || N > total_entries) {
            printf("%d %d\n ", N, total_entries);
            send_response_to_client(msg->client_pid, "Erro: número inválido de processos.");
            free(copy);
            exit(1);
        }
    
        // Dividir a cache em N blocos
        int entries_per_proc = total_entries / N;
        int resto = total_entries % N;
    
        int pipes[N][2];
        pid_t pids[N];
    
        for (int i = 0, start = 0; i < N; i++) {
            int count = entries_per_proc + (i < resto ? 1 : 0);
            int begin = start;
            start += count;
    
            if (pipe(pipes[i]) == -1) {
                perror("pipe");
                exit(0);
            }
    
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(0);
            }
    
            if (pid == 0) {
                // Processo filho
                close(pipes[i][0]); 
    
                for (int j = begin; j < begin + count; j++) {
                    const IndexEntry *entry = cache_get_by_index(j);
                    if (!entry || entry->year == -1) exit(0);
    
                    if (!entry->path || strlen(entry->path) == 0) exit(0);
                    int fd = open(entry->path, O_RDONLY);
                    if (fd == -1) exit(0);
    
                    char buf[1024];
                    ssize_t len = read(fd, buf, sizeof(buf) - 1);
                    close(fd);
                    if (len <= 0) exit(0);
                    buf[len] = '\0';
    
                    if (strstr(buf, word)) {
                        char id_buf[16];
                        snprintf(id_buf, sizeof(id_buf), "%d,", entry->id);
                        write(pipes[i][1], id_buf, strlen(id_buf));
                    }
                    
                }
    
                close(pipes[i][1]);
                exit(0);
            } else {
                pids[i] = pid;
                close(pipes[i][1]);  // processo pai fecha escrita
            }
        }

        // Processo pai lê todos os resultados
        char final_result[2048] = "";
        for (int i = 0; i < N; i++) {
            char buf[256];
            ssize_t n;
            while ((n = read(pipes[i][0], buf, sizeof(buf) - 1)) > 0) {
                buf[n] = '\0';
                strncat(final_result, buf, sizeof(final_result) - strlen(final_result) - 1);
            }
            close(pipes[i][0]);
            waitpid(pids[i], NULL, 0);  
        }
    
        size_t len = strlen(final_result);
        if (len > 0 && final_result[len - 1] == ',') {
            final_result[len - 1] = '\0';  
        }

        char formatted[2048];
        if (strlen(final_result) == 0) {
            strcpy(formatted, "[]");
        } else {
            snprintf(formatted, sizeof(formatted), "[%s]", final_result);
        }

        send_response_to_client(msg->client_pid, formatted);

        char log_msg[128];
        snprintf(log_msg, sizeof(log_msg), "Palavra='%s' com %d processo(s) filhos", word, N);
        log_operation("SEARCH_PARALLEL", log_msg);

        free(copy);

        
    } else if (strcmp(msg->operation, "STATS") == 0) {
        char response[256];
    
        int cache_current_size = cache_size();
        int disk_count = 0;
        IndexEntry *entries = NULL;
    
        if (storage_load_all(&entries, &disk_count) != 0) {
            strcpy(response, "Erro ao carregar índices do disco.");
        } else {
            snprintf(response, sizeof(response),
                "Tamanho atual da cache: %d\nNúmero total de índices no disco: %d",
                cache_current_size, disk_count);
            free(entries);
        }
    
        log_operation("STATS", "Consulta de estatísticas");
        send_response_to_client(msg->client_pid, response);

    } else if (strcmp(msg->operation, "SHUTDOWN") == 0) {

        printf("Servidor: encerramento solicitado pelo cliente %d\n", msg->client_pid);
        log_operation("SHUTDOWN", "Servidor encerrado via comando do cliente");
        cleanup();  
        exit(0);

    } else {
        printf("Servidor: operação não reconhecida: %s\n", msg->operation);
    }
}

int main(int argc, char *argv[]) {

    if (argc != 3) {
        fprintf(stderr, "Uso: %s <document_folder> <cache_size>\n", argv[0]);
        return 1;
    }

    const char *folder = argv[1];
    int max_cache_entries = atoi(argv[2]);

    if (create_server_fifo() != 0) return 1;
    if (storage_init(folder) != 0) return 1;
    if (cache_init(max_cache_entries) != 0) {
        fprintf(stderr, "Erro ao inicializar a cache\n");
        return 1;
    }

    signal(SIGINT, (void *)cleanup);  // Ctrl+C cleanup

    IndexEntry *indices = NULL;
    int total = 0;

    if (storage_load_all(&indices, &total) == 0) {
        printf("Servidor: %d índices carregados do disco.\n", total);
        for (int i = 0; i < total; i++) {
            if (indices[i].year != -1) {  // só adiciona se não estiver marcado como removido
            cache_add(&indices[i]);  // adicionar cada índice à cache
            printf("  [%d] %s (%d) - %s [%s]\n",
                   indices[i].id, indices[i].title, indices[i].year,
                   indices[i].authors, indices[i].path);
        }
    }
        free(indices);
    }

    printf("Servidor: à escuta em %s\n", SERVER_FIFO);

    while(1) {
        Message msg;
        if (receive_message_from_client(&msg) == 0) {
    
            pid_t pid = fork();
    
            if (pid < 0) {
                perror("Erro ao criar processo filho");
                continue;
            }
            if (pid == 0) {
                process_operation(&msg);
                exit(0);
            }
        }
    }

    return 0;
}
