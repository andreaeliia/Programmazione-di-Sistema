# Makefile per Controllo Lampadine
# Compatibile con Linux e Mac

PROGS = semaphore_version record_locking_version

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

%: %.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $^ -o $@ $(LINK_FLAGS)

# Regole specifiche (opzionali se si vuole personalizzare)
semaphore_version: semaphore_version.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $^ -o $@ $(LINK_FLAGS)

record_locking_version: record_locking_version.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $^ -o $@ $(LINK_FLAGS)

# Test entrambe le versioni
test: all
	@echo "=== Testing Semaphore Version ==="
	./semaphore_version
	@echo ""
	@echo "=== Testing Record Locking Version ==="
	./record_locking_version

# Pulizia dei binari
clean:
	rm -f $(PROGS)
	rm -f *.lock *.dat
	rm -f core

# Pulizia completa
distclean: clean
	rm -f *~

# Mostra informazioni di sistema
info:
	@echo "Sistema operativo: $(UNAME_S)"
	@echo "Compilatore: $(CC)"
	@echo "Flag comuni: $(COMMON_FLAGS)"
	@echo "Flag piattaforma: $(PLATFORM_FLAGS)"
	@echo "Flag linking: $(LINK_FLAGS)"
	@echo "Programmi: $(PROGS)"

.PHONY: all test clean distclean info