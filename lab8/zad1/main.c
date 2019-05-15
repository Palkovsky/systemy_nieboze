#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>

#include "images.h"

unsigned long get_timestamp(void);
void print_image(Image*);
void print_transform(Image_Transform*);

int main(int argc, char **argv)
{

  Image_Transform *trans = malloc(sizeof(Image_Transform));
  Image *img = malloc(sizeof(Image));


  int err;
  if((err = load_transform(trans,  "../box_blur.trans")) != IMG_OK)
  {
    printf("Error while loading transform: %d.\n", err);
    exit(1);
  }
  print_transform(trans);


  if((err = load_image(img, "../balloons.ascii.pgm")) != IMG_OK)
  {
    printf("Error while loading image: %d.\n", err);
    exit(1);
  }
  print_image(img);

  apply_on_image(img, trans);
  apply_on_image(img, trans);
  apply_on_image(img, trans);

  if(save_image(img, "../balloons_blur.ascii.pgm") != IMG_OK)
  {
    printf("Error while saving mona-lisa copy.");
    exit(1);
  }

  dispose_transform(trans);
  dispose_image(img);

  /*
  printf("OUT: %d\n", load_image(img, "../screws.ascii.pgm"));
  print_image(img);
  if(save_image(img, "../screws_cpy.ascii.pgm") != IMG_OK)
  {
    printf("Error while saving screws copy.");
    exit(1);
  }

  reset_image(img);
  printf("OUT: %d\n", load_image(img, "../barbara.ascii.pgm"));
  print_image(img);
  if(save_image(img, "../barbara_cpy.ascii.pgm") != IMG_OK)
  {
    printf("Error while saving barbara copy.");
    exit(1);
  }

  reset_image(img);
  printf("OUT: %d\n", load_image(img, "../balloons.ascii.pgm"));
  print_image(img);
  if(save_image(img, "../balloons_cpy.ascii.pgm") != IMG_OK)
    {
      printf("Error while saving barbara copy.");
      exit(1);
    }
    dispose_image(img);
  */
  
  return 0;
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
