#########################################################
# Makefile 
#
# author Catiuscia Melle
#
# Compilazione degli example codes 
# 	per multicast-aware applications
#  
# 	Multicast Sender e Multicast Receiver
#
#########################################################

CC=gcc
IFLAG=-I.

Send=SenderM
Rec=ReceiverM


all:interface_lister $(Rec) $(Send) $(SR)

# determinazione degli interface-index di un host
# Necessaria la compilazione con la macro _XOPEN_SOURCE (=700)
# per fare riferimento alle definizioni in SuSv4 
interface_lister: interface_lister.c
	$(CC) -o $@ $^ -D_XOPEN_SOURCE=700


# receiver 
$(Rec): MulticastReceiver.c 
	$(CC) -o $@ $^ -I.


# sender 
$(Send): MulticastSender.c 
	$(CC) -o $@ $^ -I.

clean:
	rm interface_lister $(Rec) $(Send) 
#########################################################

