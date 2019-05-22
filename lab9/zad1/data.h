#ifndef DATA_H
#define DATA_H

/*
 * STATE_WAITING
 * Cart - is accepting passengers
 * Passenger - can enter a cart
 */
#define STATE_WAITING 1

/*
 * STATE_ACTIVE
 * Cart - is on ride
 * Passenger - is on moving cart
 */
#define STATE_ACTIVE  2

/*
 * STATE_READY
 * Cart - 'start' button was pressed
 * Passenger - NUFFIN
 */
#define STATE_READY   4

typedef struct Cart {
    int cart_id;
    int cart_state;
    int rides_done;
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
