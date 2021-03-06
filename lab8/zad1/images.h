#ifndef IMAGES_H
#define IMAGES_H

#define IMG_OK 0
#define IMG_IO_ERR 1
#define IMG_NULL_PTR_ERR 2
#define IMG_TRANSFORM_ERR 3
#define IMG_INVALID_FILE 4

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

typedef struct Image
{
  char name[256];
  long width;
  long height;
  long colors;
  unsigned char **pixels;
} Image;

typedef struct Image_Transform
{
  long size;
  double **values;
} Image_Transform;

// Interface for image-manipulation
int load_image(Image*, const char*);
int save_image(Image*, const char*);
int reset_image(Image*);
int dispose_image(Image*);

int load_transform(Image_Transform*, const char*);
int dispose_transform(Image_Transform*);

int apply_on_pixel(Image*, Image_Transform*, long, long);
int apply_on_image(Image*, Image_Transform*);

#endif /* IMAGES_H */
