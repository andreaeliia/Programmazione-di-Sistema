gcc -ggdb -ansi -I../../../include -Wall -DMACOS -D_DARWIN_C_SOURCE  ${1}.c -o ${1}  -L../../../lib -lapue
