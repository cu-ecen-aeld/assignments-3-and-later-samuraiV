#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char* argv[]){
	openlog(argv[0], LOG_PID, LOG_USER);
				
	if ((argc-1) != 2){
		syslog(LOG_ERR, "Incorrect number of arguments");
		return 1;
	}
	
	char* file = argv[1];
	char* text = argv[2];
	
	int fd = open(file, O_WRONLY | O_CREAT, 0644);
	if (fd == -1){
		syslog(LOG_ERR, "Unable to create file: %s", file);
		return 1;
	}else {
		syslog(LOG_INFO, "Successfully created file: %s", file);
	}

	ssize_t nr = write(fd, text, strlen(text));
	if(nr == -1){
		syslog(LOG_ERR, "Unable to write text: %s to file: %s", text, file);
	}
	syslog(LOG_DEBUG, "Writing %s to %s", text, file);
	
	if(close(fd) == -1){
		syslog(LOG_ERR, "Unable to close file: %s", file);
	}

	return 0;
}
