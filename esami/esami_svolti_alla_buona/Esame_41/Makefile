PROGS = shell_enhanced

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

# Pulizia dei binari
clean:
	rm -f $(PROGS)

.PHONY: all clean