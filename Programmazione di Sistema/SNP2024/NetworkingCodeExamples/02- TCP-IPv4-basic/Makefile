##################################################
# Makefile 
#
# author Catiuscia Melle
#
# Compilazione degli example codes 
#
# Esempio di TCP Echo client e TCP Echo server 
# IPv4 only - no hostname resolution 
# uso dei wildcard IPv4 addresses
# 
##################################################

all: TCPServer TCPClient 

TCPServer: TCPServer.c
	gcc -ggdb -o TCPServer TCPServer.c

TCPClient: TCPClient.c
	gcc -ggdb -o TCPClient TCPClient.c

clean:
	rm TCPServer TCPClient 

##################################################