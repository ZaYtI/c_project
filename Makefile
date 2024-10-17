
CLIENT_SRC = client.c
SERVER_SRC = server.c

CLIENT_EXEC = client
SERVER_EXEC = server

CC = gcc
CFLAGS = -Wall -g

all: $(CLIENT_EXEC) $(SERVER_EXEC)

$(CLIENT_EXEC): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_EXEC) $(CLIENT_SRC)

$(SERVER_EXEC): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER_EXEC) $(SERVER_SRC)

clean:
	rm -f $(CLIENT_EXEC) $(SERVER_EXEC)

.PHONY: all clean
