#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>
#include <math.h>

#include "images.h"

#define SPLIT_BLOCK 1
#define SPLIT_INTERLEAVED 2

typedef struct ThreadArg {
  Image *image;
  Image_Transform *trans;
  long start_num;
  long type;
  long thread_count;
} ThreadArg;

void *thread_logic(void*);

/*
 * Helper functions.
 */
unsigned long get_timestamp(void);
void print_image(Image*);
void print_transform(Image_Transform*);
void print_usage(const char*);

int main(int argc, char **argv)
{
  if(argc != 6)
  {
    printf("Invalid number of arguments.\n");
    print_usage(argv[0]);
    exit(1);
  }

  long thread_count;
  long split_type;
  char *input_path;
  char *filter_path;
  char *output_path;
  pthread_t *threads;

  // Load thread count.
  thread_count = strtol(argv[1], NULL, 10);
  if(thread_count <= 0)
    { printf("Thread count must be positive.\n"); exit(1); }

  // Load split type.
  if(strcmp(argv[2], "interleaved") == 0)
    { split_type = SPLIT_INTERLEAVED; }
  else if(strcmp(argv[2], "block") == 0)
    { split_type = SPLIT_BLOCK; }
  else
    { printf("Split type must be eiter 'interleaved' or 'block'."); exit(1); }

  // Load paths.
  input_path = argv[3];
  filter_path = argv[4];
  output_path = argv[5];

  // Load image and image filter.
  Image_Transform *trans = malloc(sizeof(Image_Transform));
  Image *img = malloc(sizeof(Image));
  if(load_transform(trans, filter_path) != IMG_OK)
    { printf("Unable to load filter file %s.\n", filter_path);  exit(1); }
  print_transform(trans);

  if(load_image(img, input_path) != IMG_OK)
    { printf("Unable to load image from %s.\n", input_path); exit(1); }
  print_image(img);

  unsigned long start_time = get_timestamp();

  // Start threads
  threads = calloc(thread_count, sizeof(pthread_t));
  for(long i=0; i<thread_count; i++)
  {
    pthread_t tid;
    ThreadArg *arg = malloc(sizeof(ThreadArg));
    arg->image = img;
    arg->trans = trans;
    arg->start_num = i;
    arg->type = split_type;
    arg->thread_count = thread_count;

    if(pthread_create(&tid, NULL, thread_logic, arg) != 0)
      { printf("Error while startind %ld thread.\n", i); exit(1); }

    threads[i] = tid;
  }

  // Wait for threads to finish the work.
  for(long i=0; i<thread_count; i++)
  {
    void *ret;
    if(pthread_join(threads[i], &ret) != 0)
      { printf("Error while joining %ld thread.\n", i); exit(1); }

    unsigned long time = *((unsigned long*) ret);
    printf("Thread %ld took %ld.\n", i, time);
    free(ret);
  }

  printf("Overall time taken: %ld\n", get_timestamp() - start_time);

  // Save transformed image to output file.
  if(save_image(img, output_path) != IMG_OK)
    { printf("Error while saving output image to %s.\n", output_path); exit(1); }

  // Clean up memory
  dispose_transform(trans);
  dispose_image(img);
  free(threads);

  return 0;
}

/*
 * thread_logic()
 * Performs filtering operations on chunk of image.
 */
void *thread_logic(void *_arg)
{
  ThreadArg *arg = (ThreadArg*) _arg;
  Image *img = arg->image;
  Image_Transform *trans = arg->trans;
  unsigned long start_time = get_timestamp();

  long k = arg->start_num;
  if(arg->type == SPLIT_BLOCK)
  {
    long x0 = k * ceil(img->width/arg->thread_count);
    long x1 = (k+1) * ceil(img->width/arg->thread_count) - 1;
    //printf("Thread %ld: Img width: %ld, Thread Count: %ld\n", k, img->width, arg->thread_count);
    //printf("Thread %ld: Start %ld, End: %ld\n", k, x0, x1);
    while(x0 <= x1)
    {
      for(long row=0; row<img->height; row++)
        { apply_on_pixel(img, trans, row, x0); }
      x0++;
    }
  }
  else if(arg->type == SPLIT_INTERLEAVED)
  {
    for(long row=0; row < img->height; row++)
    {
      long x0 = k;
      while(x0 < img->width)
      {
        apply_on_pixel(img, trans, row, x0);
        x0 += arg->thread_count;
      }
    }
  }

  unsigned long *timestamp = malloc(sizeof(unsigned long));
  *timestamp = get_timestamp() - start_time;
  free(arg);
  pthread_exit(timestamp);
}

/*
 * Helpers
 */
unsigned long get_timestamp()
{
  struct timeval current_time;
	gettimeofday(&current_time, NULL);
  return current_time.tv_sec * (int) 1e6 + current_time.tv_usec;
}

void print_image(Image *img)
{
  printf("Img name: %s\n", img->name);
  printf("Dimen: %ldx%ld\n", img->width, img->height);
  printf("Colors: %ld\n", img->colors);
  printf("Size: %ld\n", sizeof(img->pixels));
}

void print_transform(Image_Transform *trans)
{
  long sz = trans->size;
  double **arr = trans->values;
  printf("Size: %ldx%ld\n", sz, sz);
  for(int row=0; row<sz; row++)
  {
    for(int col=0; col<sz; col++)
      { printf("%f ", arr[row][col]); }
    printf("\n");
  }
}

void print_usage(const char* progname)
{
  printf("Usage: %s <thread_count> <split_type> <input_img> <filter> <output_img> \n \
         \r thread_count - number of threads to start. Must be positive. \n \
         \r split_type - block/interleaved \n \
         \r input_img - path to image for processing \n \
         \r filter - path to file with filter specification \n \
         \r output_img - path to output file\n", progname);
}
