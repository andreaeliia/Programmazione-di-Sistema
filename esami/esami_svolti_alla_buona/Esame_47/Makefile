PROGS = cmdline_access

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

# Test con diversi argomenti
test1:
	@echo "Test 1: Solo nome programma"
	./$(PROGS)

test2:
	@echo "Test 2: Argomenti semplici"
	./$(PROGS) hello world 123

test3:
	@echo "Test 3: Argomenti complessi"
	./$(PROGS) "argomento con spazi" -flag 42 3.14 /path/to/file

test4:
	@echo "Test 4: Molti argomenti"
	./$(PROGS) a b c d e f g h i j k l m n o p

test_all: $(PROGS)
	@echo "=== ESECUZIONE TUTTI I TEST ==="
	@echo
	@make test1
	@echo
	@make test2
	@echo
	@make test3
	@echo
	@make test4

.PHONY: all clean test1 test2 test3 test4 test_all