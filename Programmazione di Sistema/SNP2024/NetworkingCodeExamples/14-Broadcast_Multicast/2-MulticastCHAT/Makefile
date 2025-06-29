#########################################################
# Makefile 
#
# author Catiuscia Melle
#
# Compilazione degli example codes 
# 	per multicast-aware applications
#  
# 	MulticastCHAT Sender e Receiver
#
#########################################################

CC=gcc
IFLAG=-I.

TARGET=ChatMulticast


all: $(TARGET) 

# sender e receiver: chat multicast
$(TARGET): ChatMulticast.c Utility.c
	$(CC) -o $@ $^ -I.


clean:
	rm $(TARGET)
#########################################################

