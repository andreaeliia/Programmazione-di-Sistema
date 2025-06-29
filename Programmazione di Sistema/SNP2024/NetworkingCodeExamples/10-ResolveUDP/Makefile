##################################################
# Makefile 
#
# author Catiuscia Melle
#
# Compilazione degli example codes per un 
# 	client e server UDP con resolution
#
##################################################

all: UDPServer UDPClient


UDPServer: UDPServer.c 
	gcc -o $@ $^


UDPClient: UDPClient.c 
	gcc -o $@ $^


clean:
	rm -f UDPServer UDPClient 

##################################################