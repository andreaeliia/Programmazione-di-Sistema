#include "apue.h"
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>


int main()
{
    FILE *f;
	char *stringa;
    char *fName = "/Users/patryk/Desktop/universita/Magistrale/PrimoAnno/Sistemi/Esempi/apue.3e/Esempi/EserciziEsame/Esame53/immagine.jpg";
    
    char *array;
    long fSize;
    
    
    
    if((f=fopen(fName, "rb"))==NULL)
    {
        fprintf(stderr, "Error\n");
        exit(1);
    }
    
   
    fseek (f , 0 , SEEK_END);
    fSize = ftell (f);
    rewind (f);
    array=(char *)malloc(fSize+1);

    fread(array, sizeof(char), 4, f);
    fflush(stdin);
    sprintf(stringa,"%02hhx%02hhx", array[0], array[1]);
    
    if(strcmp(stringa, "ffd8")==0){
    printf("Giusto!");}
    free(array);
}