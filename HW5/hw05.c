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
#include <errno.h>

const int INPUT_BUFFER_SIZE = 1024;
const int PATH_BUFFER_SIZE = 2048;
const size_t INITIAL_ALLOC_SIZE = 2;
const size_t REALLOC_FACTOR = 2;
const int BLOCKSIZE_CONVERSION_RATIO = 2;

struct ULongArray {
	size_t alloc_size;
	int num_inodes;
	unsigned long* inodes;
};

char* getInput(int argc, char** argv);
unsigned long getSizeDir(char* dirname, struct ULongArray* hlink);
int alreadyCountedHLink(struct ULongArray* hlink, unsigned long inode);
void addToHLink(struct ULongArray* hlink, unsigned long inode);

int main(int argc, char *argv[]) {
	char* dirname = getInput(argc, argv);

	//Initialize hard-link array
	struct ULongArray hlink;
	hlink.alloc_size = INITIAL_ALLOC_SIZE;
	hlink.num_inodes = 0;
	if((hlink.inodes = malloc(sizeof(unsigned long) * hlink.alloc_size)) == NULL) {
		perror("malloc for hlink array failed");
		exit(1);
	}

	getSizeDir(dirname, &hlink);
	
	free(hlink.inodes);
	return 0;
}

//Get directory name input from argument
char* getInput(int argc, char** argv) {
	char* input_directory = malloc(sizeof(char) * INPUT_BUFFER_SIZE);
	if (input_directory == NULL) {
		perror("malloc for input_directory failed");
		exit(1);
	}
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

//Recursively calulcate and print the number of blocks used for a directory
//Input: 
//dirname: the name of the current directory
//hlink: array that holds regular inodes with hard-links
unsigned long getSizeDir(char* dirname, struct ULongArray* hlink) {
	unsigned long total_blocks = 0;

	DIR* dirp;
	if ((dirp = opendir(dirname)) == NULL) {
		perror("Cannot open dir");
		//If access denied, just print the directory's block size
		if (errno == EACCES) {
			struct stat statBuf;
			lstat(dirname, &statBuf);
			printf("%ld\t%s\n", statBuf.st_blocks/BLOCKSIZE_CONVERSION_RATIO, dirname);
			return statBuf.st_blocks;
		}
		else {
			exit(1);
		}
		
	}
	struct dirent* direntp;
	//Loop through current directory
	while ((direntp = readdir(dirp)) != NULL) {
		char filename[PATH_BUFFER_SIZE];
		if (strcmp(dirname, "/") == 0) {
			snprintf(filename, PATH_BUFFER_SIZE, "/%s", direntp->d_name);
		}
		else {
			snprintf(filename, PATH_BUFFER_SIZE, "%s/%s", dirname, direntp->d_name);	
		}
		if (strcmp(direntp->d_name, "..") != 0) {
			struct stat statBuf;
			lstat(filename, &statBuf);
			//Handle the . directory entry
			if (strcmp(direntp->d_name, ".") == 0) {
				total_blocks += statBuf.st_blocks;
			}
			//Handle regular files
			else if (S_ISREG(statBuf.st_mode)) {
				if (statBuf.st_nlink > 1) {
					if (!alreadyCountedHLink(hlink, statBuf.st_ino)) {
						total_blocks += statBuf.st_blocks;
						addToHLink(hlink, statBuf.st_ino);
					}
				}
				else {
					total_blocks += statBuf.st_blocks;
				}
			}
			//Handle directories
			else if (S_ISDIR(statBuf.st_mode)) {
				total_blocks += getSizeDir(filename, hlink);
			}
		}
	}

	if (closedir(dirp) == -1) {
		perror("closedir failed");
		exit(1);
	}

	//Convert number of 512 blocks to number of 1024 blocks and print
	printf("%ld\t%s\n", total_blocks/BLOCKSIZE_CONVERSION_RATIO, dirname);

	return total_blocks;
}

//Determine if the given inode is already inside the hlink array
//Returns 1 if it is in the array, 0 if not
int alreadyCountedHLink(struct ULongArray* hlink, unsigned long inode) {
	for (int i = 0; i < hlink->num_inodes; ++i) {
		if (hlink->inodes[i] == inode) {
			return 1;
		}
	}
	return 0;
}

//Add the given inode to the hlink array
void addToHLink(struct ULongArray* hlink, unsigned long inode) {
	if (hlink->num_inodes >= hlink->alloc_size) {
		hlink->alloc_size *= REALLOC_FACTOR;
		if ((hlink->inodes = realloc(hlink->inodes, sizeof(unsigned long) * hlink->alloc_size)) == NULL) {
			perror("realloc in addToHLink failed");
			exit(1);
		}
	}
	hlink->inodes[hlink->num_inodes] = inode;
	hlink->num_inodes++;
}