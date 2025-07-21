ROOT="/mnt/c/Users/recre/Programmazione-di-Sistema/Programmazione di Sistema/SNP2024/apue.3e"
SRC=client.c
SRC2=server.c
BIN=$(basename $(SRC))
BIN2=$(basename $(SRC2))

all: $(BIN) $(BIN2)

$(BIN): $(SRC)
	gcc -ansi -I$(ROOT)/include -Wall -DMACOS -D_DARWIN_C_SOURCE $(SRC) -o $(BIN) -L$(ROOT)/lib -lapue

$(BIN2): $(SRC2)
	gcc -ansi -I$(ROOT)/include -Wall -DMACOS -D_DARWIN_C_SOURCE $(SRC2) -o $(BIN2) -L$(ROOT)/lib -lapue

clean:
	rm -f $(BIN) $(BIN2)