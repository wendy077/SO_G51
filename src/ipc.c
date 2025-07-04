#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "ipc.h"

int create_server_fifo() {
    if (access(SERVER_FIFO, F_OK) == -1) {
        if (mkfifo(SERVER_FIFO, 0666) == -1) {
            perror("mkfifo SERVER_FIFO");
            return -1;
        }
    }
    return 0;
}

char *get_client_fifo_name(pid_t pid) {
    static char name[64];
    snprintf(name, sizeof(name), CLIENT_FIFO_TEMPLATE, pid);
    return name;
}

int send_message_to_server(const Message *msg) {
    int fd = open(SERVER_FIFO, O_WRONLY);
    if (fd == -1) {
        perror("open SERVER_FIFO");
        return -1;
    }
    if (write(fd, msg, sizeof(Message)) != sizeof(Message)) {
        perror("write to SERVER_FIFO");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

int receive_message_from_client(Message *msg) {
    int fd = open(SERVER_FIFO, O_RDONLY);
    if (fd == -1) {
        perror("open SERVER_FIFO");
        return -1;
    }
    if (read(fd, msg, sizeof(Message)) != sizeof(Message)) {
        perror("read from SERVER_FIFO");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

int create_client_fifo(pid_t pid) {
    char *fifo_name = get_client_fifo_name(pid);
    if (mkfifo(fifo_name, 0666) == -1) {
        perror("mkfifo CLIENT_FIFO");
        return -1;
    }
    return 0;
}

void remove_client_fifo(pid_t pid) {
    char *fifo_name = get_client_fifo_name(pid);
    unlink(fifo_name);
}

int send_response_to_client(pid_t pid, const char *response) {
    char *fifo_name = get_client_fifo_name(pid);
    int fd = open(fifo_name, O_WRONLY);
    if (fd == -1) {
        perror("open client FIFO to send response");
        return -1;
    }

    if (write(fd, response, strlen(response)) == -1) {
        perror("write to client FIFO");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

char *receive_response_from_server() {
    char *fifo_name = get_client_fifo_name(getpid());
    int fd = open(fifo_name, O_RDONLY);
    if (fd == -1) {
        perror("open client FIFO to receive response");
        return NULL;
    }

    char *buffer = malloc(1024);
    if (!buffer) {
        perror("malloc");
        close(fd);
        return NULL;
    }

    ssize_t len = read(fd, buffer, 1023);
    if (len == -1) {
        perror("read from client FIFO");
        close(fd);
        free(buffer);
        return NULL;
    }

    buffer[len] = '\0';  // garantir que termina em null
    close(fd);
    return buffer;
}
