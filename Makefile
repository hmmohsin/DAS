CC = gcc
CFLAGS = -c -Wall -pthread -lm -lrt
LDFLAGS = -pthread -lm -lrt -lncurses
TARGETS = client server
CLIENT_DIR = src/client
SERVER_DIR = src/server
BIN_DIR = bin/

_CLIENT_OBJS = client.o config_parser.o load_generator.o loader.o dispatcher.o queue.o common.o communication.o res_compiler.o monitor.o file_metadata.o

_SERVER_OBJS = main.o server.o scheduler.o uprio.o queue.o config_parser.o communication.o helper.o

SERVER_OBJS = $(patsubst %,$(SERVER_DIR)/%,$(_SERVER_OBJS))
CLIENT_OBJS = $(patsubst %,$(CLIENT_DIR)/%,$(_CLIENT_OBJS))

#$(info $$CLIENT_DIR is [${CLIENT_DIR}])
#$(info $$SERVER_DIR is [${SERVER_DIR}])

all: $(TARGETS)

server: $(SERVER_OBJS)
	$(CC) $(SERVER_OBJS) -o server $(LDFLAGS)
	mv server $(BIN_DIR)

client: $(CLIENT_OBJS)
	$(CC) $(CLIENT_OBJS) -o client $(LDFLAGS)
	mv client $(BIN_DIR)

%.o: $(SERVER_DIR)/%.c
	$(CC) $(CFLAGS) $^ -o $@

%.o: $(CLIENT_DIR)/%.c
	$(CC) $(CFLAGS) $^ -o $@
clean:
	rm -rf $(SERVER_DIR)/*.o $(CLIENT_DIR)/*.o
