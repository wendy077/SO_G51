#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ipc.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s -a \"Título\" \"Autores\" \"Ano\" \"Caminho\"\n", argv[0]);
        return 1;
    }

    Message msg;
    msg.client_pid = getpid();

    if (strcmp(argv[1], "-a") == 0) {
        if (argc != 6) {
            fprintf(stderr, "Erro: uso correto: %s -a \"Título\" \"Autores\" \"Ano\" \"Caminho\"\n", argv[0]);
            return 1;
        }

        const char *title = argv[2];
        const char *authors = argv[3];
        const char *year = argv[4];
        const char *path = argv[5];

        snprintf(msg.operation, MAX_MSG_SIZE, "ADD|%s|%s|%s|%s", title, authors, year, path);

        if (send_message_to_server(&msg) != 0) {
            fprintf(stderr, "Cliente: erro ao enviar mensagem ao servidor.\n");
            return 1;
        }

        printf("Cliente: pedido de indexação enviado com sucesso.\n");

    } else if (strcmp(argv[1], "-c") == 0 && argc == 3) {
        // Verifica se argv[2] é um número (ID)
        char *endptr;
        long id = strtol(argv[2], &endptr, 10);
    
        if (*endptr == '\0') {
            // É número → GET_META
            snprintf(msg.operation, MAX_MSG_SIZE, "GET_META|%ld", id);
        } else {
            // É palavra → SEARCH_CONTENT
            snprintf(msg.operation, MAX_MSG_SIZE, "SEARCH_CONTENT|%s", argv[2]);
        }
    
        if (create_client_fifo(msg.client_pid) != 0) {
            fprintf(stderr, "Erro ao criar FIFO do cliente.\n");
            return 1;
        }
    
        if (send_message_to_server(&msg) != 0) {
            remove_client_fifo(msg.client_pid);
            return 1;
        }
    
        char *fifo_name = get_client_fifo_name(msg.client_pid);
        int fd = open(fifo_name, O_RDONLY);
        if (fd == -1) {
            perror("Erro ao abrir FIFO do cliente");
            remove_client_fifo(msg.client_pid);
            return 1;
        }
    
        char buffer[2048];
        ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            if (*endptr == '\0') {
                printf("Meta-informação:\n%s\n", buffer);
            } else {
                printf("Resultado da pesquisa por conteúdo:\n%s\n", buffer);
            }
        } else {
            printf("Sem resultados.\n");
        }
    
        close(fd);
        remove_client_fifo(msg.client_pid);

    } else if (strcmp(argv[1], "-s") == 0 && argc == 4) {
        
        if (create_client_fifo(msg.client_pid) != 0) {
            fprintf(stderr, "Erro ao criar FIFO do cliente.\n");
            return 1;
        }
    
        snprintf(msg.operation, MAX_MSG_SIZE, "SEARCH_PARALLEL|%s|%s", argv[2], argv[3]);
    
        if (send_message_to_server(&msg) != 0) {
            remove_client_fifo(msg.client_pid);
            return 1;
        }
    
        char *fifo_name = get_client_fifo_name(msg.client_pid);
        int fd = open(fifo_name, O_RDONLY);
        if (fd == -1) {
            perror("Erro ao abrir FIFO do cliente");
            remove_client_fifo(msg.client_pid);
            return 1;
        }
    
        char buffer[2048];
        ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("Resultado da pesquisa concorrente:\n%s\n", buffer);
        }
    
        close(fd);
        remove_client_fifo(msg.client_pid);

    } else if (strcmp(argv[1], "-s") == 0) {

        if (argc != 3) {
            fprintf(stderr, "Uso: %s -s campo=valor\n", argv[0]);
            return 1;
        }

        if (create_client_fifo(msg.client_pid) != 0) {
            fprintf(stderr, "Erro ao criar FIFO do cliente.\n");
            return 1;
        }

        snprintf(msg.operation, MAX_MSG_SIZE, "SEARCH|%s", argv[2]);

        if (send_message_to_server(&msg) != 0) {
            fprintf(stderr, "Erro ao enviar mensagem ao servidor.\n");
            remove_client_fifo(msg.client_pid);
            return 1;
        }

        // Esperar resposta
        char *fifo_name = get_client_fifo_name(msg.client_pid);
        int fd = open(fifo_name, O_RDONLY);
        if (fd == -1) {
            perror("Erro ao abrir FIFO do cliente para leitura");
            remove_client_fifo(msg.client_pid);
            return 1;
        }

        char buffer[1024];
        ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("Resultado da pesquisa:\n%s\n", buffer);
        } else {
            printf("Sem resultados.\n");
        }

        close(fd);
        remove_client_fifo(msg.client_pid);
        
    } else if (strcmp(argv[1], "-c") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Uso: %s -c <palavra-chave>\n", argv[0]);
            return 1;
        }
    
        if (create_client_fifo(msg.client_pid) != 0) {
            fprintf(stderr, "Erro ao criar FIFO do cliente.\n");
            return 1;
        }
    
        snprintf(msg.operation, MAX_MSG_SIZE, "SEARCH_CONTENT|%s", argv[2]);
    
        if (send_message_to_server(&msg) != 0) {
            remove_client_fifo(msg.client_pid);
            return 1;
        }
    
        // Esperar e imprimir resposta
        char *fifo_name = get_client_fifo_name(msg.client_pid);
        int fd = open(fifo_name, O_RDONLY);
        if (fd == -1) {
            perror("Erro ao abrir FIFO do cliente");
            remove_client_fifo(msg.client_pid);
            return 1;
        }
    
        char buffer[1024];
        ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("Resultado da pesquisa por conteúdo:\n%s\n", buffer);
        } else {
            printf("Sem resultados.\n");
        }
    
        close(fd);
        remove_client_fifo(msg.client_pid);

    } else if (strcmp(argv[1], "-l") == 0 && argc == 4) {
        if (create_client_fifo(msg.client_pid) != 0) {
            fprintf(stderr, "Erro ao criar FIFO do cliente.\n");
            return 1;
        }
    
        snprintf(msg.operation, MAX_MSG_SIZE, "COUNT_LINES|%s|%s", argv[2], argv[3]);
    
        if (send_message_to_server(&msg) != 0) {
            remove_client_fifo(msg.client_pid);
            return 1;
        }
    
        char *fifo_name = get_client_fifo_name(msg.client_pid);
        int fd = open(fifo_name, O_RDONLY);
        if (fd == -1) {
            perror("Erro ao abrir FIFO do cliente");
            remove_client_fifo(msg.client_pid);
            return 1;
        }
    
        char buffer[256];
        ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("%s\n", buffer);
        }
    
        close(fd);
        remove_client_fifo(msg.client_pid);

    } else if (strcmp(argv[1], "-f") == 0) {

        snprintf(msg.operation, MAX_MSG_SIZE, "SHUTDOWN");

        if (send_message_to_server(&msg) != 0) {
            fprintf(stderr, "Erro ao enviar pedido de encerramento.\n");
            return 1;
        }

        printf("Pedido de encerramento enviado ao servidor.\n");

    } else {
        fprintf(stderr, "Erro: operação desconhecida: %s\n", argv[1]);
        return 1;
    }

    return 0;
}
