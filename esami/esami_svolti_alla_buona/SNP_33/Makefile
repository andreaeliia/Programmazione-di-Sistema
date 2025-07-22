PROGS = master slave

# Compilatore
CC = gcc
COMMON_FLAGS = -ansi -Wall -I./include
LIBS = -lpthread -lm

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

# Regole specifiche per ogni programma
master: master.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $^ -o $@ $(LINK_FLAGS) $(LIBS)

slave: slave.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $^ -o $@ $(LINK_FLAGS) $(LIBS)

# Pulizia dei binari
clean:
	rm -f $(PROGS) results_slave_*.txt

# Regola per eseguire test
test: all
	@echo "Per testare:"
	@echo "1. In un terminale: ./master"
	@echo "2. In altri terminali: ./slave (modificare SLAVE_K per ogni istanza)"

.PHONY: all clean test