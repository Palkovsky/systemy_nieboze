#ifndef DATA_H
#define DATA_H

#define FTOK_SHM_PATH "/tmp"
#define FTOK_SHM_SEED 2137
#define FTOK_SEM_PATH "/tmp"
#define FTOK_SEM_SEED 2138
#define SHM_PERM 0600
#define SEM_PERM 0600

#define LOADER_EXECUTABLE "loader.out"

#define MAX_LINE_CAPACITY 1024 // To prevent SHM getting too large
#define ERR_CAP_REACHED 1
#define ERR_WEIGHT_CAP_REACHED 2

typedef struct prod_node
{
  long weight;
  pid_t producer;
  long ordnum;
  unsigned long timestamp;
} prod_node;

typedef struct prod_line
{
  long capacity;
  long weight_capacity;
  long tail;
  long head;
  long size;
  prod_node items[MAX_LINE_CAPACITY];
} prod_line;


// Interface for interacting with prod_line
prod_line *line_new(long, long);
prod_node *line_oldest(prod_line*);
long line_put(prod_line*, long, long);
void line_dispose(prod_line*);
void line_clear(prod_line*);
long line_weight(prod_line*);
long line_free_space(prod_line*);
long line_free_weight(prod_line*);

#endif /* DATA_H */
