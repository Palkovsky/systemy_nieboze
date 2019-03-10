#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

// Defines
#ifndef REPORT_FILENAME
#define REPORT_FILENAME "raport.txt"
#endif

#ifndef DLL
#include "my_library.h"
#endif

#ifdef DLL
#include <dlfcn.h>
void* dll;
typedef struct mylib_block_array {
  uint32_t size;
  char **blocks;
} mylib_block_array;
#endif

typedef struct timeval timeval;
typedef struct timespec timespec;
typedef struct rusage rusage;

// Global variables
FILE *report_file;
timeval *utime_execution_start;
timeval *utime_operation_start;
timeval *utime_operation_end;

timeval *stime_execution_start;
timeval *stime_operation_start;
timeval *stime_operation_end;

timeval *rtime_execution_start;
timeval *rtime_operation_start;
timeval *rtime_operation_end;

void printUsageAndExit(){
  printf("Usage: array_size [operations]\n");
  printf("Operations:\n");
  printf("remove_block idx - removes block at position idx\n");
  printf("move_to_memory - copies data from temporary file to block array\n");
  printf("search_directory dir lookup_string temp_file_name - performs find in dir using lookup_string as search query, results are stored in temp_file_name\n");
  if(report_file) fclose(report_file);
  exit(1);
}

void measureTimes(timeval *utime, timeval *stime, timeval *rtime){
  rusage res_usage;
  timespec timespec;

  if(getrusage(RUSAGE_SELF, &res_usage) == -1){  // calculate usage of current process
    printf("Unable to access sys/resource.h clock.\n");
    exit(3);
  }

  utime->tv_usec = res_usage.ru_utime.tv_usec;
  stime->tv_usec = res_usage.ru_stime.tv_usec;
  utime->tv_sec  = res_usage.ru_utime.tv_sec;
  stime->tv_sec  = res_usage.ru_stime.tv_sec;

  if(getrusage(RUSAGE_CHILDREN, &res_usage) == -1){ // caluclate usage of children process(find)
    printf("Error accessing user and system time by sys/resurces.h.\n");
    exit(3);
  }

  utime->tv_usec += res_usage.ru_utime.tv_usec;
  if (utime->tv_usec >= 1000000){
      utime->tv_sec++;
      utime->tv_usec -= 1000000;
  }

  stime->tv_usec += res_usage.ru_stime.tv_usec;
  if (stime->tv_usec >= 1000000){
    stime->tv_sec++;
    stime->tv_usec -= 1000000;
  }


  utime->tv_sec  += res_usage.ru_utime.tv_sec;
  stime->tv_sec  += res_usage.ru_stime.tv_sec;

  if(clock_gettime(CLOCK_REALTIME, &timespec) == -1){
    printf("Error accessing user and system time by time.h.\n");
    exit(4);
  }

  rtime->tv_usec = timespec.tv_nsec/1000;
  rtime->tv_sec = timespec.tv_sec;
}

void printTimes(const char* operation_name, timeval *u_start, timeval *s_start, timeval *r_start){
  measureTimes(utime_operation_end, stime_operation_end, rtime_operation_end);

  timeval *u_end = utime_operation_end;
  timeval *s_end = stime_operation_end;
  timeval *r_end = rtime_operation_end;

  suseconds_t realTime_micro = r_end->tv_usec - r_start->tv_usec;
  double realTime = (double) realTime_micro/1000000.0 + r_end->tv_sec - r_start->tv_sec;

  suseconds_t userTime_micro = u_end->tv_usec - u_start->tv_usec;
  double userTime = (double) userTime_micro/1000000.0  + u_end->tv_sec - u_start->tv_sec;

  suseconds_t systemTime_micro = s_end->tv_usec - s_start->tv_usec;
  double systemTime = (double) systemTime_micro/1000000.0 + s_end->tv_sec - s_start->tv_sec;;

  printf("%50s %20fs %20fs %20fs\n",
         operation_name,
         realTime,
         userTime,
         systemTime);
  fprintf(report_file,
         "%50s %20fs %20fs %20fs\n",
         operation_name,
         realTime,
         userTime,
         systemTime);
}

/*
  Implementations of user commands
 */
