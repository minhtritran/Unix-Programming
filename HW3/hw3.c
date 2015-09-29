//Minhtri Tran
//Unix Programming
//Assignment 3 - Simple shell

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

char** getCommandFromInputString(char* inputString, int* command_argc, size_t* alloc_size);

int main() {
	while(1) {
		//Set prompt string
		char* prompt;
		prompt = getenv("PS1");
		if (prompt == NULL) {
			prompt = "#> ";
		}
		printf("%s", prompt);

		//Store shell input
		char input_string[800];
		if (fgets(input_string, 800, stdin) == NULL) {
			perror("Bad input string");
		}

		//Remove trailing newline character from fgets
		char *pos;
		if ((pos=strchr(input_string, '\n')) != NULL) {
		    *pos = '\0';
		}

		//Exit program if input is "exit"
		if (strcmp(input_string, "exit") == 0)  {
			exit(0);
		}

		//Initialize variables
		size_t alloc_size = 2;
		int command_argc = 0;
		
		//Fill command_argv
		char** command_argv = getCommandFromInputString(input_string, &command_argc, &alloc_size);

		//Fork to handle command
		pid_t fork_pid = fork();
		if (fork_pid == -1) {
			perror("Fork failed");
		}

		//Handle command with child process
		if (fork_pid == 0) {
			execvp(command_argv[0], command_argv);
			perror("Command execution failed");
		}
		else {
			wait(NULL);

			//free up array
			for(int i = 0; i < alloc_size; i++) {
				free(command_argv[i]);
			}
			free(command_argv);
		}
	}
	
	return 0;
}

/*
Tokenize input string and turn it into an array
Input:
	inputString: a space-delimited input string
	command_argc: the number of current elements in command_argv
	alloc_size: the starting allocation size of command_argv
Output:
	Returns an array filled with the command and arguments
Special notes:
	Command_argc and alloc_size gets modified
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

		//realloc array by factor of 2 if not big enough
		if ((*command_argc) > (*alloc_size)) {
			(*alloc_size) = (*alloc_size) * 2;
			command_argv = realloc(command_argv, sizeof(char*) * (*alloc_size));
			if (command_argv == NULL) {
				perror("command_argv realloc failed");
			}
		}

		pch = strtok(NULL, " ");
	}

	return command_argv;
}