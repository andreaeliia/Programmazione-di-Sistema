#########################################################
# Makefile 
#
# author Catiuscia Melle
#
# Compilazione degli example codes  
# 	UDP client e server non blocking socket mode
#
##################################################

CC=gcc

all: Echo-S UDP-Client 


Echo-S:EchoServer.c Shares.c
	gcc -o $@ $^ -I.


UDP-Client:UDP-Client.c Shares.c
	gcc -o $@ $^ -I.


clean:
	rm -f Echo-S UDP-Client 
##################################################