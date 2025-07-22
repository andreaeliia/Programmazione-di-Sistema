# Makefile per Simulazione Incrocio Stradale
# Compatibile con Linux e Mac

PROGS = traffic_intersection

# Compilatore
CC = gcc
COMMON_FLAGS = -ansi -Wall -I./include

# Sistema operativo
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

# Regola specifica per traffic_intersection
traffic_intersection: traffic_intersection.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $^ -o $@ $(LINK_FLAGS)

# Test del programma
test: traffic_intersection
	@echo "=== Testing Traffic Intersection Simulation ==="
	./traffic_intersection

# Test breve (10 secondi)
test-short: traffic_intersection
	@echo "=== Short Test (10 seconds) ==="
	@echo "Modificare SIM_DURATION nel codice per test più brevi"
	timeout 10s ./traffic_intersection || true

# Verifica risorse di sistema
check-resources:
	@echo "=== Controllo Risorse Sistema ==="
	@echo "Semafori POSIX disponibili:"
	@if command -v ipcs >/dev/null 2>&1; then \
		ipcs -s 2>/dev/null | grep -E "(traffic|sem)" || echo "Nessun semaforo trovato"; \
	else \
		echo "Comando ipcs non disponibile"; \
	fi
	@echo "Memoria condivisa disponibile:"
	@if command -v ipcs >/dev/null 2>&1; then \
		ipcs -m 2>/dev/null | grep -E "(traffic|shm)" || echo "Nessuna memoria condivisa trovata"; \
	else \
		echo "Comando ipcs non disponibile"; \
	fi

# Pulizia risorse rimaste (in caso di terminazione anomala)
clean-resources:
	@echo "=== Pulizia Risorse Sistema ==="
	@echo "Rimozione semafori e memoria condivisa rimasti..."
	@-rm -f /dev/shm/traffic_* 2>/dev/null || true
	@-rm -f /dev/shm/sem.traffic_* 2>/dev/null || true
	@echo "Pulizia completata"

# Mostra informazioni di sistema
info:
	@echo "Sistema operativo: $(UNAME_S)"
	@echo "Compilatore: $(CC)"
	@echo "Flag comuni: $(COMMON_FLAGS)"
	@echo "Flag piattaforma: $(PLATFORM_FLAGS)"
	@echo "Flag linking: $(LINK_FLAGS)"
	@echo "Programmi: $(PROGS)"
	@echo "Supporto semafori POSIX: Sì"
	@echo "Supporto memoria condivisa: Sì"

# Debug con strace/dtrace (se disponibili)
debug: traffic_intersection
	@if command -v strace >/dev/null 2>&1; then \
		echo "=== Debug con strace (Linux) ==="; \
		strace -e trace=semop,shmget,shmctl,fork ./traffic_intersection; \
	elif command -v dtruss >/dev/null 2>&1; then \
		echo "=== Debug con dtruss (Mac) ==="; \
		sudo dtruss -f ./traffic_intersection; \
	else \
		echo "Tool di debug non disponibili"; \
		echo "Esecuzione normale:"; \
		./traffic_intersection; \
	fi

# Controllo memoria con valgrind (se disponibile)
memcheck: traffic_intersection
	@if command -v valgrind >/dev/null 2>&1; then \
		echo "=== Memory Check con Valgrind ==="; \
		valgrind --tool=memcheck --leak-check=full --track-fds=yes ./traffic_intersection; \
	else \
		echo "Valgrind non disponibile per memory check"; \
		echo "Esecuzione normale:"; \
		./traffic_intersection; \
	fi

# Simulazione lunga per stress test
stress-test: traffic_intersection
	@echo "=== Stress Test (modificare SIM_DURATION per test più lunghi) ==="
	@echo "Premere Ctrl+C per interrompere"
	./traffic_intersection

# Help
help:
	@echo "Target disponibili:"
	@echo "  all           - Compila il programma"
	@echo "  test          - Esegue test standard"
	@echo "  test-short    - Esegue test breve (10 sec)"
	@echo "  check-resources - Verifica risorse sistema"
	@echo "  clean-resources - Pulisce risorse rimaste"
	@echo "  info          - Mostra informazioni sistema"
	@echo "  debug         - Esegue con tool di debug"
	@echo "  memcheck      - Controllo memoria con valgrind"
	@echo "  stress-test   - Test di stress prolungato"
	@echo "  clean         - Rimuove file compilati"
	@echo "  help          - Mostra questo aiuto"

# Pulizia dei binari
clean:
	rm -f $(PROGS)
	rm -f core
	rm -f *.dSYM

# Pulizia completa
distclean: clean clean-resources
	rm -f *~
	rm -f *.log

.PHONY: all test test-short check-resources clean-resources info debug memcheck stress-test help clean distclean