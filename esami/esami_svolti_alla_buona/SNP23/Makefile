# Makefile per Server TCP Multi-Thread
# Compatibile Linux e macOS con libreria APUE

PROGS = tcp_server tcp_client

# Compilatore
CC = gcc
COMMON_FLAGS = -ansi -Wall -I./include

# Sistema operativo
UNAME_S := $(shell uname)

# Flag per piattaforma
ifeq ($(UNAME_S), Linux)
	PLATFORM_FLAGS = -DLINUX -D_GNU_SOURCE
	LINK_FLAGS = -L./lib -lapueLinux -lpthread
else ifeq ($(UNAME_S), Darwin)
	PLATFORM_FLAGS = -DMACOS -D_DARWIN_SOURCE
	LINK_FLAGS = -L./lib -lapueMacOS -lpthread
endif

# Regola di default
all: $(PROGS)

# Regole per compilazione
tcp_server: tcp_server.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $^ -o $@ $(LINK_FLAGS)

tcp_client: tcp_client.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $^ -o $@ $(LINK_FLAGS)

# Target per test completo
test: all
	@echo "=== Starting test ==="
	@echo "1. Start server in background: ./tcp_server 8001 8002 8003"
	@echo "2. Run test clients:"
	@echo "   ./tcp_client localhost 8001 100"
	@echo "   ./tcp_client localhost 8002 200"
	@echo "   ./tcp_client localhost 8003 300"
	@echo "3. Stop server with Ctrl+C"

# Target per avvio server di test
test-server: tcp_server
	./tcp_server 8001 8002 8003

# Target per test client singolo
test-client: tcp_client
	./tcp_client localhost 8001 42

# Pulizia dei binari
clean:
	rm -f $(PROGS)

# Target per help
help:
	@echo "Target disponibili:"
	@echo "  all         - Compila server e client"
	@echo "  tcp_server  - Compila solo il server"
	@echo "  tcp_client  - Compila solo il client"
	@echo "  test        - Mostra istruzioni per test"
	@echo "  test-server - Avvia server con porte di default"
	@echo "  test-client - Test client con parametri di default"
	@echo "  clean       - Rimuove binari"
	@echo "  help        - Mostra questo messaggio"
	@echo ""
	@echo "Uso server:"
	@echo "  ./tcp_server <porta1> <porta2> <porta3>"
	@echo ""
	@echo "Uso client:"
	@echo "  ./tcp_client <hostname> <porta> <valore>"

.PHONY: all test test-server test-client clean help