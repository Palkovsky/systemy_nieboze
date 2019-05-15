#ifndef IMAGES_H
#define IMAGES_H

#define IMG_OK 0
#define IMG_IO_ERR 1
#define IMG_NULL_PTR_ERR 2
#define IMG_TRANSFORM_ERR 3
#define IMG_INVALID_FILE 4

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
  float **value;
} Image_Transform;

// Interface for image-manipulation
int load_image(Image*, const char*);
int save_image(Image*, const char*);
int reset_image(Image*);
int dispose_image(Image*);

int load_transform(Image_Transform*, const char*);
int dispose_transform(Image_Transform*);

int apply_transform(Image*, Image_Transform*, long, long);

#endif /* IMAGES_H */
