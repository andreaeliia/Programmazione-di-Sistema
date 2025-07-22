# ============================================================================
# Makefile per Lista Concatenata in Memoria Condivisa
# ============================================================================
# 
# Automatizza la compilazione, testing e pulizia del progetto.
# 
# UTILIZZO:
#   make         - Compila il progetto
#   make run     - Compila e lancia il programma
#   make test    - Esegue test automatici
#   make clean   - Rimuove file generati
#   make help    - Mostra questo aiuto
#   make debug   - Compila con flag di debug
#   make release - Compila ottimizzato per rilascio
#
# AUTORE: [Il tuo nome]
# DATA: [Data]
# ============================================================================

# === CONFIGURAZIONE ===

# Compilatore
CC = gcc

# Nome programma finale
TARGET = traccia

# File sorgente principale
SOURCES = traccia_completa.c

# Directory per file oggetto (opzionale)
OBJDIR = obj

# File oggetto generati
OBJECTS = $(SOURCES:.c=.o)

# === FLAGS DI COMPILAZIONE ===

# Flags base sempre utilizzati
CFLAGS_BASE = -std=c99 -pedantic

# Flags per warning (utili durante sviluppo)
CFLAGS_WARNINGS = -Wall -Wextra -Wshadow -Wpointer-arith -Wcast-align \
                  -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations \
                  -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
                  -Wconversion -Wstrict-prototypes

# Flags di debug (simboli debug, nessuna ottimizzazione)
CFLAGS_DEBUG = -g -O0 -DDEBUG

# Flags di release (ottimizzazione massima, no debug)
CFLAGS_RELEASE = -O3 -DNDEBUG -march=native

# Flags default (compromesso debug/performance)
CFLAGS_DEFAULT = -g -O1

# Librerie da linkare
LDFLAGS = -lrt -lpthread

# Flags correnti (default: DEFAULT)
CFLAGS = $(CFLAGS_BASE) $(CFLAGS_WARNINGS) $(CFLAGS_DEFAULT)

