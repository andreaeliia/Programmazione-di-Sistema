# Makefile per Memory Mapping con Sincronizzazione Inter-Process
# Due soluzioni: Processo Padre-Figlio e Processi Indipendenti

# Variabili
CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LIBS = -lpthread -lrt

# File sorgenti
SRC1 = processo_padre_figlio.c
SRC2_FIRST = first_process.c
SRC2_SECOND = second_process.c

# Eseguibili
TARGET1 = processo_padre_figlio
TARGET2_FIRST = first_process
TARGET2_SECOND = second_process

# Target predefinito: compila tutto
all: soluzione1 soluzione2

# Soluzione 1: Processo Padre-Figlio
soluzione1: $(TARGET1)

$(TARGET1): $(SRC1)
	@echo "Compilando Soluzione 1 (Padre-Figlio)..."
	$(CC) $(CFLAGS) -o $(TARGET1) $(SRC1) $(LIBS)
	@echo "$(TARGET1) compilato con successo"

# Soluzione 2: Processi Indipendenti
soluzione2: $(TARGET2_FIRST) $(TARGET2_SECOND)

$(TARGET2_FIRST): $(SRC2_FIRST)
	@echo "Compilando Primo Processo (Soluzione 2)..."
	$(CC) $(CFLAGS) -o $(TARGET2_FIRST) $(SRC2_FIRST) $(LIBS)
	@echo "$(TARGET2_FIRST) compilato con successo"

$(TARGET2_SECOND): $(SRC2_SECOND)
	@echo "Compilando Secondo Processo (Soluzione 2)..."
	$(CC) $(CFLAGS) -o $(TARGET2_SECOND) $(SRC2_SECOND) $(LIBS)
	@echo "$(TARGET2_SECOND) compilato con successo"

# Target per compilare solo la prima soluzione
padre-figlio: $(TARGET1)

# Target per compilare solo la seconda soluzione
indipendenti: $(TARGET2_FIRST) $(TARGET2_SECOND)

# Pulizia file compilati
clean:
	@echo "Rimuovendo file compilati..."
	rm -f $(TARGET1) $(TARGET2_FIRST) $(TARGET2_SECOND)
	rm -f shared_file.txt
	@echo "Pulizia completata"

# Pulizia completa (include anche file temporanei)
distclean: clean
	@echo "Rimozione completa..."
	rm -f *.o *~ core
	@echo "Pulizia completa terminata"

# Target di aiuto
help:
	@echo "=== MAKEFILE MEMORY MAPPING SOLUTIONS ==="
	@echo ""
	@echo "Target disponibili:"
	@echo "  all              - Compila entrambe le soluzioni"
	@echo "  soluzione1       - Compila solo Soluzione 1 (Padre-Figlio)"
	@echo "  soluzione2       - Compila solo Soluzione 2 (Processi Indipendenti)"
	@echo "  padre-figlio     - Alias per soluzione1"
	@echo "  indipendenti     - Alias per soluzione2"
	@echo "  clean            - Rimuove file compilati"
	@echo "  distclean        - Pulizia completa"
	@echo "  help             - Mostra questo aiuto"
	@echo "  test-sol1        - Esegue test Soluzione 1"
	@echo "  test-sol2        - Istruzioni per test Soluzione 2"
	@echo ""
	@echo "Esempi di utilizzo:"
	@echo "  make                    # Compila tutto"
	@echo "  make soluzione1         # Compila solo padre-figlio"
	@echo "  make clean              # Pulisce file compilati"

# Target di test per Soluzione 1
test-sol1: $(TARGET1)
	@echo "Eseguendo test Soluzione 1..."
	@echo "Avviando processo padre-figlio..."
	./$(TARGET1)

# Istruzioni per testare Soluzione 2
test-sol2: $(TARGET2_FIRST) $(TARGET2_SECOND)
	@echo "Per testare la Soluzione 2:"
	@echo "1. Apri un terminale ed esegui: ./$(TARGET2_FIRST)"
	@echo "2. Apri un secondo terminale ed esegui: ./$(TARGET2_SECOND)"
	@echo "3. Osserva l'output intercalato nei due terminali"

# Target per installazione (opzionale)
install: all
	@echo "Installazione eseguibili in /usr/local/bin (richiede sudo)..."
	sudo cp $(TARGET1) $(TARGET2_FIRST) $(TARGET2_SECOND) /usr/local/bin/
	@echo "Installazione completata"

# Dichiarazione target phony
.PHONY: all soluzione1 soluzione2 padre-figlio indipendenti clean distclean help test-sol1 test-sol2 install