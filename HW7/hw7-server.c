//Minhtri Tran
//Unix Programming
//Assignment 7 - Networks and I/O Multiplexing
//Server

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <signal.h>
#include <netdb.h>
#include <sys/select.h>

#define	LISTENQ  1024
#define BUFFER_SIZE 1024
#define TIMEOUT_SECS 180
#define DEFAULT_PORT 13000

int main(int argc, char *argv[]) {
	//handle command arguments
	char* server_name = malloc(sizeof(char) * BUFFER_SIZE);
	if (server_name == NULL) {
		perror("server_name malloc failed");
		exit(1);
	}
	int server_port = DEFAULT_PORT;
	if (argc <= 1) {
		printf("Usage: server name [port]\n");
		exit(1);
	}
	if (argc >= 2) {
		server_name = argv[1];
	}

	//create socket
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		perror("socket failed");
		exit(1);
	}
	
	//set up server info
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(server_port);
	
	//bind to establish socket's interface and port
	if (bind(server_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1) {
		perror("bind failed");
		exit(1);
	}

	//listen
	if (listen(server_fd, LISTENQ) == -1) {
		perror("listen failed");
		exit(1);
	}

	//accept
	int sock_fd = accept(server_fd, (struct sockaddr *) NULL, NULL);
	while (1) {
		//select
		fd_set set;
		FD_ZERO(&set);
		int max_fd = sock_fd + 1;
		struct timeval timeout;
		timeout.tv_sec = TIMEOUT_SECS;
		timeout.tv_usec = 0;

		FD_SET(STDIN_FILENO, &set);
		FD_SET(sock_fd, &set);
		if (select(max_fd, &set, NULL, NULL, &timeout) == -1) {
			perror("select failed");
			exit(1);
		}

		char child_buf[BUFFER_SIZE];
		int n = 0;
		if (FD_ISSET(STDIN_FILENO, &set)) {
			n = read(STDIN_FILENO, child_buf, BUFFER_SIZE);

			char server_name_buf[BUFFER_SIZE];
			int server_name_size = snprintf(server_name_buf, BUFFER_SIZE, "%s: ", server_name);
			if (write(sock_fd, server_name_buf, server_name_size) == -1) {
				perror("write failed");
				exit(1);
			}
			if (write(sock_fd, child_buf, n) == -1) {
				perror("write failed");
				exit(1);
			}
		}
		if (FD_ISSET(sock_fd, &set)) {
			n = read(sock_fd, child_buf, BUFFER_SIZE);
			if (write(STDOUT_FILENO, child_buf, n) == -1) {
				perror("write failed");
				exit(1);
			}
		}

	}
	close(sock_fd);


	return 0;
}