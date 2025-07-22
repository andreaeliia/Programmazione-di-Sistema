PROGS = file_monitor

# Compilatore
CC = gcc
COMMON_FLAGS = -ansi -Wall -I./include -pthread

# Sistema operativo
UNAME_S := $(shell uname)

# Flag per piattaforma
ifeq ($(UNAME_S), Linux)
	PLATFORM_FLAGS = -DLINUX -D_GNU_SOURCE
	LINK_FLAGS = -L./lib -lapueLinux -pthread
else ifeq ($(UNAME_S), Darwin)
	PLATFORM_FLAGS = -DMACOS -D_DARWIN_SOURCE
	LINK_FLAGS = -L./lib -lapueMacOS -pthread
endif

# Regola di default
all: $(PROGS)

%: %.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $^ -o $@ $(LINK_FLAGS)

# Pulizia dei binari
clean:
	rm -f $(PROGS)

# Regola per creare il file dump.txt di test
test_data:
	@echo "Creazione file di test..."
	@bash -c 'while true; do openssl rand 1000 >> dump.txt; sleep 1; done' &
	@echo "Script di generazione dati avviato in background"

# Ferma tutti i processi che scrivono su dump.txt
stop_test:
	@pkill -f "openssl rand" || true
	@echo "Processi di test fermati"

.PHONY: all clean test_data stop_test