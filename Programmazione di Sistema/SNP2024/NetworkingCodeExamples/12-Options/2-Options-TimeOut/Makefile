###########################################################
# Makefile 
#
# author Catiuscia Melle
#
# Compilazione degli example codes  
# 	UDP client e server 
# 	
# 	Socket Options: SO_RCVTIMEO SO_SNDTIMEO
# 
###########################################################
CC=gcc

TARGETS=UDPS
TARGETS=UDPS
TARGETC2=UDPC2
TARGETS2=UDPS2

DEPS=UDPServer.c 
DEPC=UDPClient.c 
DEPS2=UDPServer2.c 
DEPC2=UDPClient2.c 

all: $(TARGETS) $(TARGETC) $(TARGETS2) $(TARGETC2) 

$(TARGETS):  $(DEPS)
	$(CC) -o $@ $(DEPS) -I.

$(TARGETC):  $(DEPC)
	$(CC) -o $@ $(DEPC) -I.
	
$(TARGETS2):  $(DEPS2)
	$(CC) -o $@ $(DEPS2) -I.

$(TARGETC2):  $(DEPC2)
	$(CC) -o $@ $(DEPC2) -I.
	
clean:
	rm -f $(TARGETS) $(TARGETC) $(TARGETS2) $(TARGETC2)
###########################################################