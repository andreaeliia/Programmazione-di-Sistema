# Lista dei programmi da generare
PROGS = Pipe

# Compilatore e flag
COMMON_FLAGS = -ansi -I../include -Wall
LINUX_FLAGS = -DLINUX -D_GNU_SOURCE
MACOS_FLAGS = -DMACOS -D_DARWIN_SOURCE

# Regola predefinita: compila tutti i programmi
all: $(PROGS)

# Regola generica per compilare ogni programma
ifeq ($(shell uname), Linux)
	gcc $(COMMON_FLAGS) $(LINUX_FLAGS) $(PROGS).c -o $(PROGS)
else ifeq ($(shell uname), Darwin)
	gcc $(COMMON_FLAGS) $(MACOS_FLAGS) $(PROGS).c -o $(PROGS)
endif

# Regola per pulire i file generati
clean:
	rm -f $(PROGS)