void search_directory(const char* dir, const char* lookup, const char* temp){
  #ifdef DLL
    int8_t (*mylib_SetFindParams)(const char*, const char*, const char*) = dlsym(dll, "mylib_SetFindParams");
    int8_t (*mylib_PerformFind)() = dlsym(dll, "mylib_PerformFind");
  #endif

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
  #ifdef DLL
    int64_t (*mylib_MoveTemporaryFileToArray)(mylib_block_array*) = dlsym(dll, "mylib_MoveTemporaryFileToArray");
  #endif

  int64_t idx = mylib_MoveTemporaryFileToArray(arr);
  if(idx >= 0) return;

  printf("Error executing 'move_to_memory' command. Error code: %i\n", (int) idx);
  printUsageAndExit();
}

void remove_block(mylib_block_array* arr, int idx){
  #ifdef DLL
    int8_t (*mylib_RemoveEntry)(mylib_block_array*, uint32_t) = dlsym(dll, "mylib_RemoveEntry");
  #endif

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
    char* operation_display = calloc(50, sizeof(char));

    //Save time before operation
    measureTimes(utime_operation_start, stime_operation_start, rtime_operation_start);

    if(strcmp("search_directory", operation) == 0){

      if(i + 3 >= count){
        printf("Search directory takes three args!\n");
        printUsageAndExit();
      }

      search_directory(operations[i+1], operations[i+2], operations[i+3]);
      sprintf(operation_display, "search_directory %s %s %s", operations[i+1], operations[i+2], operations[i+3]);

      i += 3; // search_directory takes three args

    } else if(strcmp("move_to_mem", operation) == 0){

      move_to_memory(arr);
      sprintf(operation_display, "move_to_mem");

    } else if(strcmp("remove_block", operation) == 0){

      if(i + 1 >= count){
        printf("Remove block takes one arg!\n");
        printUsageAndExit();
      }

      int block_idx = strtol(operations[i+1], NULL, 10);
      remove_block(arr, block_idx);
      sprintf(operation_display, "remove_block %i", block_idx);

      i += 1; // remove_block takes one arg

    }else{

      printf("Unrecognizable operation: '%s'\n", operation);
      printUsageAndExit();

    }

    // Print times of single operation
    printTimes(operation_display, utime_operation_start, stime_operation_start, rtime_operation_start);
    free(operation_display);
  }
}

int main(int argc, char **argv){
  if(argc < 3) {
    printUsageAndExit();
  }

  #ifdef DLL
    dll = dlopen(DLL, RTLD_LAZY);
    if(!dll){
      printf("Unable to access dynamic library.\n");
      exit(5);
    }
    mylib_block_array *(*mylib_InitArray)(uint32_t) = dlsym(dll, "mylib_InitArray");
    int8_t (*mylib_DisposeArray)(mylib_block_array*) = dlsym(dll, "mylib_DisposeArray");
  #endif

  // Open report file
  report_file = fopen(REPORT_FILENAME, "w");
  if(!report_file){
    printf("Unable to open report file.\n");
    exit(2);
  }

  // Allocating space for sys/resurce.h buffers
  timeval* timevals = calloc(9, sizeof(timeval));
  utime_execution_start = timevals;
  utime_operation_start = timevals + 1;
  utime_operation_end  = timevals+2;
  stime_execution_start = timevals+3;
  stime_operation_start = timevals+4;
  stime_operation_end = timevals+5;
  rtime_execution_start = timevals+6;
  rtime_operation_start = timevals+7;
  rtime_operation_end = timevals+8;

  // First arg is always array_size
  int array_size = strtol(argv[1], NULL, 10);
  if(array_size <= 0){
    printUsageAndExit();
  }

  // Print headers
  printf("%50s  %20s %20s %20s\n", "Operation", "Real", "User", "System");
  fprintf(report_file, "%50s  %20s %20s %20s\n", "Operation", "Real", "User", "System");

  // Load up execution_start times
  measureTimes(utime_execution_start, stime_execution_start, rtime_execution_start);

  mylib_block_array *arr = mylib_InitArray(array_size);

  char* buff = calloc(50, sizeof(char));
  sprintf(buff, "create_table %i", array_size);
  printTimes(buff, utime_execution_start, stime_execution_start, rtime_execution_start);
  free(buff);

  // Parse and execute arguments operations
  parseArgs(arr, argc - 2, argv+2);

  // Print elapsed tiems since execution
  printTimes("TOTAL", utime_execution_start, stime_execution_start, rtime_execution_start);

  // Release taken memory
  mylib_DisposeArray(arr);
  fclose(report_file);
  free(timevals);

  #ifdef DLL
    dlclose(dll);
  #endif

  return 0;
}
