/*
Minhtri Tran
Unix programming
Assignment 1: Game of Life
*/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

//print all generations of life
void printLife(char ***life_array, int num_gens, int num_rows, int num_cols) {
  for (int i=0; i<num_gens; i++) {
    printf("Generation %d:\n", i);
    for (int j=1; j<num_rows-1; j++) {
      for (int k=1; k<num_cols-1; k++) {
	if (life_array[i][j][k] ==  '*')
	  putchar(life_array[i][j][k]);
	else
	  putchar('-');
      }
      putchar('\n');
    }
    printf("================================\n");
  }
}

//return the number of live neighbors of a cell
int numLiveNeighbors(char **life_array, int cur_row, int cur_col) {
  int live_neighbors = 0;

  //orthogonally adjacent neighbors
  if (life_array[cur_row-1][cur_col] == '*')
    live_neighbors++;
  if (life_array[cur_row+1][cur_col] == '*')
    live_neighbors++;
  if (life_array[cur_row][cur_col-1] == '*')
    live_neighbors++;
  if (life_array[cur_row][cur_col+1] == '*')
    live_neighbors++;

  //diagonally adjacent neigbors
  if (life_array[cur_row-1][cur_col-1] == '*')
    live_neighbors++;
  if (life_array[cur_row-1][cur_col+1] == '*')
    live_neighbors++;
  if (life_array[cur_row+1][cur_col-1] == '*')
    live_neighbors++;
  if (life_array[cur_row+1][cur_col+1] == '*')
    live_neighbors++;

  return live_neighbors;
}

//process a generation by using the previous generation's data
void processGeneration(char **prev_life_array, char **life_array, int num_rows, int num_cols) {
  for (int i=1; i<num_rows-1; i++) {
    for (int j=1; j<num_cols-1; j++) {
      if (prev_life_array[i][j] == '*') {
	//Any live cell with fewer than two live neighbours dies
	if (numLiveNeighbors(prev_life_array, i, j) < 2) {
	  life_array[i][j] = ' ';
	}
	//Any live cell with two or three live neighbours lives on to the next generation
	else if (numLiveNeighbors(prev_life_array, i, j) == 2 || numLiveNeighbors(prev_life_array, i, j) == 3) {
	  life_array[i][j] = '*';
	}
	//Any live cell with more than three live neighbours dies
	else if (numLiveNeighbors(prev_life_array, i, j) > 3) {
	  life_array[i][j] = ' ';
	}
      }
      else {
	//Any dead cell with exactly three live neighbours becomes a live cell
	if (numLiveNeighbors(prev_life_array, i, j) == 3) {
	  life_array[i][j] = '*';
	}
      }
      
    }
  }
}

int main(int argc, char *argv[]) {

  //default values
  int num_rows = 10;
  int num_cols = 10;
  int num_generations = 10;
  char *file_name = "life.txt";

  //user-set number of rows
  if (argc >= 2) {
    char *endptr;
    int val = strtol(argv[1], &endptr, 10);
    //If there are fewer lines in the file than expected, the missing "cells" will be assumed to be dead.
    if (val > 10)
      num_rows = val;
  }
  //user-set number of columns
  if (argc >= 3) {
    char *endptr;
    int val = strtol(argv[2], &endptr, 10);
    //If the lines are shorter than expected,the missing "cells" will be assumed to be dead.
    if (val > 10)
      num_cols = val;
  }
  //user-set filename
  if (argc >= 4) {
    file_name = argv[3];
  }
  //user-set number of generations
  if (argc >= 5) {
    char *endptr;
    num_generations = strtol(argv[4], &endptr, 10);
  }
  
  //Open file
  FILE *file = fopen(file_name, "r");
  if (file == NULL) {
    fprintf(stderr, "Can't open file\n");
  }

  //pad array with extra top and bottom rows
  //and extra left and right columns
  num_rows += 2;
  num_cols += 2;

  //add an extra generation for the gen 0
  num_generations += 1;
  
  //declare and allocate 3D array
  //order: life_array[gen][row][col]
  char ***life_array;

  life_array = (char***)malloc(sizeof(char**) * num_generations);
  for (int i=0; i<num_generations; i++) {
    life_array[i] = (char**)malloc(sizeof(char*) * num_rows);
    for (int j=0; j<num_rows; j++) {
      life_array[i][j] = (char*)malloc(sizeof(char) * num_cols);
    }
  }

  //fill our life_array for all generations
  for (int cur_gen=0; cur_gen<num_generations; cur_gen++) {
    //Read file data into the zeroth generation
    if (cur_gen == 0) {
      char c;
      int cur_row = 1;
      int cur_col = 1;
      //Loop through file char by char
      while ((c = fgetc(file))) {
	if (feof(file))
	  break;

	//char is a cell
	if (c == '*') {
	  life_array[cur_gen][cur_row][cur_col] = '*';
	}
    
	//reached end of row
	if (c == '\n') {
	  cur_col = 1;
	  cur_row++;
	}
	else {
	  cur_col++;
	}
      }
    }
    //process information for later generations
    else {
      processGeneration(life_array[cur_gen-1], life_array[cur_gen], num_rows, num_cols);
    }
  }

  printLife(life_array, num_generations, num_rows, num_cols);
  
  //free heap space
  for (int i=0; i<num_generations; i++) {
    for (int j=0; j<num_rows; j++) {
      free(life_array[i][j]);
    }
  }
  for (int i=0; i<num_generations; i++) {
    free(life_array[i]);
  }
  free(life_array);
  

  fclose(file);
  
  return 0;
}
