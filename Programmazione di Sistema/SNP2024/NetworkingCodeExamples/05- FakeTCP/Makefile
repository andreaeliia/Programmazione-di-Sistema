##################################################
# Makefile 
#
# author Catiuscia Melle
#
# Compilazione degli example codes 
# esempio di client e server TCP IPv4 only, 
#
# L'esempio illustra il funzionamento del protocollo TCP 
# su Mac e su Linux 
# mediante blocchi espliciti dell'esecuzione
#
##################################################

all: FakeS FakeC 

FakeS: FakeServer.c
	gcc -o FakeS FakeServer.c

FakeC: FakeClient.c
	gcc -o FakeC FakeClient.c

clean:
	rm FakeS FakeC 

##################################################