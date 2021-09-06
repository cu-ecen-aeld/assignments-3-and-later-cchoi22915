//Writer.c
//Chris Choi
//Assignment 2


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <syslog.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>



int main(int argc, char *argv[])
{
	//start logging
	openlog(NULL, LOG_CONS, LOG_USER);

	//ensure number of input arguments is correct
	if(argc != 3)
	{
		syslog(LOG_ERR, "Invalid function arguments - ex:<directory>,<string-to-write>\n");
		return 1;
	}

	//save argument 1 and 2 locally
	char *path = argv[1];
	char *str = argv[2];

	//open file
	int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, S_IRWXU); 

	//check to see if file open
	if(fd == -1)
	{
		syslog(LOG_ERR, "Cannot open or touch file %s\n", path);
		return 1;
	}

	//count bytes while writing to ensure correct write
	ssize_t byte_count = write(fd, str, strlen(str));

	//error out if failed to write successfully or log if successful
	if(byte_count != strlen(str))
	{
		syslog(LOG_ERR, "Failed to write <%s> in <%s>\n", str, path);
	}
	else
	{
		syslog(LOG_DEBUG, "Successfully wrote <%s> to <%s>\n", str, path);
	}

	//close file
	close(fd);

	//if failed to close, log, exit
	if(fd == -1)
	{
		syslog(LOG_ERR, "File cannot be closed\n");
		return 1;
	}
	
	//close log
	closelog();

	return 0;
}
