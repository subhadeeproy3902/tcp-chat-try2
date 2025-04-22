# Detect OS
ifeq ($(OS),Windows_NT)
    PLATFORM = Windows
    CC = gcc
    CFLAGS = -Wall
    LDFLAGS = -lws2_32
    SERVER_OUT = s.exe
    CLIENT_OUT = c.exe
    RM = del /Q
else
    PLATFORM = Linux
    CC = gcc
    CFLAGS = -Wall
    LDFLAGS =
    SERVER_OUT = s.out
    CLIENT_OUT = c.out
    RM = rm -f
endif

# Targets
all: server client

server: server.c
	$(CC) $(CFLAGS) server.c -o $(SERVER_OUT) $(LDFLAGS)

client: client.c
	$(CC) $(CFLAGS) client.c -o $(CLIENT_OUT) $(LDFLAGS)

clean:
	$(RM) $(SERVER_OUT) $(CLIENT_OUT)

node-install:
	cd node && npm install

node-server:
	cd node && npm start

node-tcp:
	cd node && npm run start-tcp

.PHONY: all server client clean node-install node-server node-tcp
