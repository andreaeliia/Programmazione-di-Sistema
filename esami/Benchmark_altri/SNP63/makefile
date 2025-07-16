ROOT="/mnt/c/Users/recre/Programmazione-di-Sistema/Programmazione di Sistema/SNP2024/apue.3e"
SRC=directorydaemon.c
BIN=$(basename $(SRC))

all: $(BIN)

$(BIN): $(SRC)
	gcc -ansi -I$(ROOT)/include -Wall -DMACOS -D_DARWIN_C_SOURCE $(SRC) -o $(BIN) -L$(ROOT)/lib -lapue

clean:
	rm -f $(BIN)
