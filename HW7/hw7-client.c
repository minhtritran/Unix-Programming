//Minhtri Tran
//Unix Programming
//Assignment 7 - Networks and I/O Multiplexing
//Client

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>

#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024
#define TIMEOUT_SECS 180

int main(int argc, char *argv[]) {
	//handle command arguments
	char* client_name = malloc(sizeof(char) * BUFFER_SIZE);
	if (client_name == NULL) {
		perror("client_name malloc failed");
		exit(1);
	}
	int server_port = 13000;
	if (argc <= 1) {
		printf("Usage: client name [port]\n");
		exit(1);
	}
	if (argc >= 2) {
		client_name = argv[1];
	}

	//create socket
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd == -1) {
		perror("socket failed");
		exit(1);
	}
	
	//set up server info
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(server_port);
	if (inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr) <= 0) {
		perror("inet_pton failed");
		exit(1);
	}

	//connect to server
	if (connect(sock_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1) {
		perror("connect failed");
		exit(1);
	}


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

		//if fd == 0 means timeout
		char child_buf[BUFFER_SIZE];
		int n = 0;
		if (FD_ISSET(STDIN_FILENO, &set)) {
			n = read(STDIN_FILENO, child_buf, BUFFER_SIZE);

			char client_name_buf[BUFFER_SIZE];
			int client_name_size = snprintf(client_name_buf, BUFFER_SIZE, "%s: ", client_name);
			if (write(sock_fd, client_name_buf, client_name_size) == -1) {
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