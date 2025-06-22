# Makefile per esercizio TCP Server/Client

CC = gcc
CFLAGS = -g -Wall -I/usr/local/include -D_GNU_SOURCE
LDFLAGS = -L/usr/local/lib
LDLIBS = -lapue -lpthread

# Programmi da compilare
PROGS = server client

# Regola principale
all: $(PROGS)

# Server TCP
server: server.c
	$(CC) $(CFLAGS) -o server server.c $(LDFLAGS) $(LDLIBS)

# Client TCP multi-thread
client: client.c  
	$(CC) $(CFLAGS) -o client client.c $(LDFLAGS) $(LDLIBS)

# Test: avvia server in background
test: all
	@echo "Avvio server in background..."
	./server &
	@echo "Aspetto 2 secondi..."
	sleep 2
	@echo "Avvio client..."
	./client
	@echo "Termino server..."
	pkill -f "./server" || true

# Pulizia
clean:
	rm -f $(PROGS)

# Debug con gdb
debug-server: server
	gdb ./server

debug-client: client
	gdb ./client

# Informazioni
help:
	@echo "Comandi disponibili:"
	@echo "  make all     - Compila server e client"
	@echo "  make server  - Compila solo server"
	@echo "  make client  - Compila solo client"
	@echo "  make test    - Test automatico"
	@echo "  make clean   - Rimuove eseguibili"
	@echo ""
	@echo "Uso manuale:"
	@echo "  Terminal 1: ./server"
	@echo "  Terminal 2: ./client"

.PHONY: all clean test debug-server debug-client help