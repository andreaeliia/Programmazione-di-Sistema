##############################################################
# Makefile 
#
# author Catiuscia Melle
#
# Compilazione degli example codes per la 
# manipolazione degli indirizzi Internet
#
# L'esempio illustra le funzioni di conversione degli
# indirizzi IPv4 ed IPv6 e degli indirizzi di porta TCP/UDP
# con le funzioni:
# - inet_ntop e inet_pton
# - ntohs() e htons()
##############################################################

CC=gcc
DEPS=
IFLAGS=

TARGET=SocketAddresses

all: $(TARGET) 

$(TARGET): *.c
	$(CC) -o $@ $^


clean:
	rm $(TARGET) 

##################################################
