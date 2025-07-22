# Makefile per programmi client-server contatore file/directory
# Supporta compilazione su Linux e macOS con libreria apue

# Programmi da compilare
PROGS = A B

# Compilatore
CC = gcc
COMMON_FLAGS = -ansi -Wall -I./include

# Rileva sistema operativo
UNAME_S := $(shell uname)

# Flag specifici per piattaforma
ifeq ($(UNAME_S), Linux)
	PLATFORM_FLAGS = -DLINUX -D_GNU_SOURCE
	LINK_FLAGS = -L./lib -lapueLinux
else ifeq ($(UNAME_S), Darwin)
	PLATFORM_FLAGS = -DMACOS -D_DARWIN_SOURCE  
	LINK_FLAGS = -L./lib -lapueMacOS
endif

# Regola di default - compila tutti i programmi
all: $(PROGS)
	@echo "Compilazione completata per $(UNAME_S)"
	@echo "Programmi generati: $(PROGS)"

# Regola generica per compilare programmi C
%: %.c
	@echo "Compilando $< per $(UNAME_S)..."
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $^ -o $@ $(LINK_FLAGS)
	@echo "$@ compilato con successo"

# Compilazione client A
A: A.c
	@echo "Compilando client A..."
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) A.c -o A $(LINK_FLAGS)
	@echo "Client A compilato"

# Compilazione server B  
B: B.c
	@echo "Compilando server B..."
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) B.c -o B $(LINK_FLAGS)
	@echo "Server B compilato"

# Pulizia dei file binari
clean:
	@echo "Rimuovendo file binari..."
	rm -f $(PROGS)
	@echo "Pulizia completata"

# Reinstallazione completa
rebuild: clean all

# Mostra informazioni di sistema
info:
	@echo "Sistema operativo rilevato: $(UNAME_S)"
	@echo "Compilatore: $(CC)"
	@echo "Flag comuni: $(COMMON_FLAGS)"  
	@echo "Flag piattaforma: $(PLATFORM_FLAGS)"
	@echo "Flag linking: $(LINK_FLAGS)"
	@echo "Programmi: $(PROGS)"

# Test rapido della compilazione
test: all
	@echo "Test di compilazione completato"
	@echo "Verificando esistenza binari..."
	@ls -la $(PROGS) 2>/dev/null || echo "Alcuni binari potrebbero mancare"

# Regola per evitare conflitti con file omonimi
.PHONY: all clean rebuild info test