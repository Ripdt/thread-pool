#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <unistd.h>
#include <ctype.h>
#include "thpool.h"

#define SOCK_PATH_STRING "/tmp/pipestr"
#define SOCK_PATH_NUMBER "/tmp/pipenum"
#define PIPE_STRING_ID 1
#define PIPE_NUMBER_ID 2
#define QTD_THREADS 2
#define NUMERIC_CONSTANT 10

typedef struct {
    int newsockfd;
    int pipe_type;
} client_data_t;

void process_client(void* arg) {
    client_data_t *data = (client_data_t*) arg;
    int newsockfd = data->newsockfd;
    char buffer[1024];

    // Read data from client
    if (read(newsockfd, buffer, sizeof(buffer)) < 0) {
        perror("Falha em ler do socket");
        close(newsockfd);
        free(data);
        return;
    }

    if (data->pipe_type == PIPE_STRING_ID) {
        for (int i = 0; i < strlen(buffer); i++) {
            buffer[i] = toupper(buffer[i]);
        }
    } else if (data->pipe_type == PIPE_NUMBER_ID) {
        int num = atoi(buffer);
        num += NUMERIC_CONSTANT;
        snprintf(buffer, sizeof(buffer), "%d", num);
    }

    // Write processed data back to client
    if (write(newsockfd, buffer, strlen(buffer) + 1) < 0) {
        perror("Falha em escrever no socket");
    }

    close(newsockfd);
    free(data);
}

int create_and_bind_socket(const char *sock_path) {
    int sockfd;
    struct sockaddr_un local;
    int len;

    // Create socket
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Falha em criar o pipe");
        return -1;
    }

    // Bind socket to local address
    memset(&local, 0, sizeof(local));
    local.sun_family = AF_UNIX;
    strncpy(local.sun_path, sock_path, sizeof(local.sun_path) - 1);
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(sockfd, (struct sockaddr *)&local, len) < 0) {
        perror("Falha em capturar o socket");
        close(sockfd);
        return -1;
    }

    // Listen for connections
    if (listen(sockfd, 5) < 0) {
        perror("Falha em escutar o socket");
        close(sockfd);
        return -1;
    }

    printf("Servidor Named pipe ouvindo em %s...\n", sock_path);

    return sockfd;
}

int main() {
    int sockfd_str, sockfd_num, newsockfd, len;
    struct sockaddr_un remote;
    fd_set readfds;

    // Initialize thread pool
    puts("Inicializando threadpool com 2 threads");
    threadpool thpool = thpool_init(2);

    // Create and bind two sockets
    sockfd_str = create_and_bind_socket(SOCK_PATH_STRING);
    if (sockfd_str < 0) return 1;
    
    sockfd_num = create_and_bind_socket(SOCK_PATH_NUMBER);
    if (sockfd_num < 0) return 1;

    // Accept connections from both sockets
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sockfd_str, &readfds);
        FD_SET(sockfd_num, &readfds);
        int maxfd = (sockfd_str > sockfd_num) ? sockfd_str : sockfd_num;

        // Wait for a connection on either pipe
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("Erro no select");
            break;
        }

        client_data_t *client_data = malloc(sizeof(client_data_t));

        if (FD_ISSET(sockfd_str, &readfds)) {
            // Accept connection on pipe de string
            len = sizeof(remote);
            newsockfd = accept(sockfd_str, (struct sockaddr *)&remote, &len);
            if (newsockfd < 0) {
                perror("Falha em aceitar conexão no pipe de string");
                continue;
            }

            printf("Cliente conectado no pipe de string!\n");
            client_data->pipe_type = PIPE_STRING_ID;
        }
        else if (FD_ISSET(sockfd_num, &readfds)) {
            // Accept connection on pipe numérico
            len = sizeof(remote);
            newsockfd = accept(sockfd_num, (struct sockaddr *)&remote, &len);
            if (newsockfd < 0) {
                perror("Falha em aceitar conexão no pipe numérico");
                continue;
            }

            printf("Cliente conectado no pipe numérico!\n");
            client_data->pipe_type = PIPE_NUMBER_ID;
        }

        client_data->newsockfd = newsockfd;
        thpool_add_work(thpool, process_client, client_data);
    }

    close(sockfd_str);
    close(sockfd_num);
    thpool_destroy(thpool);
    return 0;
}
