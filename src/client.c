#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <string.h>
#include <stdbool.h>

#include <arpa/inet.h>
#include <unistd.h>

#include <pthread.h>
#include <semaphore.h>

#include "utils.c"
#include "sensors.c"

typedef struct {
    char ip[16];
    int port;
    int client_request;
} ClientConfig;

// TODO: ADAPTAR ESTA FUNCAO PARA RECEBER ARUGMENTOS VIA *ARGS
void funcao_callback_event(Sensor s, Sensor s_candidate, Sensor neighbours[]){
    bool is_ngb;

    is_ngb = CalcNewPossibleNgb(s, s_candidate, neighbours);
    if(is_ngb){
        UpdateMeasure(&s,neighbours);
    }
}

void* SensorThread(SensorType type, int coordX, int coordY, void* args){
    // CONECTION HANDLING
    ClientConfig *config = (ClientConfig*)args;

    int network_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (network_socket < 0) {
        perror("Erro ao criar socket\n");
        return NULL;
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(config->port);

    // Converte a string de IP informada na linha de comando
    if (inet_pton(AF_INET, config->ip, &server_address.sin_addr) <= 0) {
        fprintf(stderr, "Error: Invalid IP address'%s'\n", config->ip);
        close(network_socket);
        return NULL;
    }

    // Tenta conectar ao IP e porta configurados
    if (connect(network_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Erro na conexao com o servidor\n");
        close(network_socket);
        return NULL;
    }

    printf("Conexao estabelecida com sucesso com %s:%d\n", config->ip, config->port);

    // ==============================================

    Sensor s_main;
    strcpy(s_main.type, SensorTypeToString(type));
    InitSensMeasure(&s_main);
    Sensor neighbours[NUM_NEIGHBOURS];
    InitNgbsVec(neighbours);
    
    // TENNHO QUE FAZER UM JEITO MELHOR DE JA STARTAR COM O TIPO DEFINIDO
    int msg_timer; 
    if(SensorTypeToString(type) == "temperature") msg_timer = TEMP_TIMER;
    if(SensorTypeToString(type) == "humidity") msg_timer = HUMID_TIMER;
    if(SensorTypeToString(type) == "air_quality") msg_timer = AIR_QUAL_TIMER;

    // EVENT HALDER START
    // EventCall(funcao_callback_event)

    while(1) {
        sleep(msg_timer);
        // O SEND TEM QUE TER PRIORIDADE SOBRE O EVENTO
        // SERA QUE TIRAR O CALCULO DOS VIZINHOS DE PONTEIRO E USAR COPIA RESOLVE
        // UM POSSIVEL ACESSO SIMULTANEO DE MEMORIA ENTRE ESTA FUNCAO E O EVENTO
        send(network_socket, &s_main, sizeof(&s_main), 0);

        // O EVENTO E O PROPRIO RECEIVE, MAS COMO FAZER ISSO
    }

    close(network_socket);
    pthread_exit(NULL);
}


void* clienthread(void* args)
{
    ClientConfig *config = (ClientConfig*)args;

    int network_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (network_socket < 0) {
        perror("Erro ao criar socket\n");
        return NULL;
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(config->port);

    // Converte a string de IP informada na linha de comando
    if (inet_pton(AF_INET, config->ip, &server_address.sin_addr) <= 0) {
        fprintf(stderr, "Error: Invalid IP address'%s'\n", config->ip);
        close(network_socket);
        return NULL;
    }

    // Tenta conectar ao IP e porta configurados
    if (connect(network_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Erro na conexao com o servidor\n");
        close(network_socket);
        return NULL;
    }

    printf("Conexao estabelecida com sucesso com %s:%d\n", config->ip, config->port);

    // Envia a escolha do cliente (1 = reader, 2 = writer, etc.)
    send(network_socket, &config->client_request, sizeof(config->client_request), 0);

    close(network_socket);
    pthread_exit(NULL);
}

// Driver Code
int main(int argc, char *argv[]) {

    int port,coordsX, coordsY; 
    char *ip;
    SensorType sensor_type;

///////////////// ERROR HANDLING ///////////////////
    if(argc != 8){
        printf("Error: Invalid number of arguments\n");
        printf("Usage: %s <server_ip> <port> -type <temperature|humidity|air_quality> -coords <x> <y>\n", argv[0]);
        return -1;
    }
    else{
        // VAR INIT
        ip = argv[1];
        sensor_type = StringToSensorType(argv[4]);

        port = atoi(argv[2]); 
        coordsX = atoi(argv[6]);
        coordsY = atoi(argv[7]);
        ///////////

        if(strcmp(argv[3], "-type") != 0){
            printf("Error: Expected '-type' argument\n");
            printf("Usage: %s <server_ip> <port> -type <temperature|humidity|air_quality> -coords <x> <y>\n", argv[0]);
            return -1;
        }
        if(strcmp(argv[5], "-coords") != 0){
            printf("Error: Expected '-coords' argument\n");
            printf("Usage: %s <server_ip> <port> -type <temperature|humidity|air_quality> -coords <x> <y>\n", argv[0]);
            return -1;
        }
        if (sensor_type == SENSOR_UNKNOWN) {
            printf("Error: Invalid sensor type\n");
            printf("Usage: %s <server_ip> <port> -type <temperature|humidity|air_quality> -coords <x> <y>\n", argv[0]);
            return -1;
        }

        if(coordsX < 0 || coordsY < 0 ||
           coordsX > 9 || coordsY > 9){
            printf("Error: Coordinates must be in the range 0-9\n");
            printf("Usage: %s <server_ip> <port> -type <temperature|humidity|air_quality> -coords <x> <y>\n", argv[0]);
            return -1;
        }
    }
/////////////////////////////////////////////////////

    printf("Iniciando configuração do cliente...\n");
    ClientConfig config;
    strncpy(config.ip, ip, sizeof(config.ip) - 1);
    config.ip[sizeof(config.ip) - 1] = '\0';
    config.port = port;
    printf("Configuração concluida!\n");

    pthread_t tid;

    switch (sensor_type) {
        // temp
        case 0: {
            config.client_request = 0;

            if (pthread_create(&tid, NULL, clienthread, &config) != 0) {
                perror("Erro ao criar thread do cliente\n");
                return -1;
            }
            sleep(20);
            break;
        }
        // humidity
        case 1: {
            config.client_request = 1;

            // Create thread
            if (pthread_create(&tid, NULL, clienthread, &config) != 0) {
                perror("Erro ao criar thread do cliente\n");
                return -1;
            }
            sleep(20);
            break;
        }
        //air_quality
        case 2: {
            config.client_request = 2;

            if (pthread_create(&tid, NULL, clienthread, &config) != 0) {
                perror("Erro ao criar thread do cliente\n");
                return -1;
            }
            sleep(20);
            break;
        }
        default:
            printf("Error: Invalid sensor type\n");
            break;
    }
    
    // Suspend execution of
    // calling thread
    pthread_join(tid, NULL);
}