#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/times.h>

// Defines
#define UNRECOGNIZABLE_CMD "Unrecognizable command!"
#define INVALID_ARGS       "Invalid arguments!"

typedef struct tms tms;

// =============== Utility Helper Functions ==================

void printUsageAndExit(const char* err){
  printf("%s USAGE:\n", err);
  printf("generate [target_file] [record_count] [record_size]\n");
  printf("sort [file] [record_count] [record_size] [lib/sys]\n");
  printf("copy [src_file] [target_file] [record_count] [record_size] [lib/sys]\n");
  exit(1);
}

uint64_t parseUnsigned(const char* str){
  int64_t ret = strtol(str, NULL, 10);
  if(ret < 0) {
    printf("Accepting only positive integers.\n");
    exit(1);
  }
  return (uint64_t) ret;
}

uint8_t parseMethod(const char* method){
  if(strcmp(method, "lib") != 0 && strcmp(method, "sys") != 0){
    printf("Invalid method!\n");
    exit(1);
  }
  if(strcmp(method, "lib") == 0) return 1;
  return 0;
}

tms* getTimes(){
  tms *buff = calloc(1, sizeof(tms));
  if(times(buff) < 0){
    printf("Error when accessing clock!\n");
    exit(3);
  }
  return buff;
}

// =============== Core Functionality  ==================
void generate(const char* path, uint64_t count, uint64_t size){
  FILE *file = fopen(path, "w");
  if(!file){
    printf("Unable to open: %s\n", path);
    exit(2);
  }
  FILE *random = fopen("/dev/urandom", "r");
  if(!random){
    printf("Unable to acquire random number generator!\n");
    exit(2);
  }
  void *buffer = calloc(size, 1);

  while(count-- > 0){
    if(fread(buffer, 1, size, random) < 0){
      printf("Unable to read from random number generator!\n");
      exit(2);
    }

    if(fwrite(buffer, 1, size, file) < 0){
      printf("Error while saving to file!\n");
      exit(2);
    }
  }

  free(buffer);
  fclose(file);
  fclose(random);
}

void sort(const char* path, uint64_t count, uint64_t size, uint8_t method){
  void *buff1 = calloc(size, 1);
  void *buff2 = calloc(size, 1);

  if(method == 1){ //lib
    FILE *file = fopen(path, "r+");
    if(!file){
      printf("Unable to open specfied file!\n");
      exit(2);
    }

    for(int64_t i = 1; i < count; i++){
      fseek(file, i*size, 0);
      fread(buff1, 1, size, file);
      unsigned char key1 = *((unsigned char *)buff1);

      int64_t j = i-1;
      while(j >= 0){
        fseek(file, j*size, 0);
        fread(buff2, 1, size, file);
        unsigned char key2 = *((unsigned char *)buff2);

        if(key2 > key1){
          fseek(file, (j+1)*size, 0);
          fwrite(buff2, 1, size, file);
          j--;
        } else break;
      }

      fseek(file, (j+1)*size, 0);
      fwrite(buff1, 1, size, file);
    }

    fclose(file);
  } else { //sys
    int file = open(path, O_RDWR);
    if(file < 0){
      printf("Unable to open specfied file!\n");
      exit(2);
    }

    for(int64_t i = 1; i < count; i++){
      lseek(file, i*size, SEEK_SET);
      read(file, buff1, size);
      unsigned char key1 = *((unsigned char *)buff1);

      int64_t j = i-1;
      while(j >= 0){
        lseek(file, j*size, SEEK_SET);
        read(file, buff2, size);
        unsigned char key2 = *((unsigned char *)buff2);

        if(key2 > key1){
          lseek(file, (j+1)*size, SEEK_SET);
          write(file, buff2, size);
          j--;
        } else break;
      }

      lseek(file, (j+1)*size, SEEK_SET);
      write(file, buff1, size);
    }

    close(file);
  }

  free(buff1);
  free(buff2);
}

