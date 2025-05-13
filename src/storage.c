#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>    
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


int storage_load_all(IndexEntry **entries, int *count) {
    int fd = open(INDEX_FILE, O_RDONLY);
    if (fd == -1) {
        perror("storage: erro ao abrir ficheiro para leitura");
        return -1;
    }

    // Determinar o tamanho do ficheiro
    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1) {
        perror("lseek");
        close(fd);
        return -1;
    }
    lseek(fd, 0, SEEK_SET);

    int total_entries = file_size / sizeof(IndexEntry);
    *count = total_entries;

    if (total_entries == 0) {
        *entries = NULL;
        close(fd);
        return 0;
    }

    *entries = malloc(file_size);
    if (!*entries) {
        perror("malloc");
        close(fd);
        return -1;
    }

    ssize_t bytes_read = read(fd, *entries, file_size);
    if (bytes_read != file_size) {
        perror("read");
        free(*entries);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int storage_get_max_id() {
    int fd = open(INDEX_FILE, O_RDONLY);
    if (fd == -1) return 0;

    IndexEntry entry;
    int max_id = 0;

    while (read(fd, &entry, sizeof(IndexEntry)) == sizeof(IndexEntry)) {
        if (entry.year != -1 && entry.id > max_id) {
            max_id = entry.id;
        }
    }

    close(fd);
    return max_id;
}

