ROOT="/home/recre/Esami/Programmazione di Sistema/SNP2024/apue.3e"
SRC=thread_directory.c
BIN=$(basename $(SRC))



$(BIN): $(SRC)
	gcc -ansi -I$(ROOT)/include -Wall -DMACOS -D_DARWIN_C_SOURCE $(SRC) -o $(BIN) -L$(ROOT)/lib -lapue


clean:
	rm -f $(BIN)