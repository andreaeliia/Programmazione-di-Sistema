# Makefile per Scanner Inode Ricorsivo
# Compatibile con Linux e Mac

PROGS = inode_scanner

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

# Regola specifica per inode_scanner
inode_scanner: inode_scanner.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $^ -o $@ $(LINK_FLAGS)

# Test del programma
test: inode_scanner
	@echo "=== Testing Inode Scanner ==="
	@echo "Test con directory corrente:"
	./inode_scanner .
	@echo ""
	@echo "Test con directory /tmp:"
	@if [ -d /tmp ]; then \
		./inode_scanner /tmp; \
	else \
		echo "Directory /tmp non disponibile"; \
	fi

# Test con directory di esempio
test-examples: inode_scanner
	@echo "=== Test con Directory di Esempio ==="
	@echo "Creazione directory di test..."
	@mkdir -p test_dir/subdir1/subdir2
	@echo "file1" > test_dir/file1.txt
	@echo "file2" > test_dir/file2.txt
	@echo "file3" > test_dir/subdir1/file3.txt
	@echo "file4" > test_dir/subdir1/subdir2/file4.txt
	@echo ""
	@echo "Scansione directory di test:"
	./inode_scanner test_dir
	@echo ""
	@echo "Pulizia directory di test..."
	@rm -rf test_dir

# Test con varie directory di sistema
test-system: inode_scanner
	@echo "=== Test con Directory di Sistema ==="
	@echo "Test con /etc (primi 10 risultati):"
	@if [ -d /etc ]; then \
		./inode_scanner /etc | head -15; \
	else \
		echo "Directory /etc non accessibile"; \
	fi
	@echo ""
	@echo "Test con directory home utente:"
	@if [ -d "$$HOME" ]; then \
		echo "Scansione $$HOME (primi 10 risultati):"; \
		./inode_scanner "$$HOME" | head -15; \
	else \
		echo "Directory home non disponibile"; \
	fi

# Benchmark di prestazioni
benchmark: inode_scanner
	@echo "=== Benchmark Prestazioni ==="
	@echo "Tempo di esecuzione su directory corrente:"
	time ./inode_scanner .

# Verifica sintassi e warning
check: inode_scanner.c
	@echo "=== Controllo Sintassi ==="
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) -fsyntax-only $^
	@echo "Sintassi corretta!"

# Verifica memoria con valgrind (se disponibile)
memcheck: inode_scanner
	@if command -v valgrind >/dev/null 2>&1; then \
		echo "=== Memory Check con Valgrind ==="; \
		valgrind --tool=memcheck --leak-check=full ./inode_scanner .; \
	else \
		echo "Valgrind non disponibile per memory check"; \
		echo "Esecuzione normale:"; \
		./inode_scanner .; \
	fi

# Mostra informazioni di sistema
info:
	@echo "Sistema operativo: $(UNAME_S)"
	@echo "Compilatore: $(CC)"
	@echo "Flag comuni: $(COMMON_FLAGS)"
	@echo "Flag piattaforma: $(PLATFORM_FLAGS)"
	@echo "Flag linking: $(LINK_FLAGS)"
	@echo "Programmi: $(PROGS)"
	@echo "Directory corrente: $$(pwd)"
	@echo "Spazio disponibile: $$(df -h . | tail -1)"

# Aiuto sull'utilizzo
help:
	@echo "Target disponibili:"
	@echo "  all            - Compila il programma"
	@echo "  test           - Test base con directory . e /tmp"
	@echo "  test-examples  - Test con directory di esempio creata"
	@echo "  test-system    - Test con directory di sistema"
	@echo "  benchmark      - Misura prestazioni"
	@echo "  check          - Verifica sintassi"
	@echo "  memcheck       - Controllo memoria con valgrind"
	@echo "  info           - Informazioni di sistema"
	@echo "  help           - Mostra questo aiuto"
	@echo "  clean          - Rimuove file compilati"
	@echo ""
	@echo "Esempi di utilizzo manuale:"
	@echo "  ./inode_scanner /home/user"
	@echo "  ./inode_scanner ."
	@echo "  ./inode_scanner /usr/local"

# Pulizia dei binari
clean:
	rm -f $(PROGS)
	rm -rf test_dir
	rm -f core
	rm -f *.dSYM

# Pulizia completa
distclean: clean
	rm -f *~

.PHONY: all test test-examples test-system benchmark check memcheck info help clean distclean