#########################################################
# Makefile 
#
# author Catiuscia Melle
#
# Compilazione degli example codes  
# 	IPv4 Broadcast
#
#########################################################

CC=gcc
IFLAG=-I.

Breceiver=ReceiverB
Bsender=Broadcaster

all: $(Bsender) $(Breceiver)

$(Bsender): bSender.c 
	$(CC) -ggdb -o $@ $^ -I.

$(Breceiver): bReceiver.c 
	$(CC) -ggdb -o $@ $^ -I.



clean:
	rm $(Bsender) $(Breceiver)

#########################################################