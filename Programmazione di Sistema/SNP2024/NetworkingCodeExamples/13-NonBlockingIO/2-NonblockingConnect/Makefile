#########################################################
# Makefile 
#
# author Catiuscia Melle
#
# Compilazione degli example codes  
# TCP client non blocking socket mode
#
##################################################

CC=gcc

all: TCPS TCPC nonblockC


TCPS:TCPS.c Utility.c
	$(CC) -o $@ $^ -I.

TCPC:TCPC.c Utility.c
	$(CC) -o $@ $^ -I.


nonblockC:TCPC-nonblock.c Utility.c
	$(CC) -o $@ $^ -I.
	
	
clean:
	rm TCPS TCPC nonblockC
##################################################
