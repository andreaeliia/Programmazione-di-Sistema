#########################################################
# Makefile 
#
# author Catiuscia Melle
#
# Compilazione degli example codes  
# TCP client e server 
#
# Socket Options: SO_LINGER
# 
#########################################################
CC=gcc

all: Linger

# funzionamento di close con SO_LINGER
Linger: linger-client.c
	$(CC) -o Linger linger-client.c 


clean:
	rm -f Linger

#########################################################
