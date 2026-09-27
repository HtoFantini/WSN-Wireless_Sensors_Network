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

sem_t x, y;
int readercount = 0;

// Reader Function
void* reader(void* param)
{
    // Recupera o file descriptor passado diretamente pelo valor do ponteiro
    int client_sv_socket = (int)(intptr_t)param;

    // Entrada na seção crítica dos leitores
    sem_wait(&x);
    readercount++;
    if (readercount == 1) {
        sem_wait(&y); // Primeiro leitor bloqueia escritores
    }
    sem_post(&x);

    printf("\n[Reader] %d reader(s) inside (Socket %d)", readercount, client_sv_socket);

    // Simulação do trabalho de leitura / comunicação com cliente
    sleep(2);

    // Saída dos leitores
    sem_wait(&x);
    readercount--;
    if (readercount == 0) {
        sem_post(&y); // Último leitor libera os escritores
    }
    sem_post(&x);

    printf("\n[Reader] Reader leaving (Socket %d)", client_sv_socket);

    // Fecha a conexão exclusiva desta thread
    close(client_sv_socket);
    pthread_exit(NULL);
}

// Writer Function
void* writer(void* param)
{
    int client_sv_socket = (int)(intptr_t)param;

    printf("\n[Writer] Writer trying to enter (Socket %d)", client_sv_socket);

    // Exclusão mútua total para escrita
    sem_wait(&y);

    printf("\n[Writer] Writer has entered (Socket %d)", client_sv_socket);

    // Simulação do trabalho de escrita / comunicação com cliente
    sleep(2);

    sem_post(&y);

    printf("\n[Writer] Writer leaving (Socket %d)", client_sv_socket);

    // Fecha a conexão exclusiva desta thread
    close(client_sv_socket);
    pthread_exit(NULL);
}

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

    sem_init(&x, 0, 1);
    sem_init(&y, 0, 1);

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
            worker = reader;
        } else if (msg == 2) {
            worker = writer;
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