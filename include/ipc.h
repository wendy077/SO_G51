#ifndef IPC_H
#define IPC_H

#define SERVER_FIFO "/tmp/dserver_fifo"
#define CLIENT_FIFO_TEMPLATE "/tmp/client_%d_fifo"
#define MAX_MSG_SIZE 512

typedef struct {
    pid_t client_pid;
    char operation[MAX_MSG_SIZE];
} Message;

int create_server_fifo();
int send_message_to_server(const Message *msg);
int receive_message_from_client(Message *msg);
char *get_client_fifo_name(pid_t pid);
int create_client_fifo(pid_t pid);
void remove_client_fifo(pid_t pid);
int send_response_to_client(pid_t pid, const char *response);
char *receive_response_from_server();


#endif
