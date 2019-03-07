#ifndef MY_LIBRARY_H
#define MY_LIBRARY_H

#include <stdint.h>

typedef struct  mylib_block_array {
  uint32_t size;
  char **blocks;
} mylib_block_array;

/*
  Initializes fields of block array.
  Returns NULL on failure.
 */
mylib_block_array* mylib_InitArray(uint32_t size);

/*
  Sets values of private values used for performing searches.
  Returns 0 on success.
 */
int8_t mylib_SetFindParams(const char* dir, const char* lookup_string, const char* temp_target);

/*
  Tries to perform find with currently stored parameter strings(dir, lookup_string)
  and then write results to temporray file.
  0 - Sucess
  -1 - Find params not set
  -2 - Invalid directory
  -3 - Unable to execute find
 */
int8_t mylib_PerformFind();

/*
  Moves current contents of temporary file to mylib_block_array.
  0 or positive - idx of new element
  -1 - arr == NULL
  -2 - Unable to read temporary file
  -3 - No free block available
 */
int64_t mylib_MoveTemporaryFileToArray(mylib_block_array* arr);

/*
  Removes block on specified index.
  0 - Success
  -1 - arr == NULL
  -2 - index not in array range
  -3 - field already empty
 */
int8_t mylib_RemoveEntry(mylib_block_array *arr, uint32_t idx);

/*
  Frees up memory taken by my_block_array struct and block array.
  0 - success
  1 - arr == NULL
*/
int8_t mylib_DisposeArray(mylib_block_array* arr);

const char* mylib_GetItem(mylib_block_array* arr, uint32_t idx);

#endif //MY_LIBRARY_H
