#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "../include/sensors.h"
#include "../include/client.h"



void MsgReceiveUpdater(Sensor *s, Sensor *s_candidate, Sensor neighbours[]){
    MsgAction action;
    float measure_correction;
    float initial_measure = s->measurement;

    action = CalcNewPossibleNgb(*s, *s_candidate, neighbours);

    if(action == IS_NGB) {
        //
        //printf("\n[EVENTO] É vizinho! [%d][%d]!\nMedição Atual:%f\n", s_candidate->coords[0], s_candidate->coords[1], s->measurement);
        UpdateMeasure(s,neighbours);
    }

    measure_correction = s->measurement - initial_measure;

    PrintClientLogMsg(*s,*s_candidate,action,measure_correction,0);
}


void* SensorThreadCallback(void* arg) {
    SensorContext *ctx = (SensorContext*)arg;
    Sensor s_incoming;

    // Fica bloqueado em recv(). Desperta imediatamente quando o Broker manda algo
    while (recv(ctx->client_socket, &s_incoming, sizeof(Sensor), 0) > 0) {
        // DEBUG
        //printf("\n[EVENTO] Mensagem do sensor: [%d][%d]!\nValor recebido:%f\n", s_incoming.coords[0], s_incoming.coords[1], s_incoming.measurement);

        pthread_mutex_lock(ctx->lock);

        MsgReceiveUpdater(ctx->s_main, &s_incoming, ctx->neighbours);
        // DEBUG
        //printf("\n[EVENTO] Medição recalculada para:: %.2f\n\n\n", ctx->s_main->measurement);

        pthread_mutex_unlock(ctx->lock);
    }

    printf("Inscrição no topico finalizada.\n");
    return NULL;
}

void* SensorManager(SensorType type, int coordX, int coordY, void* args){
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

    // ==================== DADOS DO SENSOR ==========================

    Sensor s_main;
    ConfigSensorVals(&s_main, type, coordX, coordY);

    Sensor neighbours[NUM_NEIGHBOURS];
    InitNgbsVec(neighbours);
    
    // Mutex to control access to sensor
    pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;

    // Envia mensagem para registrar o sensor no servidor
    send(network_socket, &s_main, sizeof(Sensor), 0);

    // ========= EMPACOTAMENTO DE ARGUMENTOS ==========
    SensorContext ctx;
    ctx.client_socket = network_socket;
    ctx.s_main = &s_main;
    ctx.neighbours = neighbours;
    ctx.lock = &data_mutex;


    // EVENT HANDLING
    pthread_t event_handler_thread;
    if (pthread_create(&event_handler_thread, NULL, SensorThreadCallback, (void*)&ctx) != 0) {
                perror("Erro ao criar thread do cliente\n");
                close(network_socket);
                return NULL;
    }
    pthread_detach(event_handler_thread);

    // ============ DEFINICAO DO TIMER ===============
    int msg_timer = 0;
    switch(type){
        case SENSOR_TEMP: msg_timer = TEMP_TIMER; break;
        case SENSOR_HUMIDITY: msg_timer = HUMID_TIMER; break;
        case SENSOR_AIR_QUALITY: msg_timer = AIR_QUAL_TIMER; break;
        default: break;
    }
    
    // ============ LOOP PRINCIPAL ===============
    while(1) {
        sleep(msg_timer);
        
        pthread_mutex_lock(&data_mutex);
        Sensor s_copy = s_main;
        pthread_mutex_unlock(&data_mutex);
        
        // DEBUG
        //printf("[Timer Expirado] Publicando leitura: %.2f\n", s_copy.measurement);
        send(network_socket, &s_copy, sizeof(Sensor), 0);
    }

    close(network_socket);
    pthread_mutex_destroy(&data_mutex);
    pthread_exit(NULL);
    return NULL;
}



// Driver Code
int main(int argc, char *argv[]) {

    int port,coordX,coordY; 
    char *ip;
    SensorType sensor_type;
    
    int seed = time(NULL);
    srand(seed);

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
        coordX = atoi(argv[6]);
        coordY = atoi(argv[7]);
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

        if(coordX < 0 || coordY < 0 ||
           coordX > 9 || coordY > 9){
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

    SensorManager(sensor_type, coordX, coordY, &config);
}