void copy(const char* in_path, const char* out_path, uint64_t count, uint64_t buf_size, uint8_t method){
  void *buffer = calloc(buf_size, 1);

  if(method == 1){ // lib
    FILE *file_in  = fopen(in_path, "r");
    FILE *file_out = fopen(out_path, "w");
    if(!file_in || !file_out){
      printf("Unable to acquire descriptors of passed files!\n");
      exit(2);
    }

    while(count-- > 0){
      size_t bytes_read = fread(buffer, 1, buf_size, file_in);
      if(bytes_read < 0) {
        printf("Error reading from input file!\n");
        exit(2);
      }

      if(fwrite(buffer, 1, bytes_read, file_out) < 0){
        printf("Errow writing to target file!\n");
        exit(2);
      }

      if(bytes_read < buf_size) break;
    }

    fclose(file_in);
    fclose(file_out);
  } else { // sys
    int desc_in  = open(in_path, O_RDONLY);
    int desc_out = open(out_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR|S_IWUSR);

    if(desc_in < 0 || desc_out < 0){
      printf("Unable to acquire descriptors of passed files!\n");
      exit(2);
    }

    while(count-- > 0){
      int bytes_read = read(desc_in, buffer, buf_size);

      if(bytes_read < 0){
        printf("Unable to read from input file!\n");
        exit(2);
      }

      if(bytes_read == 0) break;

      if(write(desc_out, buffer, buf_size) < 0){
        printf("Error while writing to output file!\n");
        exit(2);
      }
    }

    close(desc_in);
    close(desc_out);
  }

  free(buffer);
}

// =============== Arg parsing  ==================
void parseArgs(int size, char **args){
  if(size == 0) printUsageAndExit(UNRECOGNIZABLE_CMD);
  size--;

  tms *start_time;
  tms *end_time;
  start_time = getTimes();

  const char* command =  *args;
  if(strcmp(command, "generate") == 0){
    if(size != 3) printUsageAndExit(INVALID_ARGS); // wrong number of parameters

    const char* path      = args[1];
    uint64_t record_count = parseUnsigned(args[2]);
    uint64_t record_size  = parseUnsigned(args[3]);

    generate(path, record_count, record_size);
  }else if(strcmp(command, "sort") == 0){
    if(size != 4) printUsageAndExit(INVALID_ARGS);

    const char* path      = args[1];
    uint64_t record_count = parseUnsigned(args[2]);
    uint64_t record_size  = parseUnsigned(args[3]);
    uint8_t  method       = parseMethod(args[4]);

    sort(path, record_count, record_size, method);
  }else if(strcmp(command, "copy") == 0){
    if(size != 5) printUsageAndExit(INVALID_ARGS);

    const char* in_path   = args[1];
    const char* out_path  = args[2];
    uint64_t record_count = parseUnsigned(args[3]);
    uint64_t record_size  = parseUnsigned(args[4]);
    uint8_t  method       = parseMethod(args[5]);

    copy(in_path, out_path, record_count, record_size, method);
  }else {
    printUsageAndExit(UNRECOGNIZABLE_CMD);
  }

  end_time = getTimes();

  clock_t user_ticks = (end_time->tms_utime - start_time->tms_utime) +
                        (end_time->tms_cutime - start_time->tms_cutime);
  clock_t system_ticks = (end_time->tms_stime - start_time->tms_stime) +
                          (end_time->tms_cstime - start_time->tms_cstime);
  double user_seconds = (double) user_ticks / sysconf(_SC_CLK_TCK);
  double system_seconds = (double) system_ticks / sysconf(_SC_CLK_TCK);

  printf("%20s %20s", "USER", "SYSTEM\n");
  printf("%20lfs %20lfs\n", user_seconds, system_seconds);

  free(start_time);
  free(end_time);
}

int main(int argc, char **argv){
  if (argc < 2) printUsageAndExit(UNRECOGNIZABLE_CMD);

  parseArgs(argc-1, argv+1);

  return 0;
}
