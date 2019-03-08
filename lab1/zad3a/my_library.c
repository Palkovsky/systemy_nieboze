#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>

#include "my_library.h"

#define DEFAULT_TEMP_FILE_PATH "results.temp"

static const char* lookup_dir = NULL;
static const char* lookup_filter = NULL;
static const char* temp_filename = NULL;

mylib_block_array* mylib_InitArray(uint32_t size){
  mylib_block_array* arr = calloc(1, sizeof(mylib_block_array));
  arr->blocks = calloc(size, sizeof(char*));
  arr->size = size;
  return arr;
}

int8_t mylib_SetFindParams(const char* dir, const char* lookup_string, const char* temp_target){
  lookup_dir = dir;
  lookup_filter = lookup_string;
  temp_filename = temp_target;
  if(temp_filename == NULL) temp_filename = DEFAULT_TEMP_FILE_PATH;
  return 0;
}

int8_t mylib_PerformFind(){
  if(lookup_dir == NULL || lookup_filter == NULL) return -1;

  // Make sure lookup_dir is valid directory
  DIR* dir = opendir(lookup_dir);
  if(dir) closedir(dir);
  else return -2;

  uint32_t command_length;
  char *command_buffer;

  command_length = strlen(lookup_dir) + strlen(lookup_filter) + 50;
  command_buffer = calloc(command_length, sizeof(char));
  sprintf(command_buffer, "find %s -name %s > %s", lookup_dir, lookup_filter, temp_filename);

  if(system(command_buffer) < 0) return -3;

  return 0;
}

int64_t mylib_MoveTemporaryFileToArray(mylib_block_array* arr){
  if(arr == NULL || arr->blocks == NULL) return -1;

  // first we look for free spot in block array
  uint32_t blockIdx = 0;
  while(arr->blocks[blockIdx] != NULL && blockIdx < arr->size) blockIdx++;
  if(blockIdx == arr->size) return -3; // no free blocks

  FILE *temp_file;
  uint32_t f_size;
  char *block_buffer;

  temp_file = fopen(temp_filename, "r");
  if(!temp_file) return -2;

  // calculating file size in bytes
  fseek(temp_file, 0, SEEK_END); // moves file cursor at the end
  f_size = ftell(temp_file);     // saves cursor position
  fseek(temp_file, 0, SEEK_SET); // moves file cursor to begining

  // allocating space for temporary file contents
  block_buffer = calloc(f_size + 1, sizeof(char));

  // copies file contents to block_buffer
  char ch;
  uint32_t idx = 0;
  while((ch = fgetc(temp_file)) != EOF)
    block_buffer[idx++] = ch;
  block_buffer[idx] = '\0'; // adding null-terminator

  // closing temporary file
  fclose(temp_file);

  // linking block_buffer with block array
  arr->blocks[blockIdx] = block_buffer;

  return blockIdx;
}

int8_t mylib_RemoveEntry(mylib_block_array *arr, uint32_t idx){
  if(arr == NULL || arr->blocks == NULL) return -1;
  if(idx >= arr->size) return -2;
  if(arr->blocks[idx] == NULL) return -3;
  free(arr->blocks[idx]);
  arr->blocks[idx] = NULL;
  return 0;
}

int8_t mylib_DisposeArray(mylib_block_array* arr){
  if(arr == NULL) return -1;
  for(int i=0; i<arr->size; i++) free(arr->blocks[i]);
  free(arr->blocks);
  free(arr);
  return 0;
}

const char* mylib_GetItem(mylib_block_array* arr, uint32_t idx){
  if(arr == NULL || arr->blocks == NULL || idx >= arr->size) return NULL;
  return arr->blocks[idx];
}
