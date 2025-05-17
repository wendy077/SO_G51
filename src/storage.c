#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>    
#include "storage.h"

#define INDEX_FILE "data/index.dat"

static char base_folder[256] = "";  // pasta onde estão os documentos


void storage_set_folder(const char *folder) {
    strncpy(base_folder, folder, sizeof(base_folder) - 1);
    base_folder[sizeof(base_folder) - 1] = '\0';
}

int storage_init(const char *folder) {
    storage_set_folder(folder);

    int fd = open(INDEX_FILE, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("open index.dat");
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

    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size <= 0) {
        *entries = NULL;
        *count = 0;
        close(fd);
        return 0;
    }
    lseek(fd, 0, SEEK_SET);

    int total_entries = file_size / sizeof(IndexEntry);
    IndexEntry *all = malloc(file_size);
    if (!all) {
        perror("malloc");
        close(fd);
        return -1;
    }

    if (read(fd, all, file_size) != file_size) {
        perror("read");
        free(all);
        close(fd);
        return -1;
    }
    close(fd);

    // Filtrar apenas entradas com year != -1
    int valid_count = 0;
    for (int i = 0; i < total_entries; i++) {
        if (all[i].year != -1) valid_count++;
    }

    *entries = malloc(valid_count * sizeof(IndexEntry));
    if (!*entries) {
        perror("malloc");
        free(all);
        return -1;
    }

    int j = 0;
    for (int i = 0; i < total_entries; i++) {
        if (all[i].year != -1) {
            (*entries)[j++] = all[i];
        }
    }

    *count = valid_count;
    free(all);
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

void storage_resolve_path(const char *relative, char *resolved, size_t size) {
    snprintf(resolved, size, "%s/%s", base_folder, relative);
}  
