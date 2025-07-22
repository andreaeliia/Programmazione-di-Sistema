PROGS = Client Server ProcessoP2

# Compilatore
CC = gcc

COMMON_FLAGS = -ansi -Wall -I./include
LINK_FLAGS = -L./lib -lapue

# sistema operativo
UNAME_S := $(shell uname)

# Flag per piattaforma
ifeq ($(UNAME_S), Linux)
	PLATFORM_FLAGS = -DLINUX -D_GNU_SOURCE
else ifeq ($(UNAME_S), Darwin)
	PLATFORM_FLAGS = -DMACOS -D_DARWIN_SOURCE
endif

# Regola di default
all: $(PROGS)

# Regola generica per compilare ogni programma
%: %.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $^ -o $@ $(LINK_FLAGS)

# Pulizia dei binari
clean:
	rm -f $(PROGS)
