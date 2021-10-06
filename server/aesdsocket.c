/*
* file: aesdsocket.c
* 
* purpose: opens a socket @ port 9000, logs message to syslog upon connection, receives data over connection to file, *	returns full content of file to client
*	logs closed connection
*	restarts accepting connection until SIGINT or SIGTERM and gracefully exits
*	deletes the temp file
*
* author: Chris Choi
*
* date: 03 October 2021
*
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <arpa/inet.h> 
#include <fcntl.h>  
#include <signal.h> 
#include <syslog.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <getopt.h>
#include <netinet/in.h>


#define	PORT			    "9000"
#define	FILE_OUT_PATH		"/var/tmp/aesdsocketdata"
#define	BUFFER_SIZE		    100
#define	MAX_CONNECTION		4

#define FD_SIZE     3
#define FD_DATA     0
#define FD_CLIENT   1
#define FD_SOCKET   2

#define SUCCESS     0

int daemonFlag = 0;

char * buffer_rx;
char * buffer_tx;

struct sockaddr_in    server_address;
struct sockaddr_in    client_address;

int fd[FD_SIZE];

pid_t pid;


static void sighandler(int signum)
{
	if((signum == SIGTERM) || signum == SIGINT)
    {

        syslog(LOG_DEBUG, "SIGINT or SIGTERM received, stopping...\n");

        for(int i = 0; i < FD_SIZE; i++)
        {
            close(fd[i]);
        }

        free(buffer_rx);
        free(buffer_tx);

        closelog();
        remove(FILE_OUT_PATH);

        exit(SUCCESS);
    }
}



int main(int argc, char* argv[])
{

    struct addrinfo socketInfo;
	struct addrinfo *sockAddrInfo;
	struct sockaddr_in clientSockAddress;

	socklen_t addressSize = sizeof(struct sockaddr);

	sigset_t sigSetOne;
    sigset_t sigSetTwo;

    int  recievedByteCount;
	int bufferIndex = 0;
	int bufferCount = 1;

	off_t filesize;

    openlog(NULL, 0, LOG_USER);

	if(signal(SIGINT, sighandler) == SIG_ERR)
	{
		printf("ERROR: SIG_ERR-Cannot handle SIGINT\n");
		exit (EXIT_FAILURE);
	}

	if(signal(SIGTERM, sighandler) == SIG_ERR)
	{
		printf("ERROR: SIG_ERR-Cannot handle SIGTERM\n");
		exit (EXIT_FAILURE);
	}

	if(argc == 2)
	{
		if(strcmp("-d", argv[1]) == 0)
			daemonFlag = 1;
	}


    // ********************* make socket
    fd[FD_SOCKET] = socket(PF_INET, SOCK_STREAM, 0);

    // ********************* check socket creation
    if(fd[FD_SOCKET] < 0)
    {
        printf("ERROR: Failed to create socket, exiting...");
        exit (EXIT_FAILURE);
    }

    socketInfo.ai_family = PF_INET;
	socketInfo.ai_socktype = SOCK_STREAM;
	socketInfo.ai_flags = AI_PASSIVE;

    // ********************* call getaddrinfo and check return_value
    int return_value = getaddrinfo(NULL, PORT, &socketInfo, &sockAddrInfo);
    if(return_value != 0)
	{
		printf("ERROR: failed to get address information, exiting... %s\n", gai_strerror(return_value));
		freeaddrinfo(sockAddrInfo);
		exit (EXIT_FAILURE);
	}

    // ********************* bind
    return_value = bind(fd[FD_SOCKET], sockAddrInfo->ai_addr, sizeof(struct sockaddr));
	if(return_value == -1)
	{
		printf("ERROR: failed to bind, exiting... %s\n", strerror(errno));
		freeaddrinfo(sockAddrInfo);
		exit (EXIT_FAILURE);
	}

    freeaddrinfo(sockAddrInfo);

	return_value = listen(fd[FD_SOCKET], MAX_CONNECTION);
	if(return_value == -1)
	{
		printf("ERROR: failed to listen, exiting...\n");
		exit (EXIT_FAILURE);
	}

    // ********************* open write file
	fd[FD_DATA] = open(FILE_OUT_PATH, O_CREAT | O_APPEND | O_RDWR, S_IRWXU);
    if(fd[FD_DATA] == -1)
	{
		printf("ERROR: failed to open file, %s \n", FILE_OUT_PATH);
		exit (EXIT_FAILURE);
	}

	sigemptyset(&sigSetOne);
	sigaddset(&sigSetOne, SIGINT);
	sigaddset(&sigSetOne, SIGTERM);

    buffer_rx = (char *)malloc(BUFFER_SIZE*sizeof(char));
    buffer_tx = (char *)malloc(BUFFER_SIZE*sizeof(char));
	if(buffer_rx == NULL || buffer_tx == NULL)
	{
		printf("ERROR: failed to malloc buffers, exiting...\n");
		exit (EXIT_FAILURE);
	}	

    if(daemonFlag)
	{
		pid = fork();

		if(pid == -1)
			exit (EXIT_FAILURE);

		else if(pid != 0)
			exit(0);

		if(setsid() == -1)
			exit (EXIT_FAILURE);

		if(chdir("/") == -1)
			exit (EXIT_FAILURE);

		open("/dev/null", O_RDWR);

		dup(0);
		dup(0);
	}
	
	while(1)
	{
		/* Get the accepted client fd */
		fd[FD_CLIENT] = accept(fd[FD_SOCKET], (struct sockaddr *)&clientSockAddress, &addressSize); 

		/* Error occurred in accept, exit (EXIT_FAILURE) on error */
		if(fd[FD_CLIENT] == -1)
		{
			printf("ERROR: failed to accept client fd, exiting...\n");
			free(buffer_rx);
			exit (EXIT_FAILURE);
		}

		char *IP = inet_ntoa(clientSockAddress.sin_addr);
		syslog(LOG_DEBUG, "Accepted connection from %s\n", IP);

		/* Don't accept a signal while recieving data */
		if((return_value = sigprocmask(SIG_BLOCK, &sigSetOne, &sigSetTwo)) == -1)
			printf("ERROR: failed to sigprocmask, exiting...\n");
		
		/* Initially, write to starting of buffer */
		bufferIndex = 0;	
		
		/* Receive data */
		while((recievedByteCount = recv(fd[FD_CLIENT], buffer_rx + bufferIndex, BUFFER_SIZE, 0)) > 0)
		{
			/* Error occurred in recv, log error */
			if(recievedByteCount == -1)
			{
				printf("ERROR: failed to received data successfully, exiting..., %s\n", strerror(errno));
				exit (EXIT_FAILURE);
			}

			/* Detect newline character */
			char* newlineloc = strchr(buffer_rx, '\n');

			bufferIndex = bufferIndex + recievedByteCount;

			/* Buffer size needs to be increased */
			if((bufferIndex) >= (bufferCount * BUFFER_SIZE))
			{
				bufferCount++;
				char* newptr = realloc(buffer_rx, bufferCount*BUFFER_SIZE*sizeof(char));

				if(newptr == NULL)
				{
					free(buffer_rx);
					printf("ERROR: failed to reallocate, exiting...\n");
					exit (EXIT_FAILURE);
				}

				else buffer_rx = newptr;
					
			}

			else if (newlineloc != NULL)
			       	break;
			
		}
		
		// ********************* unmask signals that are pending...
		if((return_value = sigprocmask(SIG_UNBLOCK, &sigSetTwo, NULL)) == -1)
			printf("ERROR: failed sigprocmask, exiting...\n");
		
		// ********************* write to file
		int wr_bytes = write(fd[FD_DATA], buffer_rx, bufferIndex);

		if(wr_bytes != bufferIndex)
			syslog(LOG_ERR, "Bytes written to file do not match bytes recieved from connection\n");
	
		filesize = lseek(fd[FD_DATA], 0, SEEK_CUR);
		
		char* newptr = realloc(buffer_tx, filesize*sizeof(char));
		if(newptr == NULL)
		{
			free(buffer_tx);
			printf("ERROR: failed to reacllocate, exiting...\n");
			exit (EXIT_FAILURE);
		}

		else buffer_tx = newptr;

		// ********************* reset file pointer to start of file and read
		lseek(fd[FD_DATA], 0, SEEK_SET);
		int fread_bytes = read(fd[FD_DATA], buffer_tx, filesize);
		if(fread_bytes == -1)
		{
			printf("ERROR: failed to read, exiting...\n");
			exit (EXIT_FAILURE);
		}

		// ********************* mask signals
		if((return_value = sigprocmask(SIG_BLOCK, &sigSetOne, &sigSetTwo)) == -1)
			printf("ERROR: failed in sigprocmask, exiting...\n");
			
            
		// ********************* send data from file to client
		int sent_bytes = send(fd[FD_CLIENT], buffer_tx, fread_bytes, 0);
		if(sent_bytes == -1)
		{
			printf("ERROR: failed to send, exiting...\n");
			exit (EXIT_FAILURE);
		}
		
		if((return_value = sigprocmask(SIG_UNBLOCK, &sigSetTwo, NULL)) == -1)
			printf("ERROR: failed in sigprocmask, exiting...\n");
		
	

		// ********************* close client
		close(fd[FD_CLIENT]);
		if(fd[FD_CLIENT] == -1)
		{
			printf("ERROR: failed to close, exiting...\n");
			exit (EXIT_FAILURE);
		}

		syslog(LOG_DEBUG, "Closed connection from %s\n", IP);

	}
}






