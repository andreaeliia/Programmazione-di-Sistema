ROOT=$(HOME)/Esami/Programmazione di Sistema/SNP2024/apue.3e
all: modify_check 

modify_check: modify_check.c
	gcc -ansi -I$(ROOT)/include -Wall -DMACOS -D_DARWIN_C_SOURCE  modify_check.c -o modify_check -L$(ROOT)/lib -lapue
clean:
	rm modify_check