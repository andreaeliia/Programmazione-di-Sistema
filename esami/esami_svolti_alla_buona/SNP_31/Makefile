PROGS = server client

# Compilatore
CC = gcc
COMMON_FLAGS = -ansi -Wall -I./include
THREAD_FLAGS = -pthread

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

# Regola per server (necessita di pthread)
server: server.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $(THREAD_FLAGS) $^ -o $@ $(LINK_FLAGS)

# Regola per client
client: client.c
	$(CC) $(COMMON_FLAGS) $(PLATFORM_FLAGS) $^ -o $@ $(LINK_FLAGS)

# Pulizia dei binari
clean:
	rm -f $(PROGS)

# Target per eseguire il server
run-server:
	./server

# Target per eseguire il client
run-client:
	./client

# Target per test con piu client
test:
	./client "Test messaggio 1" &
	./client "Test messaggio 2" &
	./client "Test messaggio 3" &
	wait

.PHONY: all clean run-server run-client test