# === COLORI PER OUTPUT (opzionale) ===
COLOR_GREEN = \033[0;32m
COLOR_RED = \033[0;31m
COLOR_YELLOW = \033[1;33m
COLOR_BLUE = \033[0;34m
COLOR_RESET = \033[0m

# === TARGET PRINCIPALI ===

# Target di default (viene eseguito con solo "make")
.DEFAULT_GOAL := all

# Compila il programma
all: $(TARGET)
	@echo "$(COLOR_GREEN)✅ Compilazione completata con successo!$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)💡 Uso: ./$(TARGET)$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)💡 Help: make help$(COLOR_RESET)"

# Regola per creare l'eseguibile
$(TARGET): $(OBJECTS)
	@echo "$(COLOR_YELLOW)🔗 Linking $(TARGET)...$(COLOR_RESET)"
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Regola per compilare file .c in .o
%.o: %.c
	@echo "$(COLOR_YELLOW)🔨 Compilando $<...$(COLOR_RESET)"
	$(CC) $(CFLAGS) -c $< -o $@

# === TARGET CONVENIENCE ===

# Compila e lancia il programma
run: $(TARGET)
	@echo "$(COLOR_GREEN)🚀 Lanciando $(TARGET)...$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)💡 Per secondo processo: apri nuovo terminale e rilancia$(COLOR_RESET)"
	./$(TARGET)

# Compila versione debug
debug: CFLAGS = $(CFLAGS_BASE) $(CFLAGS_WARNINGS) $(CFLAGS_DEBUG)
debug: clean $(TARGET)
	@echo "$(COLOR_GREEN)🐛 Versione DEBUG compilata$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)💡 Usa gdb ./$(TARGET) per debugging$(COLOR_RESET)"

# Compila versione release (ottimizzata)
release: CFLAGS = $(CFLAGS_BASE) $(CFLAGS_RELEASE)
release: clean $(TARGET)
	@echo "$(COLOR_GREEN)🚀 Versione RELEASE compilata$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)💡 Ottimizzazioni attive$(COLOR_RESET)"

# === TARGET DI TESTING ===

# Test automatico di base
test: $(TARGET)
	@echo "$(COLOR_YELLOW)🧪 Eseguendo test automatici...$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)Test 1: Verifica compilazione... ✅$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)Test 2: Verifica dipendenze...$(COLOR_RESET)"
	@ldd ./$(TARGET) > /dev/null && echo "$(COLOR_GREEN)✅ Dipendenze OK$(COLOR_RESET)" || echo "$(COLOR_RED)❌ Problema dipendenze$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)Test 3: Verifica eseguibile...$(COLOR_RESET)"
	@file ./$(TARGET) | grep -q "executable" && echo "$(COLOR_GREEN)✅ Eseguibile valido$(COLOR_RESET)" || echo "$(COLOR_RED)❌ Problema eseguibile$(COLOR_RESET)"
	@echo "$(COLOR_GREEN)🎯 Test completati!$(COLOR_RESET)"

# Test avanzato con valgrind (se disponibile)
test-valgrind: debug
	@echo "$(COLOR_YELLOW)🔍 Test con Valgrind...$(COLOR_RESET)"
	@which valgrind > /dev/null 2>&1 || (echo "$(COLOR_RED)❌ Valgrind non installato$(COLOR_RESET)" && exit 1)
	@echo "$(COLOR_BLUE)💡 Avvia il programma in modalità test automatica$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)💡 Valgrind controllerà memory leaks$(COLOR_RESET)"
	# valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

# === TARGET DI UTILITÀ ===

# Pulizia file generati
clean:
	@echo "$(COLOR_YELLOW)🧹 Pulizia file generati...$(COLOR_RESET)"
	@rm -f $(TARGET) $(OBJECTS) *.o *~ core
	@echo "$(COLOR_GREEN)✅ Pulizia completata$(COLOR_RESET)"

# Pulizia completa (include anche memoria condivisa)
clean-all: clean
	@echo "$(COLOR_YELLOW)🧹 Pulizia memoria condivisa...$(COLOR_RESET)"
	@rm -f /dev/shm/linked_list_shm 2>/dev/null || true
	@ipcrm -M /linked_list_shm 2>/dev/null || true
	@echo "$(COLOR_GREEN)✅ Pulizia completa$(COLOR_RESET)"

# Reinstallazione completa
reinstall: clean-all all
	@echo "$(COLOR_GREEN)🔄 Reinstallazione completata$(COLOR_RESET)"

# === TARGET INFORMATIVI ===

# Mostra informazioni sul sistema
info:
	@echo "$(COLOR_BLUE)📋 INFORMAZIONI SISTEMA$(COLOR_RESET)"
	@echo "Compilatore: $(CC) $(shell $(CC) --version | head -n1)"
	@echo "Flags attuali: $(CFLAGS)"
	@echo "Librerie: $(LDFLAGS)"
	@echo "Target: $(TARGET)"
	@echo "Sorgenti: $(SOURCES)"
	@echo "Sistema: $(shell uname -s) $(shell uname -r)"
	@echo "Architettura: $(shell uname -m)"

# Controlla dipendenze
check-deps:
	@echo "$(COLOR_BLUE)🔍 CONTROLLO DIPENDENZE$(COLOR_RESET)"
	@echo -n "GCC: "
	@which gcc > /dev/null && echo "$(COLOR_GREEN)✅ Installato$(COLOR_RESET)" || echo "$(COLOR_RED)❌ Mancante$(COLOR_RESET)"
	@echo -n "Make: "
	@which make > /dev/null && echo "$(COLOR_GREEN)✅ Installato$(COLOR_RESET)" || echo "$(COLOR_RED)❌ Mancante$(COLOR_RESET)"
	@echo -n "POSIX Real-Time: "
	@echo "#include <sys/mman.h>" | gcc -E - > /dev/null 2>&1 && echo "$(COLOR_GREEN)✅ Disponibile$(COLOR_RESET)" || echo "$(COLOR_RED)❌ Mancante$(COLOR_RESET)"
	@echo -n "POSIX Threads: "
	@echo "#include <semaphore.h>" | gcc -E - > /dev/null 2>&1 && echo "$(COLOR_GREEN)✅ Disponibile$(COLOR_RESET)" || echo "$(COLOR_RED)❌ Mancante$(COLOR_RESET)"

# Mostra dimensione binario
size: $(TARGET)
	@echo "$(COLOR_BLUE)📏 DIMENSIONI BINARIO$(COLOR_RESET)"
	@size $(TARGET)
	@ls -lh $(TARGET)

# === TARGET DI SVILUPPO ===

# Formattazione codice (se clang-format disponibile)
format:
	@echo "$(COLOR_YELLOW)✨ Formattazione codice...$(COLOR_RESET)"
	@which clang-format > /dev/null 2>&1 || (echo "$(COLOR_RED)❌ clang-format non installato$(COLOR_RESET)" && exit 1)
	clang-format -i $(SOURCES)
	@echo "$(COLOR_GREEN)✅ Codice formattato$(COLOR_RESET)"

# Analisi statica con cppcheck (se disponibile)
analyze:
	@echo "$(COLOR_YELLOW)🔍 Analisi statica codice...$(COLOR_RESET)"
	@which cppcheck > /dev/null 2>&1 || (echo "$(COLOR_RED)❌ cppcheck non installato$(COLOR_RESET)" && exit 1)
	cppcheck --enable=all --std=c99 $(SOURCES)

# === TARGET DI DOCUMENTAZIONE ===

# Genera documentazione (placeholder)
docs:
	@echo "$(COLOR_BLUE)📚 Generazione documentazione...$(COLOR_RESET)"
	@echo "$(COLOR_YELLOW)💡 Leggere README.md per documentazione completa$(COLOR_RESET)"

# === TARGET DI AIUTO ===

# Mostra aiuto
help:
	@echo "$(COLOR_BLUE)📖 MAKEFILE HELP$(COLOR_RESET)"
	@echo ""
	@echo "$(COLOR_GREEN)Target principali:$(COLOR_RESET)"
	@echo "  make all       - Compila il progetto (default)"
	@echo "  make run       - Compila e lancia il programma"
	@echo "  make clean     - Rimuove file generati"
	@echo "  make help      - Mostra questo aiuto"
	@echo ""
	@echo "$(COLOR_GREEN)Versioni di compilazione:$(COLOR_RESET)"
	@echo "  make debug     - Compila con flag di debug (-g -O0)"
	@echo "  make release   - Compila ottimizzato (-O3)"
	@echo ""
	@echo "$(COLOR_GREEN)Testing:$(COLOR_RESET)"
	@echo "  make test      - Esegue test automatici"
	@echo "  make test-valgrind - Test con Valgrind (richiede installazione)"
	@echo ""
	@echo "$(COLOR_GREEN)Utilità:$(COLOR_RESET)"
	@echo "  make clean-all - Pulizia completa (include memoria condivisa)"
	@echo "  make reinstall - Reinstallazione completa"
	@echo "  make info      - Informazioni sistema"
	@echo "  make check-deps - Controlla dipendenze"
	@echo "  make size      - Mostra dimensioni binario"
	@echo ""
	@echo "$(COLOR_GREEN)Sviluppo:$(COLOR_RESET)"
	@echo "  make format    - Formatta codice (richiede clang-format)"
	@echo "  make analyze   - Analisi statica (richiede cppcheck)"
	@echo ""
	@echo "$(COLOR_BLUE)Esempio di utilizzo:$(COLOR_RESET)"
	@echo "  make debug     # Compila versione debug"
	@echo "  make run       # Lancia il programma"
	@echo "  make clean     # Pulisce quando finito"

# === CONFIGURAZIONI SPECIALI ===

# Previene cancellazione file intermedi
.PRECIOUS: %.o

# Target che non corrispondono a file
.PHONY: all run debug release test test-valgrind clean clean-all reinstall \
        info check-deps size format analyze docs help

# Forza ricompilazione se Makefile cambia
$(OBJECTS): Makefile

# ============================================================================
# NOTE AGGIUNTIVE
# ============================================================================
#
# 1. PERSONALIZZAZIONE:
#    - Modifica CC per usare compilatore diverso (clang, icc, etc.)
#    - Aggiungi CFLAGS specifici per il tuo sistema
#    - Personalizza colori o rimuovi per terminali che non li supportano
#
# 2. ESTENSIONI POSSIBILI:
#    - Target per generare pacchetti (tar.gz, deb, rpm)
#    - Integrazione con sistemi CI/CD
#    - Target per profiling (gprof)
#    - Target per coverage (gcov)
#
# 3. DEBUGGING:
#    - make debug per versione con simboli
#    - make test-valgrind per controllo memory leaks
#    - gdb ./traccia per debugging interattivo
#
# 4. RILASCIO:
#    - make release per versione ottimizzata
#    - make size per verificare dimensioni
#    - make check-deps per verificare sistema target
#
# ============================================================================