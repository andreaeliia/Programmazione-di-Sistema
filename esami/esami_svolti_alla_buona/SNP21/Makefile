# Makefile per IPC Benchmark
# Compatibile Linux e macOS con libreria APUE

PROGS = ipc_benchmark

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

# Regola per compilazione
%: %.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $^ -o $@ $(LINK_FLAGS)

# Pulizia dei binari
clean:
	rm -f $(PROGS)
	rm -f /tmp/benchmark_fifo /tmp/benchmark_mmap

# Target per eseguire il test
test: ipc_benchmark
	./ipc_benchmark

# Target per help
help:
	@echo "Target disponibili:"
	@echo "  all      - Compila tutti i programmi"
	@echo "  clean    - Rimuove binari e file temporanei"
	@echo "  test     - Compila ed esegue il benchmark"
	@echo "  help     - Mostra questo messaggio"

.PHONY: all clean test help