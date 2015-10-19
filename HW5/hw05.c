//Minhtri Tran
//Unix Programming
//Assignment 5 - du

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

const int INPUT_BUFFER_SIZE = 800;
const int PATH_BUFFER_SIZE = 1600;

char* getInput(int argc, char** argv);
unsigned long getSizeDir(char* dirname);

int main(int argc, char *argv[]) {
	char* dirname = getInput(argc, argv);

	getSizeDir(dirname);

	return 0;
}

//Get directory name input from argument
char* getInput(int argc, char** argv) {
	char* input_directory = malloc(sizeof(char) * INPUT_BUFFER_SIZE);
	if (argc == 1) {
		input_directory = ".";
	}
	else if (argc == 2) {
		strcpy(input_directory, argv[1]);
	}
	else {
		puts("Usage: du <dirname>");
		exit(1);
	}
	return input_directory;
}

unsigned long getSizeDir(char* dirname) {
	unsigned long total_blocks = 0;
	DIR* dirp = opendir(dirname);
	struct dirent* direntp;
	while ((direntp = readdir(dirp)) != NULL) {
		char filename[PATH_BUFFER_SIZE];
		snprintf(filename, PATH_BUFFER_SIZE, "%s/%s", dirname, direntp->d_name);
		if (strcmp(direntp->d_name, "..") != 0) {
			struct stat statBuf;
			lstat(filename, &statBuf);
			if (strcmp(direntp->d_name, ".") == 0 || S_ISREG(statBuf.st_mode)) {
				total_blocks += statBuf.st_blocks;
			}
			else if (S_ISDIR(statBuf.st_mode)) {
				total_blocks += getSizeDir(filename);
			}
		}
	}
	printf("%ld\t%s\n", total_blocks/2, dirname);
	return total_blocks;
}