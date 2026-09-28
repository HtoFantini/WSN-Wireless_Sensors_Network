#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>

#include "../include/sensors.h"


typedef struct {
    int sv_socket;
    SensorType type;
    int active;
} Subscriber;



void add_subscriber(int sv_socket, SensorType type);
void remove_subscriber(int sv_socket);
void broadcast(Sensor *s_sub, int sender_sv_socket);
void* handle_client(void* arg);
int main(int argc, char *argv[]);


#endif // SERVER_H