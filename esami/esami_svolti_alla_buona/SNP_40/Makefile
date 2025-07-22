# Makefile per il sistema stream client-server
# Compatibile con Linux e macOS usando libreria APUE

PROGS = server client

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

# Regola generica per compilazione
%: %.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $^ -o $@ $(LINK_FLAGS)

# Regole specifiche per chiarezza
server: server.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) server.c -o server $(LINK_FLAGS)

client: client.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) client.c -o client $(LINK_FLAGS)

# Pulizia dei binari
clean:
	rm -f $(PROGS) received_data.txt

# Target per test
test: all
	@echo "Programmi compilati con successo"
	@echo "Per eseguire:"
	@echo "1. Avviare il server: ./server"
	@echo "2. In un altro terminale: ./client"

.PHONY: all clean test