#########################################################
# Makefile 
#
# author Catiuscia Melle
#
# Compilazione degli example codes per un 
# 	client e server con I/O multiplexing  
#
# Il server, se usa AF_INET6, imposta l'opzione 
# 	IPV6_V6ONLY
#
# Il Fake Client fa invii in cascata, senza rilevare i 
# 	pending error sul socket (broken pipe)
#
#########################################################
CC=gcc
IFLAG=-I.

DEPS=SelectServer.c Utility.c
DEPC=SelectClient.c Utility.c
DEPF=FakeClient.c Utility.c

# Targets
SERVERT=SelectS
CLIENTT=SelectC
FAKEC=FakeC

all: $(CLIENTT) $(SERVERT) $(FAKEC)

$(SERVERT): $(DEPS)
	$(CC) -o $@ $(DEPS) $(IFLAGS)

$(CLIENTT): $(DEPC)
	$(CC) -o $@ $(DEPC) $(IFLAGS)

$(FAKEC): $(DEPF)
	$(CC) -o $@ $(DEPF) $(IFLAGS)

clean:
	rm -f $(SERVERT) $(CLIENTT) $(FAKEC)

##################################################
