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
#include <stdbool.h>

const unsigned int INPUT_BUFFER_SIZE = 800;
const size_t ARGV_INITIAL_SIZE = 2;
const unsigned int ARGV_REALLOC_FACTOR = 2;
const size_t COMMANDS_INITIAL_SIZE = 1;
const unsigned int COMMANDS_REALLOC_FACTOR = 2;

typedef struct {
	char* command_string;
	char** argv;
	int argc;
	size_t alloc_size;
} command;

char* setPromptString();
char* getInputString();
void fillCommandArray(	char* input_string, command** commands,
						int* num_commands, size_t* commands_alloc_size);
char** getCommandArgv(char* inputString, int* command_argc, size_t* alloc_size);
void resize2DCharArray(char*** array, size_t* alloc_size);
void resizeCommandArray(command** array, size_t* alloc_size);
bool handleShellCommand(command cmd);
int** createPipes(int num_commands);
void executeCommand(int** fds, int num_commands, command cmd, int index);
void closeAllPipes(int** fds, int num_commands);
void freeCommandArray(command* commandArray, int num_commands);
void waitForAllChildProcesses(int num_commands);
void free2DCharArray(char** array, size_t alloc_size);

int main() {
	while(1) {
		char* prompt = setPromptString();
		printf("%s", prompt);

		char* input_string = getInputString();

		//Initialize array to hold commands
		size_t commands_alloc_size = COMMANDS_INITIAL_SIZE;
		int num_commands = 0;
		command* commands = malloc(sizeof(command) * commands_alloc_size);

		fillCommandArray(input_string, &commands, &num_commands, &commands_alloc_size);

		bool wasShellCommand = handleShellCommand(commands[0]);

		if (wasShellCommand == false) {
			int** fds = createPipes(num_commands);

			for (int i = 0; i < num_commands; ++i) {
				pid_t fork_pid = fork();
				if (fork_pid == -1) {
					perror("Fork failed");
					exit(1);
				}
				//Child process
				else if (fork_pid == 0) {
					executeCommand(fds, num_commands, commands[i], i);
				}
			}

			//Clean up the shell process
			closeAllPipes(fds, num_commands);
			waitForAllChildProcesses(num_commands);
			freeCommandArray(commands, num_commands);
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
		exit(1);
	}
	if (fgets(input_string, INPUT_BUFFER_SIZE, stdin) == NULL) {
		perror("Bad input string");
		exit(1);
	}

	//Remove trailing newline character from fgets
	char *pos;
	if ((pos=strchr(input_string, '\n')) != NULL) {
	    *pos = '\0';
	}

	return input_string;
}

/*
Fills the commands array based on the input_string
Input:
	input_string: a string to be tokenized into commands by "|"
	commands: a pointer to an array that will be filled with commands
	num_commands: a pointer to the number of commands
	commands_alloc_size: a pointer to the alloc_size
*/
void fillCommandArray(	char* input_string, command** commands,
						int* num_commands, size_t* commands_alloc_size) {
	char* pch = strtok(input_string, "|");
	//Loop through string by | delimiter
	while (pch != NULL) {
		(*commands)[(*num_commands)].command_string =  malloc(strlen(pch) * sizeof(char));
		if ((*commands)[(*num_commands)].command_string == NULL) {
			perror("commands[(*num_commands)].command_string malloc failed");
			exit(1);
		}

		//add token to array
		strcpy((*commands)[(*num_commands)].command_string, pch);

		(*num_commands)++;

		//resize array if not big enough
		if ((*num_commands) > (*commands_alloc_size)) {
			resizeCommandArray(commands, commands_alloc_size);
		}

		pch = strtok(NULL, "|");
	}

	for (int i = 0; i < (*num_commands); ++i) {
		//Initialize variables
		(*commands)[i].alloc_size = ARGV_INITIAL_SIZE;
		(*commands)[i].argc = 0;
		(*commands)[i].argv = getCommandArgv((*commands)[i].command_string, 
														&(*commands)[i].argc, 
														&(*commands)[i].alloc_size);
	}
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
char** getCommandArgv(char* inputString, int* command_argc, size_t* alloc_size) {
	char** command_argv = malloc(sizeof(char*) * (*alloc_size));
	if (command_argv == NULL) {
		perror("command_argv malloc failed");
		exit(1);
	}

	char* pch = strtok(inputString, " ");

	//Loop through string by space delimiter
	while (pch != NULL) {
		//add token to array
		command_argv[(*command_argc)] =  malloc(strlen(pch) * sizeof(char));
		if (command_argv[(*command_argc)] == NULL) {
			perror("command_argv[(*command_argc)] malloc failed");
			exit(1);
		}

		strcpy(command_argv[(*command_argc)], pch);

		(*command_argc)++;

		//resize array if not big enough
		if ((*command_argc) > (*alloc_size)) {
			resize2DCharArray(&command_argv, alloc_size);
		}

		pch = strtok(NULL, " ");
	}

	command_argv[(*command_argc)] = NULL;

	return command_argv;
}

/*
Resizes a 2D char array by a factor
array and alloc_size gets modified
Input:
	array: a 2D char array to resize
	alloc_size: current allocation size
*/
void resize2DCharArray(char*** array, size_t* alloc_size) {
	(*alloc_size) = (*alloc_size) * ARGV_REALLOC_FACTOR;
	(*array) = realloc(*array, sizeof((*array)[0]) * (*alloc_size));
	if ((*array) == NULL) {
		perror("(*array) realloc failed");
		exit(1);
	}
}

/*
Resizes a command array by a factor
array and alloc_size gets modified
Input:
	array: a command array to resize
	alloc_size: current allocation size
*/
void resizeCommandArray(command** array, size_t* alloc_size) {
	(*alloc_size) = (*alloc_size) * COMMANDS_REALLOC_FACTOR;
	(*array) = realloc(*array, sizeof((*array)[0]) * (*alloc_size));
	if ((*array) == NULL) {
		perror("(*array) realloc failed");
		exit(1);
	}
}

/*
Takes a command and process it as a shell command
Return:	true if the command was processed
		false otherwise
*/
bool handleShellCommand(command cmd) {
	if (cmd.argv[0] == NULL) {
		return true;
	}
	//Handle exit command
	else if (strcmp(cmd.argv[0], "exit") == 0)  {
		exit(0);
	}
	//Handle cd command
	else if (strcmp(cmd.argv[0], "cd") == 0) {
		char* path = (cmd.argv[1] != NULL) ? cmd.argv[1] : getenv("HOME");
		if (chdir(path) == -1) {
			perror("Chdir failed");
			exit(1);
		}
		return true;
	}
	else {
		return false;
	}
}

/*
Create pipes corresponding the number of commands, one for each child process
Return an array of 2-int array file descriptors
*/
int** createPipes(int num_commands) {
	//Create pipes for each child process
	int** fds = malloc(sizeof(int*) * num_commands);
	if (fds == NULL) {
		perror("fds malloc failed");
		exit(1);
	}
	for (int i = 0; i < num_commands; ++i) {
		int* fd = malloc(sizeof(int) * 2);
		if (fd == NULL) {
			perror("fd malloc failed");
			exit(1);
		}

		fds[i] = fd;
		if (pipe(fds[i]) == -1) {
			perror("Pipe failed");
			exit(1);
		}
	}
	return fds;
}

/*
Execute command while taking pipes into account
Input:
	fds: an array to 2-int array file descriptors
	num_commands: the number of commands
	index: the current index of the command in the pipe chain
*/
void executeCommand(int** fds, int num_commands, command cmd, int index) {
	//close stdin, duplicate previous command's pipe input to stdin
	//except when this is first command
	if (index != 0) {
		int prev_cmd_index = index - 1;
	 	if (dup2(fds[prev_cmd_index][0], 0) == -1) {
	 		perror("dup2(fds[prev_cmd_index][0], 0) failed");
	 		exit(1);
	 	}
	}

	//close stdout, duplicate current pipe output to stdout
	//except when this is the last command
	if (index != (num_commands - 1)) {
		if (dup2(fds[index][1], 1) == -1) {
	 		perror("dup2(fds[index][1], 1) failed");
	 		exit(1);
	 	}
	}

	closeAllPipes(fds, num_commands);

	execvp(cmd.argv[0], cmd.argv);
	perror("Command execution failed");
	exit(1);
}

/*
Close all ends to all pipes
Input:
	fds: an array of 2-int array file descriptors
	num_commands: the number of commands (same as the number of pipes)
*/
void closeAllPipes(int** fds, int num_commands) {
	for (int i = 0; i < num_commands; ++i) {
		close(fds[i][0]);
		close(fds[i][1]);
	}
}

//Wait for all child processes to terminate before continuing shell
void waitForAllChildProcesses(int num_commands) {
	for (int i = 0; i < num_commands; ++i) {
		if (wait(NULL) == -1) {
			perror("wait failed");
			exit(1);
		}
	}
}

//Free up the array of commands
void freeCommandArray(command* commands, int num_commands) {
	for (int i = 0; i < num_commands; ++i) {
		free2DCharArray(commands[i].argv, commands[i].alloc_size);
	}
	free(commands);
}

/*
Free up a 2D char array given the alloc_size
Input:
	array: a 2D array to free up
	alloc_size: array's allocated size on the heap
*/
void free2DCharArray(char** array, size_t alloc_size) {
	for(int i = 0; i < alloc_size; i++) {
		free(array[i]);
	}
	free(array);
}