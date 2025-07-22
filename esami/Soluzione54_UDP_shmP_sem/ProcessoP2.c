#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "tlpi_hdr.h"
#include <stdio.h>
#include <stdlib.h>

#define shm_name "IPText"

int
main()
{
    int fd;
    char *addr;
    struct stat sb;

    fd = shm_open(shm_name, O_RDONLY, 0);
    if (fd == -1){
        perror("shm_open");
        exit(-1);
    }

    if (fstat(fd, &sb) == -1){
        perror("fstat");
        exit(-1);
    }

    addr = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED){
        perror("mmap");
        exit(-1);
    }

    if (close(fd) == -1){
        perror("close");
        exit(-1);
    }

    FILE * fd_output = fopen("output.txt", "w");
    fprintf(fd_output, "%s\n", addr);
    exit(0);
}