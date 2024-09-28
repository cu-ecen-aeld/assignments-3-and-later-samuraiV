#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <string.h>

#define PORT "9000"  // the port users will be connecting to
#define BACKLOG 10	 // how many pending connections queue will hold
#define MAXDATASIZE 1000000 // max number of bytes we can get at once
#define FILENAME "/var/tmp/aesdsocketdata"


int fd; // file descriptor for /var/tmp/aesdsocketdata file
int sockfd, new_fd, num_bytes;  // listen on sock_fd, new connection on new_fd, number of recv bytes

void sigchld_handler(int s)
{
	(void)s; // quiet unused variable warning

	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

void sig_handler(int s)
{
	remove(FILENAME);
	close(fd);
	close(sockfd);
	close(new_fd);
	syslog(LOG_INFO, "Caught signal, exiting");
	closelog();	
	exit(-1);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void demonize(){
	pid_t pid, sid;

	// Fork off the parent process
	pid = fork();
	if(pid < 0){
		exit(EXIT_FAILURE);
	}
	if(pid > 0){
		exit(EXIT_SUCCESS); // Parent exits
	}

	// Create a new session
	sid = setsid();
	if(sid < 0){
		exit(EXIT_FAILURE);
	}

}

int main(int argc, char* argv[])
{
	openlog(argv[0], LOG_PID, LOG_USER);
	fd = open(FILENAME, O_CREAT|O_TRUNC, 0644);
	close(fd);

	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	char buf[MAXDATASIZE];
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		//fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			return -1;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		//fprintf(stderr, "server: failed to bind\n");
		return -1;
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		return -1;
	}

	sa.sa_handler = sig_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("sigaction");
		return -1;
	}
	if (sigaction(SIGTERM, &sa, NULL) == -1) {
		perror("sigaction");
		return -1;
	}
	sa.sa_handler = sigchld_handler; // reap all dead processes
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		return -1;
	}

	//printf("server: waiting for connections...\n");

	if(argc>1){
		if(strcmp(argv[1], "-d") == 0){
			demonize();
		}
	}

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		syslog(LOG_INFO, "Accepted connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			
			if((num_bytes = recv(new_fd, buf, MAXDATASIZE, 0)) == -1){
				perror("recv");
				exit(1);
			}
							
			//printf("server: received %s\n", buf);
			fd = open(FILENAME, O_WRONLY | O_APPEND, 0644);
			ssize_t nr = write(fd, buf, num_bytes);
			close(fd);
			if(nr == -1){
				return -1;
			}

			fd = open(FILENAME, O_RDONLY, 0644);
			nr = read(fd, buf, MAXDATASIZE);
			//printf("server: file read: %s\n", buf);
			close(fd);
			if(nr == -1){
				return -1;
			}

			if (send(new_fd, buf, nr, 0) == -1)
				perror("send");
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
		syslog(LOG_INFO, "Closed connection from %s\n", s);
	}
	
	closelog();
	return 0;
}
