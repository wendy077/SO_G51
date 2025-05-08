#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "ipc.h"
#include "index.h"
#include "storage.h"

void cleanup() {
    unlink(SERVER_FIFO);
    printf("\nServidor: FIFO removido.\n");
}

int main() {
    if (create_server_fifo() != 0) return 1;
    if (storage_init() != 0) return 1;

    printf("Servidor: à escuta em %s\n", SERVER_FIFO);
    signal(SIGINT, (void *)cleanup);  // Ctrl+C cleanup

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
                }

                free(entry);
            } else {
                printf("Servidor: operação não reconhecida: %s\n", msg.operation);
            }
        }
    }

    return 0;
}
