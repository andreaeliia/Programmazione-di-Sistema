##################################################
# Makefile 
#
# author Catiuscia Melle
#
# Compilazione degli example codes 
# esempio di client e server TCP 
#
#
##################################################

all: TCPS TCPC TCPC2 TCPS2 TCPS3

TCPS: TCPConcurrentS.c
	gcc -ggdb -o TCPS TCPConcurrentS.c

# Copia modificata da me per vedere il contenuto di sockaddr col debugger
TCPS2: TCPConcurrentS2.c
	gcc -ggdb -o TCPS2 TCPConcurrentS2.c

# Copia modificata da me per vedere l'effetto di SO_REUSEADDR
TCPS3: TCPConcurrentS3.c
	gcc -ggdb -o TCPS3 TCPConcurrentS3.c

TCPC: TCPClient.c
	gcc -ggdb -o TCPC TCPClient.c

TCPC2: TCPClient2.c
	gcc -ggdb -o TCPC2 TCPClient2.c

clean:
	rm -f TCPS TCPC TCPC2 TCPS2

##################################################