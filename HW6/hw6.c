//Minhtri Tran
//Unix Programming
//Assignment 6 - I/O redirection, signals, and globbing

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <signal.h>
#include <glob.h>

const unsigned int INPUT_BUFFER_SIZE = 800;
const size_t ARGV_INITIAL_SIZE = 2;
const unsigned int ARGV_REALLOC_FACTOR = 2;
const size_t COMMANDS_INITIAL_SIZE = 2;
const unsigned int COMMANDS_REALLOC_FACTOR = 2;

typedef struct {
	char* command_string;
	char** argv;
	int argc;
	size_t alloc_size;
	char* input_redirect_filename;
	char* output_redirect_filename;
	bool output_append_flag;
} command;

char* setPromptString();
char* getInputString();
void fillCommandArray(	char* input_string, command** commands,
						int* num_commands, size_t* commands_alloc_size);
void fillCommand(command* cmd);
void addArgToArray(char* arg, command* cmd);
void resize2DCharArray(char*** array, size_t* alloc_size);
void resizeCommandArray(command** array, size_t* alloc_size);
bool handleShellCommand(command cmd);
int** createPipes(int num_commands);
void executeCommand(int** fds, int num_commands, command cmd, int index);
void closeAllPipes(int** fds, int num_commands);
void freeCommandArray(command* commandArray, int num_commands);
void waitForAllChildProcesses(int num_commands);
void free2DCharArray(char** array, size_t alloc_size);
bool fileExists(char* filename);

int main() {
	//Set up to ignore SIGINT and SIGQUIT
	struct sigaction newact, oldact;
	newact.sa_handler = SIG_IGN;
	sigaction(SIGINT, &newact, &oldact);
	sigaction(SIGQUIT, &newact, &oldact);

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
					sigaction(SIGINT, &oldact, NULL);
					sigaction(SIGQUIT, &oldact, NULL);
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
		(*commands)[(*num_commands)].command_string = strcpy((*commands)[(*num_commands)].command_string, pch);
		(*num_commands)++;

		//resize array if not big enough
		if ((*num_commands) > (*commands_alloc_size)) {
			resizeCommandArray(commands, commands_alloc_size);
		}

		pch = strtok(NULL, "|");
	}
	
	for (int i = 0; i < (*num_commands); ++i) {
		fillCommand(&(*commands)[i]);
	}
}

//Tokenize input string and fill given command
void fillCommand(command* cmd) {
	//Initial variables
	(*cmd).alloc_size = ARGV_INITIAL_SIZE;
	(*cmd).argc = 0;
	(*cmd).input_redirect_filename = NULL;
	(*cmd).output_redirect_filename = NULL;
	(*cmd).output_append_flag = false;

	(*cmd).argv = malloc(sizeof(char*) * ((*cmd).alloc_size));
	if ((*cmd).argv == NULL) {
		perror("(*cmd).argv malloc failed");
		exit(1);
	}

	char* pch = strtok((*cmd).command_string, " ");
	bool input_redirect_flag = false;
	bool output_redirect_flag = false;
	//Loop through string by space delimiter
	while (pch != NULL) {
		//Handle i/o redirection
		if (strcmp(pch, "<") == 0) {
			input_redirect_flag = true;
		}
		else if (strcmp(pch, ">") == 0) {
			output_redirect_flag = true;
		}
		else if (strcmp(pch, ">>") == 0) {
			output_redirect_flag = true;
			(*cmd).output_append_flag = true;
		}
		else if (input_redirect_flag == true) {
			(*cmd).input_redirect_filename = malloc(strlen(pch) * sizeof(char));
			if ((*cmd).input_redirect_filename == NULL) {
				perror("(*cmd).input_redirect_filename malloc failed");
				exit(1);
			}
			(*cmd).input_redirect_filename = strcpy((*cmd).input_redirect_filename, pch);
			input_redirect_flag = false;
		}
		else if (output_redirect_flag == true) {
			(*cmd).output_redirect_filename = malloc(strlen(pch) * sizeof(char));
			if ((*cmd).output_redirect_filename == NULL) {
				perror("(*cmd).output_redirect_filename malloc failed");
				exit(1);
			}
			(*cmd).output_redirect_filename = strcpy((*cmd).output_redirect_filename, pch);
			output_redirect_flag = false;
		}
		else {
			//Handle globbing
			glob_t globt;
			glob(pch, 0, NULL, &globt);
			if (globt.gl_pathc > 0) {
				for (int i = 0; i < globt.gl_pathc; i++) {
					addArgToArray(globt.gl_pathv[i], cmd);
				}
			}
			else {
				addArgToArray(pch, cmd);
			}
		}

		pch = strtok(NULL, " ");
	}

	((*cmd).argv)[((*cmd).argc)] = NULL;
	
}

//Add the given argument to the given command
void addArgToArray(char* arg, command* cmd) {
	//add token to array
	((*cmd).argv)[((*cmd).argc)] = malloc(strlen(arg) * sizeof(char));
	if (((*cmd).argv)[((*cmd).argc)] == NULL) {
		perror("command_argv[((*cmd).argc)] malloc failed");
		exit(1);
	}

	((*cmd).argv)[((*cmd).argc)] = strcpy(((*cmd).argv)[((*cmd).argc)], arg);

	((*cmd).argc)++;

	//resize array if not big enough
	if (((*cmd).argc) > ((*cmd).alloc_size)) {
		resize2DCharArray(&((*cmd).argv), &(*cmd).alloc_size);
	}
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
	(*array) = realloc((*array), sizeof((*array)[0]) * (*alloc_size));
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
	//Handle input redirection
	else if (index == 0 && cmd.input_redirect_filename != NULL) {
		int input_fd = open(cmd.input_redirect_filename, O_RDONLY);
		if (input_fd == -1) {
			perror("open input file failed");
			exit(1);
		}
		if (dup2(input_fd, 0) == -1) {
	 		perror("dup2(input_fd, 0) failed");
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
	//Handle output redirection
	else if (index == (num_commands - 1) && cmd.output_redirect_filename != NULL) {
		int output_fd;
		//Open file if it already exists
		if (fileExists(cmd.output_redirect_filename) == true) {
			if (cmd.output_append_flag == true) {
				output_fd = open(cmd.output_redirect_filename, O_WRONLY | O_APPEND);
			}
			else {
				output_fd = open(cmd.output_redirect_filename, O_WRONLY | O_TRUNC);
			}
		}
		//Create file if it doesn't exist
		else {
			output_fd = open(cmd.output_redirect_filename, O_WRONLY | O_CREAT, 0777);
		}
		if (output_fd == -1) {
			perror("open output file failed");
			exit(1);
		}
		if (dup2(output_fd, 1) == -1) {
	 		perror("dup2(output_fd, 1) failed");
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
		free2DCharArray(commands[i].argv, commands[i].argc);
		free(commands[i].input_redirect_filename);
		free(commands[i].output_redirect_filename);
		free(commands[i].command_string);
	}
	free(commands);
}

/*
Free up a 2D char array given the argc
Input:
	array: a 2D array to free up
	argc: array's argc
*/
void free2DCharArray(char** array, size_t argc) {
	for(int i = 0; i < argc; i++) {
		free(array[i]);
	}
	free(array);
}

//Check whether the given file exists or not
bool fileExists(char* filename) {
    struct stat st;
    int result = stat(filename, &st);
    return result == 0;
}