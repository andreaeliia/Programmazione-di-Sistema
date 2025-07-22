# Makefile per Sistema Multiplayer su Matrice 200x200
# Compatibile Linux e macOS con libreria APUE

PROGS = game_server game_client

# Compilatore
CC = gcc
COMMON_FLAGS = -ansi -Wall -I./include

# Sistema operativo
UNAME_S := $(shell uname)

# Flag per piattaforma
ifeq ($(UNAME_S), Linux)
	PLATFORM_FLAGS = -DLINUX -D_GNU_SOURCE
	LINK_FLAGS = -L./lib -lapueLinux
else ifeq ($(UNAME_S), Darwin)
	PLATFORM_FLAGS = -DMACOS -D_DARWIN_SOURCE
	LINK_FLAGS = -L./lib -lapueMacOS
endif

# Regola di default
all: $(PROGS)

# Regole specifiche per compilazione
game_server: game_server.c game_protocol.h
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) game_server.c -o $@ $(LINK_FLAGS)

game_client: game_client.c game_protocol.h
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) game_client.c -o $@ $(LINK_FLAGS)

# Target per test completo (istruzioni)
test:
	@echo "=== Test del Sistema Multiplayer ==="
	@echo ""
	@echo "1. Avvia il server:"
	@echo "   ./game_server"
	@echo ""
	@echo "2. Avvia client multipli (terminali separati):"
	@echo "   ./game_client Player1"
	@echo "   ./game_client Player2 192.168.1.100"
	@echo "   ./game_client Player3"
	@echo ""
	@echo "3. Controlli nel client:"
	@echo "   u = su    n = giù"
	@echo "   h = sinistra    j = destra"
	@echo "   q = quit"
	@echo ""
	@echo "4. Obiettivo:"
	@echo "   Elimina gli altri giocatori scontrandoti con loro!"

# Target per avvio server
run-server: game_server
	./game_server

# Target per test client locale
run-client: game_client
	./game_client TestPlayer

# Pulizia dei binari
clean:
	rm -f $(PROGS)

# Target per creazione header se necessario
game_protocol.h:
	@echo "Errore: file game_protocol.h mancante!"
	@echo "Assicurati che sia presente nella directory corrente."
	@exit 1

# Target per help
help:
	@echo "Target disponibili:"
	@echo "  all         - Compila server e client"
	@echo "  game_server - Compila solo il server"
	@echo "  game_client - Compila solo il client"
	@echo "  test        - Mostra istruzioni per test completo"
	@echo "  run-server  - Avvia server"
	@echo "  run-client  - Avvia client di test"
	@echo "  clean       - Rimuove binari"
	@echo "  help        - Mostra questo messaggio"
	@echo ""
	@echo "File richiesti:"
	@echo "  game_server.c   - Codice server"
	@echo "  game_client.c   - Codice client"
	@echo "  game_protocol.h - Header protocolli condivisi"
	@echo ""
	@echo "Uso server:"
	@echo "  ./game_server"
	@echo ""
	@echo "Uso client:"
	@echo "  ./game_client <nickname> [server_ip]"

.PHONY: all test run-server run-client clean help