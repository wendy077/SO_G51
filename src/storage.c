#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include "storage.h"

int storage_init() {
    int fd = open(INDEX_FILE, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("storage: erro ao criar ficheiro");
        return -1;
    }
    close(fd);
    return 0;
}

int storage_append_index(const IndexEntry *entry) {
    if (!entry) return -1;

    int fd = open(INDEX_FILE, O_WRONLY | O_APPEND);
    if (fd == -1) {
        perror("storage: erro ao abrir ficheiro para escrita");
        return -1;
    }

    ssize_t written = write(fd, entry, sizeof(IndexEntry));
    if (written != sizeof(IndexEntry)) {
        perror("storage: erro ao escrever entrada");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}
