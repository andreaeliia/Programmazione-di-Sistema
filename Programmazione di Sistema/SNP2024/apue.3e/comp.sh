gcc -ansi -I../include -Wall -DLINUX -D_GNU_SOURCE ${1}.c -o ${1} -L../lib -lapue
