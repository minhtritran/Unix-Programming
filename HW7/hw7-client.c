//Minhtri Tran
//Unix Programming
//Assignment 7 - Networks and I/O Multiplexing
//1-to-1 Chat Client

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024
#define TIMEOUT_SECS 180

void handleInput(int argc, char* argv[], char** client_name, char* server_ip, int* server_port);
int connectToServer(char* server_ip, int server_port);
bool handleOutgoingMsg(int sock_fd, char* msg_buf, char* client_name);
bool handleIncomingMsg(int sock_fd, char* msg_buf);

int main(int argc, char *argv[]) {
	char* client_name;
	char server_ip[BUFFER_SIZE];
	int server_port;
	handleInput(argc, argv, &client_name, server_ip, &server_port);

	int sock_fd = connectToServer(server_ip, server_port);
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

		//if fd == 0 means timeout
		char msg_buf[BUFFER_SIZE];
		memset(&msg_buf, 0, sizeof(msg_buf));
		if (FD_ISSET(STDIN_FILENO, &set)) {
			if (!handleOutgoingMsg(sock_fd, msg_buf, client_name)) {
				printf("Quitting..\n");
				break;
			}
		}
		if (FD_ISSET(sock_fd, &set)) {
			if (!handleIncomingMsg(sock_fd, msg_buf)) {
				printf("Server closed.\n");
				break;
			}
		}
	}

	free(client_name);
	close(sock_fd);

	return 0;
}

//Handle command arguments
void handleInput(int argc, char* argv[], char** client_name, char* server_ip, int* server_port) {
	*client_name = malloc(sizeof(char) * BUFFER_SIZE);
	if (*client_name == NULL) {
		perror("client_name malloc failed");
		exit(1);
	}
	if (argc <= 3) {
		printf("Usage: client name ip port\n");
		exit(1);
	}
	if (argc >= 2) {
		strcpy(*client_name, argv[1]);
	}
	if (argc >= 3) {
		strcpy(server_ip, argv[2]);
	}
	if (argc >= 4) {
		char* endptr;
		*server_port = strtol(argv[3], &endptr, 10);
		if (*endptr != '\0') {
			fprintf(stderr, "%s is not a valid port.\n", argv[3]);
			exit(1);
		}
	}
}

//Connect to the given server ip and port
//Returns a socket file descriptor
int connectToServer(char* server_ip, int server_port) {
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
	int pton_return_val = inet_pton(AF_INET, server_ip, &servaddr.sin_addr);
	if (pton_return_val == 0) {
		fprintf(stderr, "%s is an invalid ip address.\n", server_ip);
		exit(1);
	}
	else if (pton_return_val == -1) {
		perror("inet_pton failed");
		exit(1);
	}

	//connect to server
	if (connect(sock_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1) {
		perror("connect failed");
		exit(1);
	}

	return sock_fd;
}

//Handle outgoing messages
//Returns false if client (you) sends a 'quit' message, true otherwise
bool handleOutgoingMsg(int sock_fd, char* msg_buf, char* client_name) {
	//read from socket
	int n = read(STDIN_FILENO, msg_buf, BUFFER_SIZE);

	//handle client quitting
	if (strcmp(msg_buf, "quit\n") == 0) {
		if (write(sock_fd, msg_buf, n) == -1) {
			perror("write failed");
			exit(1);
		}
		close(sock_fd);
		return false;
	}

	//write message to socket
	char client_message[BUFFER_SIZE];
	int client_message_n = snprintf(client_message, BUFFER_SIZE, "%s: %s", client_name, msg_buf);
	if (write(sock_fd, client_message, client_message_n) == -1) {
		perror("write failed");
		exit(1);
	}

	return true;
}

//Handle incoming messages
//Returns false if server sends a 'quit' message, true otherwise
bool handleIncomingMsg(int sock_fd, char* msg_buf) {
	int n = read(sock_fd, msg_buf, BUFFER_SIZE);

	//handle server quitting
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