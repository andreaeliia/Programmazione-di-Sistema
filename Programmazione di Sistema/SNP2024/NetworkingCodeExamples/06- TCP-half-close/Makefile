#########################################################
# Makefile 
#
# author Catiuscia Melle
#
# Compilazione degli example codes  
# TCP client e server 
#
#
# Server risponde con un messaggio finale se passata l'opzione 1, 
# 		non invia messaggio finale se passata l'opzione 2
#
# Client invia messaggio ed esegue half-close con opzione 2
#
#########################################################
CC=gcc

all: Server Client

# Server per lo Shut client
Server: Server.c
	$(CC) -o $@  $^
	
Client: shut-client.c
	$(CC) -o $@ $^

clean:
	rm Server Client 

#########################################################
