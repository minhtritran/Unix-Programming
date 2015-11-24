//Minhtri Tran
//Unix Programming
//Assignment 7 - Networks and I/O Multiplexing
//1-to-1 Chat Server

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
#include <stdbool.h>

#define	LISTENQ 1024
#define BUFFER_SIZE 1024
#define TIMEOUT_SECS 180
#define DEFAULT_PORT 13000

void handleInput(int argc, char* argv[], char** server_name, int* server_port);
int setUpServer(int server_port);
bool handleOutgoingMsg(int sock_fd, char* msg_buf, char* server_name);
bool handleIncomingMsg(int sock_fd, char* msg_buf);

int main(int argc, char *argv[]) {
	char* server_name;
	int server_port;
	handleInput(argc, argv, &server_name, &server_port);

	int server_fd = setUpServer(server_port);

	while (1) {
		//accept
		printf("Waiting for client connection..\n");
		int sock_fd = accept(server_fd, (struct sockaddr *) NULL, NULL);
		printf("Connection established. Start chatting!\n");
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

			char msg_buf[BUFFER_SIZE];
			memset(&msg_buf, 0, sizeof(msg_buf));
			if (FD_ISSET(STDIN_FILENO, &set)) {
				if (!handleOutgoingMsg(sock_fd, msg_buf, server_name)) {
					printf("Closing server..\n");
					free(server_name);
					exit(0);
				}
			}
			if (FD_ISSET(sock_fd, &set)) {
				if (!handleIncomingMsg(sock_fd, msg_buf)) {
					printf("Connection ended.\n");
					break;
				}
			}
		}
	}
	return 0;
}

//Handle command arguments
void handleInput(int argc, char* argv[], char** server_name, int* server_port) {
	*server_name = malloc(sizeof(char) * BUFFER_SIZE);
	if (*server_name == NULL) {
		perror("server_name malloc failed");
		exit(1);
	}
	*server_port = DEFAULT_PORT;
	if (argc <= 1) {
		printf("Usage: server name [port]\n");
		exit(1);
	}
	if (argc >= 2) {
		strcpy(*server_name, argv[1]);
	}
	if (argc >= 3) {
		char* endptr;
		*server_port = strtol(argv[2], &endptr, 10);
		if (*endptr != '\0') {
			fprintf(stderr, "%s is not a valid port.  Defaulting to port %d.\n", argv[2], DEFAULT_PORT);
			*server_port = DEFAULT_PORT;
		}
	}
}

//Set up server using the given port
//Returns the server file descriptor
int setUpServer(int server_port) {
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

	return server_fd;
}

//Handle outgoing messages
//Returns false if server (you) sends a 'quit' message, true otherwise
bool handleOutgoingMsg(int sock_fd, char* msg_buf, char* server_name) {
	int n = read(STDIN_FILENO, msg_buf, BUFFER_SIZE);

	//handle server quitting
	if (strcmp(msg_buf, "quit\n") == 0) {
		if (write(sock_fd, msg_buf, n) == -1) {
			perror("write failed");
			exit(1);
		}
		close(sock_fd);
		return false;
	}

	char server_message[BUFFER_SIZE];
	int server_message_n = snprintf(server_message, BUFFER_SIZE, "%s: %s", server_name, msg_buf);
	if (write(sock_fd, server_message, server_message_n) == -1) {
		perror("write failed");
		exit(1);
	}

	return true;
}

//Handle incoming messages
//Returns false if client sends a 'quit' message, true otherwise
bool handleIncomingMsg(int sock_fd, char* msg_buf) {
	int n = read(sock_fd, msg_buf, BUFFER_SIZE);

	//handle client quitting
	if (strcmp(msg_buf, "quit\n") == 0) {
		close(sock_fd);
		return false;
	}

	if (write(STDOUT_FILENO, msg_buf, n) == -1) {
		perror("write failed");
		exit(1);
	}

	return true;
}