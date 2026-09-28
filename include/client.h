#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>

#include "../include/sensors.h"


typedef struct {
    char ip[16];
    int port;
    int client_request;
} ClientConfig;

typedef struct {
    int client_socket;
    Sensor *s_main;
    Sensor *neighbours;
    pthread_mutex_t *lock;
} SensorContext;



void MsgReceiveUpdater(Sensor *s, Sensor *s_candidate, Sensor neighbours[]);
void* SensorThreadCallback(void* arg);
void* SensorManager(SensorType type, int coordX, int coordY, void* args);
int main(int argc, char *argv[]);


#endif // CLIENT_H