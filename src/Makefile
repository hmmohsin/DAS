CC = gcc
DEPS = server.h uprio.h queue.h
CFLAGS = -c -Wall -pthread -lm -lrt
LDFLAGS = -pthread -lm -lrt -lncurses
TARGETS = client server
CLIENT_OBJS = client.o config_parser.o load_generator.o loader.o dispatcher.o queue.o common.o communication.o res_compiler.o monitor.o file_metadata.o
SERVER_OBJS = main.o server.o scheduler.o uprio.o queue.o noise.o config_parser.o communication.o helper.o
BIN_DIR = bin
CLIENT_DIR = src/client
SERVER_DIR = src/server

all: $(TARGETS) move

move:
	mkdir -p $(BIN_DIR)
	mv *.o $(TARGETS) $(BIN_DIR)

client: $(CLIENT_OBJS)
	$(CC) $(CLIENT_OBJS) -o client $(LDFLAGS)

server: $(SERVER_OBJS)
	$(CC) $(SERVER_OBJS) -o server $(LDFLAGS)

%.o: $(CLIENT_DIR)/%.c
	$(CC) $(CFLAGS) $^ -o $@

%.o: $(SERVER_DIR)/%.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(BIN_DIR)/*
