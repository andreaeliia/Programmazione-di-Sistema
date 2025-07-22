# Makefile per Network Data Collector
# Sistema multi-thread con condition variable

PROGS = network_data_collector

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

# Regola specifica per compilazione
%: %.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $^ -o $@ $(LINK_FLAGS)

# Target per test con parametri di default
test: network_data_collector
	./network_data_collector

# Target per test rapido (5 secondi di intervallo)
test-fast: network_data_collector
	./network_data_collector 5 test_responses.txt

# Target per test con servizio UUID
test-uuid: network_data_collector
	./network_data_collector 10 uuid_responses.txt "httpbin.org/uuid"

# Target per test con servizio time
test-time: network_data_collector
	./network_data_collector 8 time_responses.txt "httpbin.org/now"

# Pulizia dei binari
clean:
	rm -f $(PROGS)
	rm -f *.txt

# Target per help
help:
	@echo "Target disponibili:"
	@echo "  all           - Compila il programma"
	@echo "  test          - Esegue test con parametri default"
	@echo "  test-fast     - Test con intervallo 5 secondi"
	@echo "  test-uuid     - Test con servizio UUID"
	@echo "  test-time     - Test con servizio time"
	@echo "  clean         - Rimuove binari e file di output"
	@echo "  help          - Mostra questo messaggio"
	@echo ""
	@echo "Uso manuale:"
	@echo "  ./network_data_collector [intervallo] [file] [url]"
	@echo ""
	@echo "Esempi:"
	@echo "  ./network_data_collector 10 responses.txt"
	@echo "  ./network_data_collector 5 data.txt \"httpbin.org/uuid\""

.PHONY: all test test-fast test-uuid test-time clean help