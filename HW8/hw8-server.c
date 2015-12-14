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
#define MAX_USERS 500

struct User {
	char name[BUFFER_SIZE];
	int fd;
	pthread_t tid;
	bool alive;
};

struct Queue {
	char* items[BUFFER_SIZE];
	int senders[BUFFER_SIZE];
	int front;
	int back;
	int num_items;
};

struct ChatSystem {
	int server_fd;
	struct User users[MAX_USERS];
	int num_users;
	struct Queue msg_queue;
	pthread_mutex_t users_lock;
	pthread_mutex_t msg_queue_lock;
	pthread_cond_t msq_queue_cond_c;
	pthread_cond_t msq_queue_cond_p;
	int terminatee_fd;
	pthread_mutex_t terminate_lock;
	pthread_cond_t terminate_cond_c;
	pthread_cond_t terminate_cond_p;
};

void handleInput(int argc, char* argv[], int* server_port);
int setUpServer(int server_port);

void initializeChat(struct ChatSystem* chat, int server_port);
void insertMessage(struct ChatSystem* chat, char* message, int sender_fd);
void popMessage(struct ChatSystem* chat, char** message, int* sender_fd);

void* adder_thread(void* p);
void* user_thread(void* p);
void* broadcaster_thread(void* p);
void* remover_thread(void* p);

int main(int argc, char *argv[]) {
	int server_port;
	handleInput(argc, argv, &server_port);

	struct ChatSystem chat;
	initializeChat(&chat, server_port);

	pthread_t adder_tid;
	pthread_create(&adder_tid, NULL, adder_thread, (void*)&chat);

	pthread_t broadcaster_tid;
	pthread_create(&broadcaster_tid, NULL, broadcaster_thread, (void*)&chat);

	pthread_t remover_tid;
	pthread_create(&remover_tid, NULL, remover_thread, (void*)&chat);

	printf("Waiting for client connection on port %d..\n", server_port);

	pthread_join(adder_tid, NULL);
	pthread_join(broadcaster_tid, NULL);
	pthread_join(remover_tid, NULL);
	pthread_mutex_destroy(&chat.users_lock);
	pthread_mutex_destroy(&chat.msg_queue_lock);
	pthread_mutex_destroy(&chat.terminate_lock);

	return 0;
}

//Initialize chat variables
void initializeChat(struct ChatSystem* chat, int server_port) {
	(*chat).server_fd = setUpServer(server_port);
	(*chat).num_users = 0;
	(*chat).msg_queue.front = 0;
	(*chat).msg_queue.back = 0;
	(*chat).msg_queue.num_items = 0;
	(*chat).users_lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	(*chat).msg_queue_lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	(*chat).msq_queue_cond_c = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	(*chat).msq_queue_cond_p = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	(*chat).terminatee_fd = -1;
	(*chat).terminate_lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	(*chat).terminate_cond_c = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	(*chat).terminate_cond_p = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
}

