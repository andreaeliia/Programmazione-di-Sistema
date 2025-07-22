# Makefile per i processi A e B
PROGS = processo_a processo_b

# Compilatore
CC = gcc
COMMON_FLAGS = -ansi -Wall -I./include
# sistema operativo
UNAME_S := $(shell uname)
# Flag per piattaforma
ifeq ($(UNAME_S), Linux)
	PLATFORM_FLAGS = -DLINUX -D_GNU_SOURCE
	LINK_FLAGS = -L./lib -lapueLinux -lpthread -lrt
else ifeq ($(UNAME_S), Darwin)
	PLATFORM_FLAGS = -DMACOS -D_DARWIN_SOURCE
	LINK_FLAGS = -L./lib -lapueMacOS -lpthread
endif

# Regola di default
all: $(PROGS)

%: %.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $^ -o $@ $(LINK_FLAGS)

# Pulizia dei binari e file temporanei
clean:
	rm -f $(PROGS)
	rm -f file_*.dat
	rm -f received_from_thread_*.dat

# Regola per test
test: all
	@echo "Per testare:"
	@echo "1. Avvia processo_a in un terminale"
	@echo "2. Avvia processo_b in un altro terminale"
	@echo "3. Inserisci il fd della pipe quando richiesto"

.PHONY: all clean test