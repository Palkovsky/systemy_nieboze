#ifndef DATA_H
#define DATA_H

#define STATE_NOT_ACCEPTING 1
#define STATE_ACCEPTING 2
#define STATE_EXITING 4
#define STATE_RIDING 8
#define STATE_WAITING_FOR_START 16
#define STATE_STARTED 32

typedef struct Cart {
    int cart_id;
    int cart_state;
    int rides_done;
    int passengers_inside;
    struct Cart *next;
    struct Cart *prev;
} Cart;


typedef struct Passenger {
    int passenger_id;
    int passenger_state;
    struct Passenger *next;
    struct Passenger *prev;
} Passenger;

Passenger *init_passenger_queue(int);
Cart *init_roller_coaster(int);

#endif /* DATA_H */
