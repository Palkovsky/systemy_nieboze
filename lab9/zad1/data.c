#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>

#include "data.h"

Passenger *init_passenger_queue(int size)
{
    Passenger *head;
    Passenger *current;

    head = malloc(sizeof(Passenger) * size);
    current = head;
    
    for(int i=0; i<size; i++)
    {
        current->passenger_id = i;
        current->passenger_state = STATE_NOT_ACCEPTING;

        if(i > 0)
            { current->prev = current-1;}
        if(i < size-1)
            { current->next = current+1;}

        current++;
    }

    current--;
    current->next = head;
    head->prev = current;

    return head;
}

Cart *init_roller_coaster(int size){
    Cart *head;
    Cart *current;

    head = malloc(sizeof(Cart) * size);
    current = head;
    
    for(int i=0; i<size; i++)
    {
        current->cart_id = i;
        current->cart_state = STATE_NOT_ACCEPTING;
        current->rides_done = 0;
        current->passengers_inside = 0;

        if(i > 0)
            { current->prev = current-1;}
        if(i < size-1)
            { current->next = current+1;}

        current++;
    }

    current--;
    current->next = head;
    head->prev = current;

    return head;
}
