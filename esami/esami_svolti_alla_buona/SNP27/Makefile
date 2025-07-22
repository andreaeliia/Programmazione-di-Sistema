# Makefile per Sistema IPC con Coda Circolare
# Compatibile Linux e macOS con libreria APUE

PROGS = packet_sender packet_receiver

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

# Regole specifiche per compilazione
packet_sender: packet_sender.c packet_protocol.h
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) packet_sender.c -o $@ $(LINK_FLAGS)

packet_receiver: packet_receiver.c packet_protocol.h
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) packet_receiver.c -o $@ $(LINK_FLAGS)

# Target per test completo
test:
	@echo "=== Test Sistema Pacchetti ==="
	@echo ""
	@echo "1. Avvia il receiver (terminal 1):"
	@echo "   ./packet_receiver"
	@echo ""
	@echo "2. Avvia il sender (terminal 2):"
	@echo "   ./packet_sender 50 100"
	@echo ""
	@echo "3. Osserva il flusso di pacchetti:"
	@echo "   - Receiver mostra pacchetti ricevuti e processati"
	@echo "   - Due thread consumer elaborano i pacchetti"
	@echo "   - Coda circolare gestisce il buffering"
	@echo ""
	@echo "4. Termina con Ctrl+C nel receiver"

# Target per demo veloce
demo: all
	@echo "=== Demo Automatica ==="
	@echo "Avviando receiver in background..."
	./packet_receiver &
	@sleep 2
	@echo "Inviando 20 pacchetti..."
	./packet_sender 20 200
	@sleep 3
	@echo "Demo completata"

# Target per test receiver
test-receiver: packet_receiver
	./packet_receiver

# Target per test sender
test-sender: packet_sender
	./packet_sender 10 500

# Pulizia dei binari e socket
clean:
	rm -f $(PROGS)
	rm -f /tmp/packet_socket

# Verifica header dipendenze
packet_protocol.h:
	@echo "Errore: file packet_protocol.h mancante!"
	@echo "Assicurati che sia presente nella directory corrente."
	@exit 1

# Target per help
help:
	@echo "Target disponibili:"
	@echo "  all            - Compila sender e receiver"
	@echo "  packet_sender  - Compila solo sender"
	@echo "  packet_receiver- Compila solo receiver"
	@echo "  test           - Mostra istruzioni test completo"
	@echo "  demo           - Esegue demo automatica"
	@echo "  test-receiver  - Avvia solo receiver"
	@echo "  test-sender    - Avvia sender con parametri test"
	@echo "  clean          - Rimuove binari e socket"
	@echo "  help           - Mostra questo messaggio"
	@echo ""
	@echo "File richiesti:"
	@echo "  packet_sender.c   - Codice sender"
	@echo "  packet_receiver.c - Codice receiver"
	@echo "  packet_protocol.h - Header protocolli condivisi"
	@echo ""
	@echo "Uso sender:"
	@echo "  ./packet_sender [num_packets] [delay_ms]"
	@echo "  Esempio: ./packet_sender 100 50"
	@echo ""
	@echo "Uso receiver:"
	@echo "  ./packet_receiver"

.PHONY: all test demo test-receiver test-sender clean help