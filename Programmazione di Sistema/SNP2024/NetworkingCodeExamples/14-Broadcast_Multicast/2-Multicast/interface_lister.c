/**
@addtogroup Group14
@{
*/
/**
@file 	interface_lister.c
@author Catiuscia Melle
@brief 	API di lettura delle interfacce di un host, secondo lo standard SuSv4.

*/

#include <net/if.h>
#include <stdio.h>
#include <errno.h>


int main(int argc, char *argv[]){
	
	char IName[IF_NAMESIZE] = "";
	
	/*
	if_nameindex() ritorna i nomi e gli indici di tutte le schede di rete dell'host, in una
	struct if_nameindex, allocata dinamicamente, che deve essere rilascita
	invocando if_freenameindex()
	*/
	struct if_nameindex *ptr = if_nameindex();
	
	if (ptr != NULL)
	{
		while (ptr->if_name != NULL)
		{
			printf("Interface Index = %d - Interface Name = %s\n", 
							if_nametoindex(ptr->if_name),
							if_indextoname(ptr->if_index, IName));		
			ptr++;
		}//wend
	}
	else
	{
		perror("Error on if_nameindex(): ");
	}
	
return 0;
}
/**@}*/