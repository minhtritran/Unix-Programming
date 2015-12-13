//Minhtri Tran
//Unix Programming
//Assignment 8 - Multi-threading
//Chat Room Server

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
#include <pthread.h>

#define	LISTENQ 1024
#define BUFFER_SIZE 1024
#define TIMEOUT_SECS 180
#define DEFAULT_PORT 13000
#define MAX_USERS 20

struct User {
	char name[BUFFER_SIZE];
	int fd;
	pthread_t tid;
};

struct Queue {
	char* items[BUFFER_SIZE];
	int front;
	int back;
	int num_items;
};

struct ChatSystem {
	int server_fd;
	struct User users[MAX_USERS];
	int num_users;
	struct Queue msg_queue;
};

void handleInput(int argc, char* argv[], char** server_name, int* server_port);
int setUpServer(int server_port);
bool handleOutgoingMsg(int sock_fd, char* msg_buf, char* server_name);
bool handleIncomingMsg(int sock_fd, char* msg_buf);

bool insert(struct Queue* msg_queue, char* message);
char* pop(struct Queue* msg_queue);

void* adder_thread(void* p);
void* user_thread(void* p);

int main(int argc, char *argv[]) {
	char* server_name;
	int server_port;
	handleInput(argc, argv, &server_name, &server_port);

	//initialize chat variables
	struct ChatSystem chat;
	chat.server_fd = setUpServer(server_port);
	chat.num_users = 0;
	chat.msg_queue.front = 0;
	chat.msg_queue.back = 0;
	chat.msg_queue.num_items = 0;

	pthread_t adder_tid;
	pthread_create(&adder_tid, NULL, adder_thread, (void*)&chat);
	printf("Waiting for client connection..\n");

	pthread_join(adder_tid, NULL);

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

bool insert(struct Queue* msg_queue, char* message) {
	//check to see if queue is full
	if ((*msg_queue).num_items > BUFFER_SIZE) {
		return false;
	}

	//If front is past the end of the array, set it back to the beginning
	if ((*msg_queue).front >= BUFFER_SIZE) {
		(*msg_queue).front = 0;
	}

	char buf[BUFFER_SIZE];
	(*msg_queue).items[(*msg_queue).front] = buf;
	strcpy((*msg_queue).items[(*msg_queue).front], message);
	
	(*msg_queue).front++;
	(*msg_queue).num_items++;

	return true;
}

char* pop(struct Queue* msg_queue) {
	if ((*msg_queue).num_items == 0) {
		return NULL;
	}

	//If back is past the end of the array, set it back to the beginning
	if ((*msg_queue).back >= BUFFER_SIZE) {
		(*msg_queue).back = 0;
	}

	(*msg_queue).back++;
	(*msg_queue).num_items--;
	return (*msg_queue).items[(*msg_queue).back - 1];
}

void* adder_thread(void* p) {
	struct ChatSystem* chat = (struct ChatSystem *)p;
	while (1) {
		//accept
		int sock_fd = accept((*chat).server_fd, (struct sockaddr *) NULL, NULL);
		
		//create user and fill it with the user name and associated fd
		struct User user;
		int n = read(sock_fd, user.name, BUFFER_SIZE);
		if (n == -1) {
			perror("read failed");
			exit(1);
		}
		user.fd = sock_fd;

		(*chat).users[(*chat).num_users] = user;

		pthread_create(&(*chat).users[(*chat).num_users].tid, NULL, user_thread, (void*)chat);
		(*chat).num_users++;

		printf("Connection established. Start chatting!\n");
	}

	return NULL;
}

void* user_thread(void* p) {
	struct ChatSystem* chat = (struct ChatSystem *)p;
	
	pthread_t self = pthread_self();

	size_t user_index = -1;
	for (size_t i = 0; i < (*chat).num_users; i++) {
		if ((*chat).users[i].tid == self) {
			user_index = i;
		}
	}

	if (user_index == -1) {
		printf("%s\n", "No such user");
	}

	char signin_msg[BUFFER_SIZE];
	snprintf(signin_msg, BUFFER_SIZE, "%s has signed in.", (*chat).users[user_index].name);
	
	insert(&(*chat).msg_queue, signin_msg);
	printf("%s\n", (*chat).msg_queue.items[0]);

	while(1) {
		sleep(1);
		printf("%s\n", (*chat).users[user_index].name);
	}

	return NULL;
}