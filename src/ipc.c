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