//Handle command arguments
void handleInput(int argc, char* argv[], int* server_port) {
	*server_port = DEFAULT_PORT;
	if (argc >= 2) {
		char* endptr;
		*server_port = strtol(argv[1], &endptr, 10);
		if (*endptr != '\0') {
			fprintf(stderr, "%s is not a valid port.  Using default port.\n", argv[1]);
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

//Insert given message into message queue and keep track of the sender's fd
void insertMessage(struct ChatSystem* chat, char* message, int sender_fd) {
	pthread_mutex_lock(&(*chat).msg_queue_lock);
	while((*chat).msg_queue.num_items == BUFFER_SIZE) {
		pthread_cond_wait(&(*chat).msq_queue_cond_p, &(*chat).msg_queue_lock);
	}

	//If front is past the end of the array, set it back to the beginning
	if ((*chat).msg_queue.front >= BUFFER_SIZE) {
		(*chat).msg_queue.front = 0;
	}

	char buf[BUFFER_SIZE];
	(*chat).msg_queue.items[(*chat).msg_queue.front] = buf;
	strcpy((*chat).msg_queue.items[(*chat).msg_queue.front], message);
	(*chat).msg_queue.senders[(*chat).msg_queue.front] = sender_fd;
	
	(*chat).msg_queue.front++;
	(*chat).msg_queue.num_items++;

	pthread_cond_signal(&(*chat).msq_queue_cond_c);
	pthread_mutex_unlock(&(*chat).msg_queue_lock);
}

//Set the given message variable to be the earliest message in queue
//Also set the given sender_fd to hold the message sender's fd
void popMessage(struct ChatSystem* chat, char** message, int* sender_fd) {
	pthread_mutex_lock(&(*chat).msg_queue_lock);
	while((*chat).msg_queue.num_items == 0) {
		pthread_cond_wait(&(*chat).msq_queue_cond_c, &(*chat).msg_queue_lock);
	}

	//If back is past the end of the array, set it back to the beginning
	if ((*chat).msg_queue.back >= BUFFER_SIZE) {
		(*chat).msg_queue.back = 0;
	}

	(*chat).msg_queue.back++;
	(*chat).msg_queue.num_items--;

	strcpy(*message, (*chat).msg_queue.items[(*chat).msg_queue.back - 1]);
	*sender_fd = (*chat).msg_queue.senders[(*chat).msg_queue.back - 1];

	memset(&(*chat).msg_queue.items[(*chat).msg_queue.back - 1], 0, BUFFER_SIZE);

	pthread_cond_signal(&(*chat).msq_queue_cond_p);
	pthread_mutex_unlock(&(*chat).msg_queue_lock);

}

//Thread that handles adding a user
void* adder_thread(void* p) {
	struct ChatSystem* chat = (struct ChatSystem *)p;
	while (1) {
		int sock_fd = accept((*chat).server_fd, (struct sockaddr *) NULL, NULL);
		
		//create user and fill it with the user name and associated fd
		struct User user;
		int n = read(sock_fd, user.name, BUFFER_SIZE);
		if (n == -1) {
			perror("read failed");
			exit(1);
		}
		user.fd = sock_fd;
		user.alive = true;

		//insert a sign in message to the message queue
		char signin_msg[BUFFER_SIZE];
		snprintf(signin_msg, BUFFER_SIZE, "%s signed in.\n", user.name);
		insertMessage(chat, signin_msg, user.fd);

		pthread_mutex_lock(&(*chat).users_lock);

		//write who is online to socket
		for (size_t i = 0; i < (*chat).num_users; i++) {
			if ((*chat).users[i].alive) {
				char msg_buf[BUFFER_SIZE];
				int msg_buf_n = snprintf(msg_buf, BUFFER_SIZE, "%s is online.\n", (*chat).users[i].name);
				if (write(user.fd, msg_buf, msg_buf_n) == -1) {
					perror("write failed");
					exit(1);
				}	
			}
		}

		//add user to users array
		(*chat).users[(*chat).num_users] = user;

		//create a user thread
		pthread_create(&(*chat).users[(*chat).num_users].tid, NULL, user_thread, (void*)chat);
		
		pthread_mutex_unlock(&(*chat).users_lock);

		(*chat).num_users++;
	}

	return NULL;
}

//Thread that handles user inputs
void* user_thread(void* p) {
	struct ChatSystem* chat = (struct ChatSystem *)p;
	
	//get this thread's user index
	pthread_t self = pthread_self();
	size_t user_index = -1;
	pthread_mutex_lock(&(*chat).users_lock);
	for (size_t i = 0; i < (*chat).num_users; i++) {
		if ((*chat).users[i].tid == self) {
			user_index = i;
		}
	}
	pthread_mutex_unlock(&(*chat).users_lock);

	if (user_index == -1) {
		printf("%s\n", "No such user");
	}

	while(1) {
		char msg_buf[BUFFER_SIZE];
		memset(&msg_buf, 0, sizeof(msg_buf));
		int n = read((*chat).users[user_index].fd, msg_buf, BUFFER_SIZE);
		if (n == -1) {
			perror("read failed");
			exit(1);
		}
		if (strcmp(msg_buf, "quit\n") == 0) {
			//set the terminatee tid to be this thread
			pthread_mutex_lock(&(*chat).terminate_lock);
			while((*chat).terminatee_fd > -1) {
				pthread_cond_wait(&(*chat).terminate_cond_p, &(*chat).terminate_lock);
			}

			(*chat).terminatee_fd = (*chat).users[user_index].fd;

			pthread_cond_signal(&(*chat).terminate_cond_c);
			pthread_mutex_unlock(&(*chat).terminate_lock);
			break;
		}

		insertMessage(chat, msg_buf, (*chat).users[user_index].fd);
	}

	return NULL;
}

//Thread that handles broadcasting messages from message queue to all users
void* broadcaster_thread(void* p) {
	struct ChatSystem* chat = (struct ChatSystem *)p;

	while(1) {
		char* message = malloc(sizeof(char) * BUFFER_SIZE);
		if (message == NULL) {
			perror("malloc mesasge failed");
			exit(1);
		}

		int sender_fd;
		popMessage(chat, &message, &sender_fd);

		//broadcast to users
		pthread_mutex_lock(&(*chat).users_lock);
		for (size_t i = 0; i < (*chat).num_users; i++) {
			if ((*chat).users[i].alive && (*chat).users[i].fd != sender_fd) {
				if (write((*chat).users[i].fd, message, BUFFER_SIZE) == -1) {
					perror("write failed");
					exit(1);
				}
			}
		}
		pthread_mutex_unlock(&(*chat).users_lock);
		
		//print to server
		printf("%s", message);
		
		free(message);
	}

	return NULL;
}

//Thread that handles removing a user
void* remover_thread(void* p) {
	struct ChatSystem* chat = (struct ChatSystem *)p;

	while(1) {
		pthread_mutex_lock(&(*chat).terminate_lock);
		while((*chat).terminatee_fd == -1) {
			pthread_cond_wait(&(*chat).terminate_cond_c, &(*chat).terminate_lock);
		}

		pthread_mutex_lock(&(*chat).users_lock);

		//get the users index of the thread to be terminated
		size_t index = -1;
		for (size_t i = 0; i < (*chat).num_users; i++) {
			if ((*chat).users[i].fd == (*chat).terminatee_fd) {
				index = i;
				break;
			}
		}

		pthread_join((*chat).users[index].tid, NULL);
		(*chat).terminatee_fd = -1;
		(*chat).users[index].alive = false;

		//insert sign off message into queue
		char signoff_msg[BUFFER_SIZE];
		snprintf(signoff_msg, BUFFER_SIZE, "%s signed off.\n", (*chat).users[index].name);
		insertMessage(chat, signoff_msg, (*chat).users[index].fd);

		pthread_mutex_unlock(&(*chat).users_lock);

		pthread_cond_signal(&(*chat).terminate_cond_p);
		pthread_mutex_unlock(&(*chat).terminate_lock);
	}
	return NULL;
}