# Diretórios
SRC_DIR = src
INC_DIR = include
BIN_DIR = bin

# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -I$(INC_DIR)

# Fontes e objetos
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:.c=.o)

# Binários
CLIENT_BIN = $(BIN_DIR)/dclient
SERVER_BIN = $(BIN_DIR)/dserver

# Objetos específicos
CLIENT_OBJ = $(SRC_DIR)/client.o $(SRC_DIR)/ipc.o
SERVER_OBJ = $(SRC_DIR)/server.o $(SRC_DIR)/ipc.o $(SRC_DIR)/index.o $(SRC_DIR)/storage.o

# Alvo por omissão
all: $(CLIENT_BIN) $(SERVER_BIN)

$(CLIENT_BIN): $(CLIENT_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(SERVER_BIN): $(SERVER_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Limpeza
clean:
	rm -f $(SRC_DIR)/*.o
	rm -rf $(BIN_DIR)

# Executa o servidor
run-server: $(SERVER_BIN)
	./$(SERVER_BIN)

# Executa o cliente com mensagem de teste
run-client: $(CLIENT_BIN)
	./$(CLIENT_BIN) "Mensagem de teste"

.PHONY: all clean run-server run-client
