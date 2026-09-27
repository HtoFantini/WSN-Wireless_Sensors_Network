#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>


#include "utils.c"
#include "sensors.c"

#define MAX_SUBSCRIBERS MATRIX_SIZE*MATRIX_SIZE

typedef struct {
    int sv_socket;
    SensorType type;
    int active;
} Subscriber;

Subscriber subscribers[MAX_SUBSCRIBERS];
pthread_mutex_t sub_mutex = PTHREAD_MUTEX_INITIALIZER;

// Adiciona um nó conectado à lista de assinantes
void add_subscriber(int sv_socket, SensorType type) {
    pthread_mutex_lock(&sub_mutex);
    for (int i = 0; i < MAX_SUBSCRIBERS; i++) {
        if (!subscribers[i].active) {
            subscribers[i].sv_socket = sv_socket;
            subscribers[i].type = type;
            subscribers[i].active = 1;
            break;
        }
    }
    pthread_mutex_unlock(&sub_mutex);
}


// Remove o nó caso desconecte
void remove_subscriber(int sv_socket) {
    pthread_mutex_lock(&sub_mutex);
    for (int i = 0; i < MAX_SUBSCRIBERS; i++) {
        if (subscribers[i].active && subscribers[i].sv_socket == sv_socket) {
            subscribers[i].active = 0;
            close(sv_socket);
            break;
        }
    }
    pthread_mutex_unlock(&sub_mutex);
}


// Dispara a mensagem para todos os assinantes do mesmo tipo
void broadcast(Sensor *s_sub, int sender_sv_socket) {
    pthread_mutex_lock(&sub_mutex);
    for (int i = 0; i < MAX_SUBSCRIBERS; i++) {
        if (subscribers[i].active && 
            subscribers[i].type == s_sub->type && 
            subscribers[i].sv_socket != sender_sv_socket) 
        {
            send(subscribers[i].sv_socket, s_sub, sizeof(Sensor), 0);
        }
    }
    pthread_mutex_unlock(&sub_mutex);
}

// Thread dedicada a escutar cada conexão persistente
void* handle_client(void* arg) {
    int client_sv_socket = (int)(intptr_t)arg;
    Sensor s_msg;

    // Primeiro pacote esperado: Registro inicial
    if (recv(client_sv_socket, &s_msg, sizeof(Sensor), 0) <= 0) {
        close(client_sv_socket);
        return NULL;
    }

    add_subscriber(client_sv_socket, s_msg.type);
    printf("[Broker] Sensor tipo %d conectado (Socket %d)\n", s_msg.type, client_sv_socket);

    // Loop contínuo: qualquer dado novo que chegar vira broadcast
    while (recv(client_sv_socket, &s_msg, sizeof(Sensor), 0) > 0) {
        printf("[Broker] Reenviando leitura (%.2f) do tipo %d para o grupo\n", 
               s_msg.measurement, s_msg.type);
        broadcast(&s_msg, client_sv_socket);
    }

    printf("[Broker] Sensor desconectado (Socket %d)\n", client_sv_socket);
    remove_subscriber(client_sv_socket);
    return NULL;
}


int readercount = 0;

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Error: Invalid number of arguments\n");
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        return -1;
    }

    const char *ip_str = argv[1];
    int port = atoi(argv[2]);

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Error: Invalid port number '%s' (range from 1 to 65535).\n", argv[2]);
        return 1;
    }

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("Erro ao criar socket");
        return 1;
    }

    // Permite reutilizar a porta imediatamente se o servidor for reiniciado
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip_str, &serverAddr.sin_addr) <= 0) {
        fprintf(stderr, "Erro: IP '%s' invalido.\n", ip_str);
        close(serverSocket);
        return 1;
    }

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Erro no bind");
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 50) < 0) {
        perror("Erro no listen");
        close(serverSocket);
        return 1;
    }

    printf("Servidor escutando em %s:%d\n", ip_str, port);

    while (1) {
        struct sockaddr_storage clientStorage;
        socklen_t addr_size = sizeof(clientStorage);

        int newSocket = accept(serverSocket, (struct sockaddr*)&clientStorage, &addr_size);
        if (newSocket < 0) {
            perror("Erro no accept");
            continue;
        }

        // Lê a escolha do cliente (1 = reader, 2 = writer)
        int msg = 0;
        ssize_t bytes_recv = recv(newSocket, &msg, sizeof(msg), 0);
        if (bytes_recv <= 0) {
            close(newSocket);
            continue;
        }

        pthread_t tid;
        void* (*worker)(void*) = NULL;
        
        // É AQUI QUE FAREMOS A DIFERENCIACAO DO TIPO
        if (msg == 1) {
            worker = "reader";
        } else if (msg == 2) {
            worker = "writer";
        } else {
            fprintf(stderr, "Opcao desconhecida (%d) do socket %d\n", msg, newSocket);
            close(newSocket);
            continue;
        }

        // Passa o socket diretamente por VALOR encapsulado no ponteiro
        if (pthread_create(&tid, NULL, worker, (void*)(intptr_t)newSocket) != 0) {
            perror("Falha ao criar thread");
            close(newSocket);
        } else {
            // Desassocia a thread para limpar recursos da stack automaticamente ao terminar
            pthread_detach(tid);
        }
    }

    close(serverSocket);
    return 0;
}