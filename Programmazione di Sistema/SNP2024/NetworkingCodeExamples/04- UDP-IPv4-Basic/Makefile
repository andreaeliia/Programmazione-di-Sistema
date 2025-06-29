############################################################
# Makefile 
#
# author Catiuscia Melle
#
# Compilazione degli example codes  
# comunicazioni over UDP (IPv4 only)
#
# I due esempi differiscono in 
# - UDP echo client e server skeleton
# - UDP connected sockets: il server connette il 
#	socket al primo client che lo contatta e durante 
#	questo intervallo non è in grado di ricevere traffico
#	da altri nodi.
# 
# La demo prevede di mandare in esecuzione 
#   connected-UDPS -> looping-UDPC -> più istanze di echoC
#   Tutte le istanze di echoC rimangono bloccate nella 
#	recvfrom() perché non ricevono la notifica dell'ICMP
#	port unreachable...fino alla fine del ciclo
#
###########################################################
CC=gcc
DEPS=
IFLAGS=

TARGETS=echoS
TARGETC=echoC

TARGETS2=connectedUDPS
TARGETC2=loopingUDPC


all: $(TARGETS) $(TARGETC) $(TARGETS2) $(TARGETC2)

$(TARGETS): UDPServer.c
	$(CC) -o $@ $^

$(TARGETC): UDPClient.c
	$(CC) -o $@ $^

$(TARGETS2): UDPServer-connected.c
	$(CC) -o $@ $^

$(TARGETC2): UDPClient-counter.c
	$(CC) -o $@ $^

clean:
	rm $(TARGETS) $(TARGETC) $(TARGETS2) $(TARGETC2)

############################################################