#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include "Header.h"

#define SIZE		1000000
#define INTERVAL	100

static int array[SIZE];

struct timeval start_time;
struct timeval end_time;

int fillArray()
{
	srand(getpid());
	int i = 0;

	for (i = 0; i < SIZE; ++i)
		array[i] = (rand() % INTERVAL) + 1;

	if (i == SIZE)
		return 1;
	else
		return 0;
}

int pipe_transfer()
{
	int data_processed = 0;
    int total_data = 0;
    int file_pipes[2];
    int buffer[BUFSIZ + 1];
    int total_buffer[SIZE];

    pid_t fork_result;

    memset(buffer, '\0', sizeof(buffer));

    if (pipe(file_pipes) == 0) {
        fork_result = fork();
        if (fork_result == -1) {
            fprintf(stderr, "Fork failure");
            exit(EXIT_FAILURE);
        }
        
        if (fork_result != 0) {
            close(file_pipes[1]);
            int i, j = 0;

            gettimeofday(&start_time, NULL);

            while((data_processed = read(file_pipes[0], buffer, sizeof(buffer))) > 0){
                total_data += data_processed;
                for (i = 0; i <= (data_processed / sizeof(int)); ++i)
                {
                    total_buffer[i + j] = buffer[i];
                }
                j += i - 1;
            }

            gettimeofday(&end_time, NULL);

            int seconds = end_time.tv_sec - start_time.tv_sec;
            float microseconds = end_time.tv_usec - start_time.tv_usec;

            float milliseconds = (seconds * 1000) + (microseconds / 1000.0);

            printf("Pipe transfer: %.4f ms\n", milliseconds);

            for (i = 0; i < SIZE; ++i)
            {
                if(array[i] != total_buffer[i]){
                    printf("Dati NON ricevuti correttamente\n");
                    exit(EXIT_FAILURE);
                }
            }
            return 0;
            
        }

        else {
            data_processed = write(file_pipes[1], array, sizeof(array));
            printf("Wrote %d bytes on pipe\n", data_processed);
            kill(getpid(), SIGINT);
        }
    }
    return 0;
}

int client(){
    
    int res = 0;
    int sockfd = 0;
    
    struct sockaddr_in server;
    socklen_t len = sizeof(server);
    
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket error: ");
        return FAILURE;
    }
    
    res = connect(sockfd, (struct sockaddr *)&server, len);
    if (res != 0)
    {
        perror("connect() error: ");
        close(sockfd);
        return FAILURE;
    }
    
    double millis_end = 0;
    ssize_t n = 0;
    
    gettimeofday(&start_time, NULL);
    n = send(sockfd, array, sizeof(array), 0);
    if (n == -1)
    {
        perror("send() error: ");
        close(sockfd);
        return FAILURE;
    }
    printf("Client queued %d bytes to server\n", (int) n);
    
    n = recv(sockfd, &millis_end, sizeof(double), 0);
    if (n == -1) {
        perror("recv() error: ");
        close(sockfd);
        return FAILURE;
    } else if (n == 0) {
        printf("server closed connection\n");
    }
    if (n > 0) {
        double millis_start = (start_time.tv_sec * 1000) + ((double) start_time.tv_usec / 1000.0);
        printf("TCP transfer: %.4f ms\n", millis_end - millis_start);
    }
    close(sockfd);

    return 0;
}

int shm_transfer()
{
    int * shared_array;
    void *shared_memory = (void *)0;
    int shmid;

    shmid = shmget((key_t)1234, sizeof(array), 0666 | IPC_CREAT);

    if (shmid == -1) {
        fprintf(stderr, "shmget failed\n");
        exit(EXIT_FAILURE);
    }

    shared_memory = shmat(shmid, (void *)0, 0);
    if (shared_memory == (void *)-1) {
        fprintf(stderr, "shmat failed\n");
        exit(EXIT_FAILURE);
    }

    shared_array = (int *)shared_memory;

    pid_t fork_result;

    fork_result = fork();
    if (fork_result == -1) {
        fprintf(stderr, "Fork failure");
        exit(EXIT_FAILURE);
    }

    gettimeofday(&start_time, NULL);

    int i = 0;
    if (fork_result == 0)
    {
        for (i = 0; i < SIZE; ++i)
        {
            shared_array[i] = array[i];
        }
        printf("Wrote %ld bytes on shared memory\n", sizeof(array));
        exit(EXIT_SUCCESS);
    }
    else
    {
        int status;
        waitpid(fork_result, &status, 0);

        int received_array[SIZE];

        for(i = 0; i < SIZE; i++)
        {
            received_array[i] = shared_array[i];
        }
        gettimeofday(&end_time, NULL);

        int seconds = end_time.tv_sec - start_time.tv_sec;
        float microseconds = end_time.tv_usec - start_time.tv_usec;
        float milliseconds = (seconds * 1000) + (microseconds / 1000.0);

        printf("SHM transfer: %.4f ms\n", milliseconds);
        
        if (shmdt(shared_memory) == -1) {
            fprintf(stderr, "shmdt failed\n");
            exit(EXIT_FAILURE);
        }

        for (i = 0; i < SIZE; ++i)
        {
            if(array[i] != received_array[i]){
                printf("Dati NON ricevuti correttamente all'indice %d\n", i);
                exit(EXIT_FAILURE);
            }
        }

        return 0;
    }

    return 0;
}

int main(int argc, char const *argv[])
{
	if(!fillArray())
	{
		perror("fillArray");
		exit(-1);
	}

    printf("Trasferimento tramite PIPE\n\n");

    if(pipe_transfer())
    {
        perror("pipe_transfer");
        exit(-1);
    }

    printf("\nTrasferimento tramite TCP\n\n");

    if (client())
    {
        perror("client");
        exit(-1);
    }

    printf("\nTrasferimento tramite memoria condivisa\n\n");

    if (shm_transfer())
    {
        perror("shm_transfer");
        exit(-1);
    }

	return 0;
}