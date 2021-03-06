#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>

#include "data.h"

prod_line *line_new(long capacity, long weight)
{
  prod_line *line = (prod_line*) malloc(sizeof(prod_line));
  if(line == NULL)
  {
    return NULL;
  }

  line->capacity = capacity;
  line->weight_capacity = weight;
  line->tail = 0;
  line->head = 0;
  line->size = 0;

  return line;
}

void line_dispose(prod_line *line)
{
  free(line);
}

void line_clear(prod_line *line)
{
  line->tail = 0;
  line->head = 0;
  line->size = 0;
}

long line_weight(prod_line *line)
{
  long i = line->head;
  long weight = 0;

  for(int j=0; j<line->size; j++)
  {
    weight += line->items[i].weight;
    i++;
    if(i >= MAX_LINE_CAPACITY)
    {
      i = 0;
    }
  }

  return weight;
}

long line_free_space(prod_line *line)
{
  return line->capacity - line->size;
}


long line_free_weight(prod_line *line)
{
  return line->weight_capacity - line_weight(line);
}

prod_node *line_oldest(prod_line *line)
{
  if(line->size == 0)
  {
    return NULL;
  }

  prod_node *item = line->items + line->head;
  line->size--;
  line->head++;

  if(line->head >= MAX_LINE_CAPACITY)
  {
    line->head = 0;
  }

  return item;
}

long line_put(prod_line *line, long weight, long ordnum)
{
  if(line->capacity < line->size + 1 || MAX_LINE_CAPACITY < line->size + 1)
  {
    return ERR_CAP_REACHED;
  }
  if(line->weight_capacity < line_weight(line) + weight)
  {
      return ERR_WEIGHT_CAP_REACHED;
  }

  prod_node *item = line->items + line->tail;
  item->weight = weight;
  item->ordnum = ordnum;
  item->producer = getpid();

  struct timeval current_time;
	gettimeofday(&current_time, NULL);
  item->timestamp = current_time.tv_sec * (int) 1e6 + current_time.tv_usec;

  line->size++;
  line->tail++;
  if(line->tail >= MAX_LINE_CAPACITY)
  {
    line->tail = 0;
  }

  return 0;
}


