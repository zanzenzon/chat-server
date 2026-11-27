CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
BIN = bin

all: $(BIN)/server $(BIN)/client $(BIN)/launcher

$(BIN)/server: src/server.c src/queue.c src/logger.c
	@mkdir -p $(BIN) logs

	$(CC) $(CFLAGS) -o $@ $^

$(BIN)/client: src/client.c
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN)/launcher: src/launcher.c
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(BIN)

.PHONY: all clean
