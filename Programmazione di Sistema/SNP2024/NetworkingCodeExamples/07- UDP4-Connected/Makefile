############################################################
# Makefile 
#
# author Catiuscia Melle
#
# Compilazione degli example codes  
# comunicazioni over UDP (IPv4 only)
#
###########################################################
CC=gcc
DEPS=
IFLAGS=

TARGETS=echoS
TARGETC=echoC
CConnected=ConnectedClient


all: $(TARGETS) $(TARGETC) $(CConnected)

$(TARGETS): UDPServer-connected.c
	$(CC) -ggdb -o $@ $^

$(TARGETC): UDPClient.c
	$(CC) -ggdb -o $@ $^


$(CConnected): ClientConnected.c
	$(CC) -ggdb -o $@ $^

clean:
	rm $(TARGETS) $(TARGETC) $(CConnected)

############################################################