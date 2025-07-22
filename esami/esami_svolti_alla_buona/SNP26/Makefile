# Makefile per Calcolo Parallelo Mantisse
# Compatibile con Linux e Mac

PROGS = mantissa_calculator

# Compilatore
CC = gcc
COMMON_FLAGS = -ansi -Wall -I./include

# Sistema operativo
UNAME_S := $(shell uname)

# Flag per piattaforma
ifeq ($(UNAME_S), Linux)
PLATFORM_FLAGS = -DLINUX -D_GNU_SOURCE
LINK_FLAGS = -L./lib -lapueLinux -lpthread -lm
else ifeq ($(UNAME_S), Darwin)
PLATFORM_FLAGS = -DMACOS -D_DARWIN_SOURCE
LINK_FLAGS = -L./lib -lapueMacOS -lpthread -lm
endif

# Regola di default
all: $(PROGS)

%: %.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $^ -o $@ $(LINK_FLAGS)

# Regola specifica per mantissa_calculator
mantissa_calculator: mantissa_calculator.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $^ -o $@ $(LINK_FLAGS)

# Test del programma
test: mantissa_calculator
	@echo "=== Testing Mantissa Calculator ==="
	./mantissa_calculator

# Test rapido con meno numeri (per debug)
test-quick: mantissa_calculator
	@echo "=== Quick Test (modificare MAX_NUMBERS nel codice) ==="
	./mantissa_calculator

# Benchmark con diverse configurazioni
benchmark: mantissa_calculator
	@echo "=== Benchmark Completo ==="
	time ./mantissa_calculator

# Mostra informazioni di sistema
info:
	@echo "Sistema operativo: $(UNAME_S)"
	@echo "Compilatore: $(CC)"
	@echo "Flag comuni: $(COMMON_FLAGS)"
	@echo "Flag piattaforma: $(PLATFORM_FLAGS)"
	@echo "Flag linking: $(LINK_FLAGS)"
	@echo "Programmi: $(PROGS)"
	@echo "Thread supportati: pthread"
	@echo "Librerie matematiche: libm"

# Controllo memoria (se valgrind disponibile)
memcheck: mantissa_calculator
	@if command -v valgrind >/dev/null 2>&1; then \
		echo "=== Memory Check con Valgrind ==="; \
		valgrind --tool=memcheck --leak-check=full ./mantissa_calculator; \
	else \
		echo "Valgrind non disponibile per memory check"; \
	fi

# Profiling (se gprof disponibile)
profile: mantissa_calculator.c
	@echo "=== Compilazione per Profiling ==="
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) -pg $^ -o mantissa_calculator_prof $(LINK_FLAGS)
	@echo "Eseguire: ./mantissa_calculator_prof && gprof mantissa_calculator_prof gmon.out > profile.txt"

# Pulizia dei binari
clean:
	rm -f $(PROGS)
	rm -f mantissa_calculator_prof
	rm -f gmon.out profile.txt
	rm -f core
	rm -f *.dSYM

# Pulizia completa
distclean: clean
	rm -f *~
	rm -f *.log

.PHONY: all test test-quick benchmark info memcheck profile clean distclean