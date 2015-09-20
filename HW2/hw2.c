//Minhtri Tran
//Unix Programming
//Homework 2

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

extern char **environ;

char *extractName(char *from);
char *extractValue(char *from);
int findNameInEnviron(char **environ, char *name);

int main(int argc, char *argv[]) {
  //If no arguments were issued, display environ
  if (argc == 1) {
    for (int i = 0; environ[i] != NULL; i++) {
      puts(environ[i]);
    }
    exit(0);
  }

  int dash_i = 0;
  int num_existing_name_pairs = 0;
  int num_new_name_pairs = 0;
  int num_inputted_pairs = 0;

  int command_index = -1;
  
  for (int i = 1; argv[i] != NULL; i++) {

    //Determine if user inputted -i before entering name=value pairs
    if (num_inputted_pairs == 0 && strcmp(argv[i],"-i") == 0) {
      dash_i = 1;
    }

    //Find the number of existing and new inputted name=value pairs
    //We need this number to define existing and new name arrays
    if (strchr(argv[i], '=') != NULL) {
      char *input_name = extractName(argv[i]);
      if (findNameInEnviron(environ, input_name) != -1 && dash_i == 0) {
	num_existing_name_pairs++;
      }
      else {
	num_new_name_pairs++;
      }

      num_inputted_pairs++;
    }

    //Process arg as a command if arg is after the name=value pairs
    if (num_inputted_pairs > 0 && strchr(argv[i], '=') == NULL && command_index == -1) {
      command_index = i;
    }
  }

  //Handle command
  char **command_argv = NULL;
  if (command_index != -1) {
    
    command_argv = (char**)malloc(sizeof(char*) * (argc - command_index));
    
    for (int i = command_index; argv[i] != NULL; i++) {
      command_argv[i - command_index] = argv[i];
    }
    
  }
  
  assert(num_existing_name_pairs + num_new_name_pairs == num_inputted_pairs);
  
  //this array stores all the existing inputted name=value pairs
  char **existing_name_values = (char**)malloc(sizeof(char*) * num_existing_name_pairs);

  //this array stores all new inputted name=value pairs
  char **new_name_values = (char**)malloc(sizeof(char*) * num_new_name_pairs);

  int existing_name_index = 0;
  int new_name_index = 0;
  //Loop through inputted args and add the name, value pairs to both the existing and new name arrays
  for (int i = 1; argv[i] != NULL; i++) {
    
    if (strchr(argv[i], '=') != NULL) {
      
      char *value =extractValue(argv[i]);
      assert(strlen(value) > 0);

      char *name = extractName(argv[i]);
      assert(strlen(name) > 0);

      char *input_name = extractName(argv[i]);
      if (findNameInEnviron(environ, input_name) != -1 && dash_i == 0) {
        existing_name_values[existing_name_index] = argv[i];
	existing_name_index++;
      }
      else {
        new_name_values[new_name_index] = argv[i];
	new_name_index++;
      }
    }
  }

  //Loop through existing name array and modify the corresponding values in the environ array
  for(int i = 0; existing_name_values[i] != NULL; i++) {

    char *input_name = extractName(existing_name_values[i]);
    int index = findNameInEnviron(environ, input_name);
    if (index != -1) {
      environ[index] = existing_name_values[i];
    }
  }

  //Initialize a new environ array in the heap
  if (num_new_name_pairs > 0) {
    int num_pairs_in_environ = 0;
    for (int i = 0; environ[i] != NULL; i++) {
      num_pairs_in_environ++;
    }

    int num_pairs_in_new_environ = 0;
    if (dash_i == 0) {
      num_pairs_in_new_environ = num_pairs_in_environ + num_new_name_pairs;
    }
    else {
      num_pairs_in_new_environ = num_new_name_pairs;
    }
    
    //Initialize a new environ array
    char **new_environ = (char**)malloc(sizeof(char*) * (num_pairs_in_new_environ));

    //don't ignore inherited environment
    if (dash_i == 0) {
      for (int i = 0; environ[i] != NULL; i++) {
	new_environ[i] = environ[i];
      }
      
      for (int i = 0; new_name_values[i] != NULL; i++) {
	new_environ[num_pairs_in_environ + i] = new_name_values[i];
      }
    }
    //ignore inherited environment
    else {
      for (int i = 0; new_name_values[i] != NULL; i++) {
	new_environ[i] = new_name_values[i];
      }
    }

    //Make the new environ array NULL-terminated
    new_environ[num_pairs_in_new_environ] = NULL;

    //If no commands was given, print environment
    if (command_index == -1) {
      //Print out new environment
      for (int i = 0; new_environ[i] != NULL; i++) {
	puts(new_environ[i]);
      }
    }
    //If a command was given, execute it
    else {
      //I tried working without execvpe, but it kept giving me weird errors
      //so I'm just going to use execvpe
      execvpe(command_argv[0], command_argv, new_environ);
      perror("Command execution failed");
    }
  }
  //Use original environ array
  else {
    //If no command was given, print environment
    if (command_index == -1) {
      //Print out environment
      for (int i = 0; environ[i] != NULL; i++) {
	puts(environ[i]);
      }
    }
    //If a command was given, execute it
    else {
      execvp(command_argv[0], command_argv);
      perror("Command execution failed");
    }
  }
  
}

//Given a name=value string, return the name portion
char *extractName(char *from) {
  char *equals_index = strchr(from, '=');
  
  assert(equals_index != NULL);
  
  int size = strlen(from) - strlen(equals_index);
  char *name = malloc(sizeof(char) * size);
  strncpy(name, from, size);
  name[size] = '\0';
  
  return name;
}

//Given a name=value string, return the value portion
char *extractValue(char *from) {
  char *equals_index = strchr(from, '=');

  assert(equals_index != NULL);
  
  return equals_index + 1;
}

//Returns the index of the name=value pair given the environ array and a name
//Returns -1 if the name does not exist in the environ array
int findNameInEnviron(char **environment, char *name) {
  for (int i = 0; environment[i] != NULL; i++) {
    char *environ_name = extractName(environment[i]);
    if(strcmp(name, environ_name) == 0) {
      return i;
    }
  }
  return -1;
}
