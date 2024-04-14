
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CLIENTS 100
#define PORT 9000
#define BUFFER_SIZE 1024

int client_sockets[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void printClients() {
  printf("Clients: [ ");
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (client_sockets[i] != -1) {
      printf("%d ", client_sockets[i]);
    }
  }
  printf("]\n");
}

void addClient(int client) {
  pthread_mutex_lock(&lock);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (client_sockets[i] == -1) {
      client_sockets[i] = client;
      client_count++;
      break;
    }
  }
  pthread_mutex_unlock(&lock);
}

void removeClient(int client) {
  pthread_mutex_lock(&lock);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (client_sockets[i] == client) {
      client_sockets[i] = -1;
      break;
    }
  }
  client_count--;
  pthread_mutex_unlock(&lock);
}

void sendToAllClients(char *message, int socket) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (client_sockets[i] != -1 && client_sockets[i] != socket) {
      write(client_sockets[i], message, strlen(message));
    }
  }
}

void *connection_handler(void *socket_desc) {
  int sock = *(int*)socket_desc;
  char buffer[BUFFER_SIZE] = {0};

  while (1) {
    int n = read(sock, buffer, BUFFER_SIZE);

    if (n <= 0) {
      removeClient(sock);
      printf("Client disconnected\n");
      printClients();
      break;
    }

    printf("Received message: %s\n", buffer);
    sendToAllClients(buffer, sock);
    memset(buffer, 0, BUFFER_SIZE);
  }

  close(sock);
  pthread_exit(0);
  return 0;
}


int main() {
  int sockfd, new_socket;
  struct sockaddr_in address;

  int opt = 1;
  int addrlen = sizeof(address);

  for (int i = 0; i < MAX_CLIENTS; i++) {
    client_sockets[i] = -1;
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  int bindConn = bind(sockfd, (struct sockaddr *)&address, sizeof(address));
  if (bindConn < 0) {
    perror("bind failed");
    return 1;
  }

  listen(sockfd, MAX_CLIENTS);
  printf("Server listening on port %d\n", PORT);

  while (1) {
    printf("Waiting for connection...\n");
    if (client_count < MAX_CLIENTS) {
      new_socket = accept(sockfd, (struct sockaddr *)&address, (socklen_t*)&addrlen);

      if (new_socket < 0) {
        perror("accept failed");
        continue;
      }

      printf("Connection accepted\n");
      addClient(new_socket);
      printClients();

      pthread_t client_thread;
      int *new_socket_ptr = malloc(sizeof(int));
      *new_socket_ptr = new_socket;

      if (pthread_create(&client_thread, NULL, connection_handler, (void*)new_socket_ptr) < 0) {
        perror("could not create thread");
        free(new_socket_ptr); // Free memory on fail
        removeClient(new_socket);
        return 1;
      }
      pthread_detach(client_thread);
    } else {
      printf("Max clients reached\n");
    }
  }
  return 0;
}
