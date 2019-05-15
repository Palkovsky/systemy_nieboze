#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "images.h"

/*
 * load_image()
 * Opens file and parses file headers.
 */
int load_image(Image *buf, const char* path)
{
  if(buf == NULL)
  {
    return IMG_NULL_PTR_ERR;
  }

  FILE *fp;
  char *pgma;
  size_t filesz;

  if((fp = fopen(path, "r")) == NULL)
  {
    return IMG_IO_ERR;
  }

  // Acquire filesize
  fseek(fp, 0, SEEK_END);
  filesz = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  pgma = calloc(sizeof(char), filesz);
  if(fread(pgma, sizeof(char), filesz, fp) != filesz)
  {
    return IMG_IO_ERR;
  }

  char *header_sep = "\t\r\n";
  char *body_sep = "\t\r\n ";

  char *magic_byte = strtok(pgma, header_sep);
  if(magic_byte == NULL || strcmp(magic_byte, "P2") != 0)
    { goto img_err; }

  char *name = strtok(NULL, header_sep);
  if(name == NULL)
    { goto img_err; }
  strcpy(buf->name, name);

  char *width = strtok(NULL, body_sep);
  if(width == NULL)
    { goto img_err; }
  buf->width = strtol(width, NULL, 10);

  char *height = strtok(NULL, header_sep);
  if(height == NULL)
    { goto img_err; }
  buf->height = strtol(height, NULL, 10);

  char *colors = strtok(NULL, header_sep);
  if(colors == NULL)
    { goto img_err; }
  buf->colors = strtol(colors, NULL, 10);

  // Initialize 2D image matrix
  unsigned char **pixels = calloc(buf->height, sizeof(char*));
  for(long i=0; i<buf->height; i++)
  {
    pixels[i] = calloc(buf->width, sizeof(char));
  }
  buf->pixels = pixels;

  long row = 0;
  long col = 0;
  char *token;
  for(;;)
  {
    token = strtok(NULL, body_sep);
    if(token == NULL)
      { break; }
    if(col >= buf->width)
      { col=0; row++; }
    if(row >= buf->height)
      {break;}

    unsigned long grayscale = strtoul(token, NULL, 10);
    //printf("%s ==> %d\n", token, (unsigned char) grayscale);
    pixels[row][col] = (char) grayscale;
    col++;
  }

  fclose(fp);
  return IMG_OK;

 img_err:
  fclose(fp);
  return IMG_INVALID_FILE;
}

/*
 * save_image()
 * Puts image data to file.
 */
int save_image(Image *img, const char *path)
{
  if(img == NULL)
    { return IMG_NULL_PTR_ERR; }

  FILE *fp;
  if((fp = fopen(path, "w")) == NULL)
    { return IMG_IO_ERR; }

  fprintf(fp,
          "P2\n%s\n%ld %ld\n%ld\n",
          img->name, img->width, img->height, img->colors);

  int split_cnt = 0;
  for(int row=0; row<img->height; row++)
  {
    for(int col=0; col<img->width; col++)
    {
      // make sure no more than 70 chars in one line
      if(split_cnt + 1 == img->width)
        {fprintf(fp, " %3d\n", img->pixels[row][col]); split_cnt = 0;}
      else
        {fprintf(fp, " %3d", img->pixels[row][col]); split_cnt += 1;}
    }
  }

  fclose(fp);
  return IMG_OK;
}

int reset_image(Image* img)
{
  if(img == NULL)
    { return IMG_NULL_PTR_ERR; }

  for(long i=0; i<img->height; i++)
    { free(img->pixels[i]); }
  free(img->pixels);

  return IMG_OK;
}

int dispose_image(Image *img)
{
  int res;
  if((res = reset_image(img)) != IMG_OK)
    { return res; }
  free(img);
  return IMG_OK;
}

int load_transform(Image_Transform* buf, const char* path)
{
  return 0;
}

int dispose_transform(Image_Transform* trans)
{
  return 0;
}

int apply_transform(Image* img, Image_Transform* trans, long row, long col)
{
  return 0;
}
