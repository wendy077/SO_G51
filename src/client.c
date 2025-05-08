#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ipc.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s -a \"Título\" \"Autores\" \"Ano\" \"Caminho\"\n", argv[0]);
        return 1;
    }

    Message msg;
    msg.client_pid = getpid();

    // Apenas comando -a nesta fase
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
    } else {
        fprintf(stderr, "Erro: operação desconhecida: %s\n", argv[1]);
        return 1;
    }

    return 0;
}
