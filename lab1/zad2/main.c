#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/times.h>

// Defines
#ifndef REPORT_FILENAME
#define REPORT_FILENAME "raport.txt"
#endif

#ifndef DLL
  #include "my_library.h"
#endif

#ifdef DLL
  void* dll;
  typedef struct mylib_block_array {
    uint32_t size;
    char **blocks;
  } mylib_block_array;
#endif

typedef struct timespec timespec;
typedef struct tms tms;

// Global variables
FILE *report_file;
tms *tms_execution_start; // time of benchmark start
tms *tms_operation_start; // start of current step
tms *tms_operation_end;   // helper buffer
timespec *ts_execution_start;
timespec *ts_operation_start;
timespec *ts_operation_end;

void printUsageAndExit(){
  printf("Usage: array_size [operations]\n");
  printf("Operations:\n");
  printf("remove_block idx - removes block at position idx\n");
  printf("move_to_memory - copies data from temporary file to block array\n");
  printf("search_directory dir lookup_string temp_file_name - performs find in dir using lookup_string as search query, results are stored in temp_file_name\n");
  if(report_file) fclose(report_file);
  exit(1);
}

void measureTimes(tms *tms_struct, timespec *timespec_struct){
  if(times(tms_struct) == -1){
    printf("Unable to access sys/time.h clock.\n");
    exit(3);
  }

  if(clock_gettime(CLOCK_REALTIME, timespec_struct) == -1){
    printf("Unable to acces time.h clock.\n");
    exit(4);
  }
}

void printTimes(const char* operation_name, tms *tms_start_time, timespec *ts_start_time){
  measureTimes(tms_operation_end, ts_operation_end);

  long realTime = ts_operation_end->tv_sec - ts_start_time->tv_sec; //real time in secound

  clock_t userTime_clocks = tms_operation_end->tms_cutime - tms_start_time->tms_cutime;
  userTime_clocks += tms_operation_end->tms_utime - tms_start_time->tms_utime;
  double userTime = userTime_clocks/(CLOCKS_PER_SEC/1000.0);

  clock_t systemTime_clocks = tms_operation_end->tms_cstime - tms_start_time->tms_cstime;
  systemTime_clocks += tms_operation_end->tms_stime - tms_start_time->tms_stime;
  double systemTime = systemTime_clocks/(CLOCKS_PER_SEC/1000.0);

  printf("%20s %20lus %20.3fms %20.3fms\n",
         operation_name,
         realTime,
         userTime,
         systemTime);
  fprintf(report_file,
         "%20s %20lus %20.3fms %20.3fms\n",
         operation_name,
         realTime,
         userTime,
         systemTime);
}

void search_directory(const char* dir, const char* lookup, const char* temp){
  int8_t status;

  status = mylib_SetFindParams(dir, lookup, temp);
  if(status != 0){
    printf("Error executing 'search_directory' command with args: (%s, %s, %s). Error code: %i\n",
           dir, lookup, temp, status);
    printUsageAndExit();
  }

  status = mylib_PerformFind();
  if(status != 0){
    printf("Error executing 'search_directory' command with args: (%s, %s, %s). Error code: %i\n",
           dir, lookup, temp, status);
    printUsageAndExit();
  }
}

void move_to_memory(mylib_block_array* arr){
  int64_t idx = mylib_MoveTemporaryFileToArray(arr);
  if(idx >= 0) return;

  printf("Error executing 'move_to_memory' command. Error code: %i\n", (int) idx);
  printUsageAndExit();
}

void remove_block(mylib_block_array* arr, int idx){
  int8_t status = mylib_RemoveEntry(arr, idx);
  if(status == 0) return;

  printf("Error executing 'remove_block' command with args: (%i). Error code: %i\n", idx, status);
  printUsageAndExit();
}

/*
  Three types of operations:
    - performing search(search_directory)
    - copying search result(move_to_mem)
    - removing from memory(remove_block)
 */
void parseArgs(mylib_block_array *arr, uint32_t count, char** operations){
  for(uint32_t i = 0; i < count; i++){

    char* operation = operations[i];

    //Save time before operation
    measureTimes(tms_operation_start, ts_operation_start);

    if(strcmp("search_directory", operation) == 0){

      if(i + 3 >= count){
        printf("Search directory takes three args!\n");
        printUsageAndExit();
      }

      search_directory(operations[i+1], operations[i+2], operations[i+3]);
      i += 3; // search_directory takes three args

    } else if(strcmp("move_to_mem", operation) == 0){

      move_to_memory(arr);

    } else if(strcmp("remove_block", operation) == 0){

      if(i + 1 >= count){
        printf("Remove block takes one arg!\n");
        printUsageAndExit();
      }

      int block_idx = strtol(operations[i+1], NULL, 10);
      remove_block(arr, block_idx);
      i += 1; // remove_block takes one arg

    }else{

      printf("Unrecognizable operation: '%s'\n", operation);
      printUsageAndExit();

    }

    // Print times of single operation
    printTimes(operation, tms_operation_start, ts_operation_start);
  }
}

int main(int argc, char **argv){
  if(argc < 3) {
    printUsageAndExit();
  }

  // Open report file
  report_file = fopen(REPORT_FILENAME, "w");
  if(!report_file){
    printf("Unable to open report file.\n");
    exit(2);
  }

  // Allocating space for sys/time.h buffers
  tms_execution_start = calloc(1, sizeof(tms));
  tms_operation_start = calloc(1, sizeof(tms));
  tms_operation_end = calloc(1, sizeof(tms));
  // Allocating space for time.h buffers
  ts_execution_start = calloc(1, sizeof(timespec));
  ts_operation_start = calloc(1, sizeof(timespec));
  ts_operation_end = calloc(1, sizeof(timespec));

  // First arg is always array_size
  int array_size = strtol(argv[1], NULL, 10);
  if(array_size <= 0){
    printUsageAndExit();
  }

  mylib_block_array *arr = mylib_InitArray(array_size);

  // Load up execution_start tirmes
  measureTimes(tms_execution_start, ts_execution_start);

  printf("%20s  %20s %20s %20s\n", "Operation", "Real", "User", "System");
  fprintf(report_file, "%20s  %20s %20s %20s\n", "Operation", "Real", "User", "System");
  printTimes("START", tms_execution_start, ts_execution_start);

  // Parse and execute arguments operations
  parseArgs(arr, argc - 2, argv+2);

  // Print elapsed tiems since execution
  printTimes("OVERALL", tms_execution_start, ts_execution_start);

  // Release taken memory
  mylib_DisposeArray(arr);
  fclose(report_file);
  free(tms_execution_start);
  free(tms_operation_start);
  free(tms_operation_end);
  free(ts_execution_start);
  free(ts_operation_start);
  free(ts_operation_end);

  return 0;
}
