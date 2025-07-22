# Makefile per gli esercizi di programmazione di sistema
# Supporta Linux e macOS con libreria APUE

PROGS = esercizio1 esercizio2 target_program

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

# Regola generica per compilare i programmi
%: %.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $^ -o $@ $(LINK_FLAGS)

# Target specifico per l'esercizio 2 (richiede libm per sqrt)
esercizio2: esercizio2.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $^ -o $@ $(LINK_FLAGS) -lm

# Pulizia dei binari e file temporanei
clean:
	rm -f $(PROGS) shared_file.tmp

# Target per eseguire gli esercizi
run1: esercizio1
	./esercizio1

run2: esercizio2 target_program
	./esercizio2

# Target per il test completo
test: all
	@echo "=== Test Esercizio 1 ==="
	./esercizio1
	@echo ""
	@echo "=== Test Esercizio 2 ==="
	./esercizio2

.PHONY: all clean run1 run2 test