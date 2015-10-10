//Minhtri Tran
//Unix Programming
//Assignment 4 - Simple shell with piping

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

const unsigned int INPUT_BUFFER_SIZE = 800;
const size_t ARGV_INITIAL_SIZE = 2;
const unsigned int ARGV_REALLOC_FACTOR = 2;
const unsigned int PIPE_BUFFER_SIZE = 800;

typedef struct {
	char* command_string;
	char** argv;
	int argc;
	size_t alloc_size;
} command;

char* setPromptString();
char* getInputString();
char** getCommandFromInputString(char* inputString, int* command_argc, size_t* alloc_size);
void resize2DArray(char*** array, size_t* alloc_size);
void free2DArray(char** array, size_t alloc_size);


int main() {
	while(1) {
		char* prompt = setPromptString();
		printf("%s", prompt);

		char* input_string = getInputString();

		int num_commands = 0;
		command* commands = malloc(sizeof(command) * 2);


		char* pch = strtok(input_string, "|");

		//Loop through string by | delimiter
		while (pch != NULL) {
			commands[num_commands].command_string =  malloc(strlen(pch) * sizeof(char));
			if (commands[num_commands].command_string == NULL) {
				perror("commands[num_commands].command_string malloc failed");
			}

			//add token to array
			strcpy(commands[num_commands].command_string, pch);

			num_commands++;

			//resize array if not big enough
			// if (num_commands > (*alloc_size)) {
			// 	resize2DArray(&command_argv, alloc_size);
			// }

			pch = strtok(NULL, "|");
		}

		//Initialize variables
		commands[0].alloc_size = ARGV_INITIAL_SIZE;
		commands[0].argc = 0;
		commands[0].argv = getCommandFromInputString(commands[0].command_string, 
													&commands[0].argc, 
													&commands[0].alloc_size);

		commands[1].alloc_size = ARGV_INITIAL_SIZE;
		commands[1].argc = 0;
		commands[1].argv = getCommandFromInputString(commands[1].command_string, 
													&commands[1].argc, 
													&commands[1].alloc_size);
		
		if (commands[0].argv[0] == NULL) {
			continue;
		}
		//Handle exit command
		else if (strcmp(commands[0].argv[0], "exit") == 0)  {
			exit(0);
		}
		//Handle cd command
		else if (strcmp(commands[0].argv[0], "cd") == 0) {
			char* path = (commands[0].argv[1] != NULL) ? commands[0].argv[1] : getenv("HOME");
			if (chdir(path) == -1) {
				perror("Chdir failed");
			}
		}
		else {
			char buffer[PIPE_BUFFER_SIZE];
			for (int i = 0; i < num_commands; ++i) {
				//Create pipe
				int fd[2];
				if (pipe(fd) == -1) {
					perror("Pipe failed");
				}

				//Fork
				pid_t fork_pid = fork();

				if (fork_pid == -1) {
					perror("Fork failed");
				}
				//Child process
				else if (fork_pid == 0) {
					
					//close stdin, duplicate pipe input to stdin
					//except when this is first command
					if (i != 0) {
						int wbytes = write(fd[1], buffer, strlen(buffer));
					 	fprintf(stdout, "Bytes written: %d\n", wbytes);
						dup2(fd[0], 0);
					}

					//close stdout, duplicate pipe output to stdout
					if (i != (num_commands - 1)) {
						dup2(fd[1], 1);
					}

					execvp(commands[i].argv[0], commands[i].argv);
					perror("Command execution failed");
					exit(1);
				}
				//Parent process
				else {

					//write buffer to fd[1]
					// if (i != 0) {
					// 	int wbytes = write(fd[1], buffer, strlen(buffer));
					// 	fprintf(stdout, "Bytes written: %d\n", wbytes);
					// }

					wait(NULL);

					//close write
					close(fd[1]);

					//read fd[0] to buffer
					if (i != (num_commands - 1)) {
						int rbytes = read(fd[0], buffer, PIPE_BUFFER_SIZE);
						fprintf(stdout, "Bytes read: %d\n", rbytes);
					}

					//close read
					close(fd[0]);
					
					free2DArray(commands[i].argv, commands[i].alloc_size);
				}
			}
			
		}
	}
	
	return 0;
}

//Set and return a promp string
char* setPromptString() {
	char* prompt = getenv("PS1");
	if (prompt == NULL) {
		prompt = "#> ";
	}
	return prompt;
}

//Read the shell input into a string and return it
char* getInputString() {
	char* input_string = malloc(sizeof(char) * INPUT_BUFFER_SIZE);
	if (input_string == NULL) {
		perror("input_string malloc failed");
	}
	if (fgets(input_string, INPUT_BUFFER_SIZE, stdin) == NULL) {
		perror("Bad input string");
	}

	//Remove trailing newline character from fgets
	char *pos;
	if ((pos=strchr(input_string, '\n')) != NULL) {
	    *pos = '\0';
	}

	return input_string;
}

/*
Tokenize input string and turn it into an array
command_argc and alloc_size gets modified
Input:
	inputString: a space-delimited input string
	command_argc: the number of current elements in command_argv
	alloc_size: the starting allocation size of command_argv
Output:
	Returns an array filled with the command and arguments
*/
char** getCommandFromInputString(char* inputString, int* command_argc, size_t* alloc_size) {
	char** command_argv = malloc(sizeof(char*) * (*alloc_size));
	if (command_argv == NULL) {
		perror("command_argv malloc failed");
	}

	char* pch = strtok(inputString, " ");

	//Loop through string by space delimiter
	while (pch != NULL) {
		//add token to array
		command_argv[(*command_argc)] =  malloc(strlen(pch) * sizeof(char));
		if (command_argv[(*command_argc)] == NULL) {
			perror("command_argv[(*command_argc)] malloc failed");
		}

		strcpy(command_argv[(*command_argc)], pch);

		(*command_argc)++;

		//resize array if not big enough
		if ((*command_argc) > (*alloc_size)) {
			resize2DArray(&command_argv, alloc_size);
		}

		pch = strtok(NULL, " ");
	}

	command_argv[(*command_argc)] = NULL;

	return command_argv;
}

/*
Resizes a 2D array by a factor
array and alloc_size gets modified
Input:
	array: a 2D array to resize
	alloc_size: current allocation size
*/
void resize2DArray(char*** array, size_t* alloc_size) {
	(*alloc_size) = (*alloc_size) * ARGV_REALLOC_FACTOR;
	(*array) = realloc(*array, sizeof((*array)[0]) * (*alloc_size));
	if ((*array) == NULL) {
		perror("(*array) realloc failed");
	}
}

/*
Free up a 2D array given the alloc_size
Input:
	array: a 2D array to free up
	alloc_size: array's allocated size on the heap
*/
void free2DArray(char** array, size_t alloc_size) {
	for(int i = 0; i < alloc_size; i++) {
		free(array[i]);
	}
	free(array);
}