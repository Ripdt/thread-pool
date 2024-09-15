#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include "thpool.h"

#define SOCK_PATH "/tmp/pipeso"

typedef struct {
    int newsockfd;
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

    printf("Dado recebido: %s\n", buffer);

    // Process data (convert to uppercase)
    for (int i = 0; i < strlen(buffer); i++) {
        buffer[i] = toupper(buffer[i]);
    }

    sleep(5); // debug

    // Write processed data back to client
    if (write(newsockfd, buffer, strlen(buffer) + 1) < 0) {
        perror("Falha em escrever no socket");
    }

    printf("Dado enviado de volta para o cliente.\n");

    close(newsockfd);
    free(data); // Liberar a memória alocada
}

int main() {
    int sockfd, newsockfd, len;
    struct sockaddr_un local, remote;

    // Create socket
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Falha em criar o pipe");
        return 1;
    }

    // Bind socket to local address
    memset(&local, 0, sizeof(local));
    local.sun_family = AF_UNIX;
    strncpy(local.sun_path, SOCK_PATH, sizeof(local.sun_path) - 1);
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(sockfd, (struct sockaddr *)&local, len) < 0) {
        perror("Falha em capturar o socket");
        close(sockfd);
        return 1;
    }

    // Listen for connections
    if (listen(sockfd, 5) < 0) {
        perror("Falha em escutar o socket");
        close(sockfd);
        return 1;
    }

    printf("Servidor Named pipe ouvindo em %s...\n", SOCK_PATH);

    // Initialize thread pool with 2 threads
    puts("Inicializando threadpool com 2 threads");
    threadpool thpool = thpool_init(2);

    // Accept connections
    memset(&remote, 0, sizeof(remote));
    len = sizeof(remote);
    while (newsockfd = accept(sockfd, (struct sockaddr *)&remote, &len)) {
        if (newsockfd < 0) {
            perror("Falha em aceitar conexão");
            close(sockfd);
            thpool_destroy(thpool); // Destruir o threadpool ao sair
            return 1;
        }

        printf("Cliente conectado!\n");

        // Allocate memory for client data and add work to the thread pool
        client_data_t *client_data = malloc(sizeof(client_data_t));
        client_data->newsockfd = newsockfd;
        thpool_add_work(thpool, process_client, client_data);
    }

    close(sockfd);
    thpool_destroy(thpool); // Destruir o threadpool ao sair
    return 0;
}
