
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CLIENTS 100
#define PORT 8080
#define BUFFER_SIZE 1024

typedef struct {
  int sock;
  char *name;
} Client;

Client client_sockets[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

void printClients() {
  printf("Clients: [");
  for (int i = 0; i < client_count; i++) {
    printf("%s: %i", client_sockets[i].name, client_sockets[i].sock);
  }
  printf("]\n");
}

void *connection_handler(void *socket_desc) {
  int sock = *(int*)socket_desc;
  char buffer[BUFFER_SIZE] = {0};
  while (1) {
    int n = read(sock, buffer, BUFFER_SIZE);

    if (n <= 0) {
      pthread_mutex_lock(&client_mutex);
      for (int i = 0; i < client_count; i++) {
        if (client_sockets[i].sock == sock) {
          client_sockets[i].sock = -1;
          client_sockets[i].name = NULL;
          client_count--;
          break;
        }
      }

      pthread_mutex_unlock(&client_mutex);
      printf("Client disconnected\n");
      printClients();
      break;
    }

    printf("Received message: %s\n", buffer);

    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < client_count; i++) {
      if (client_sockets[i].sock != sock && client_sockets[i].sock != -1) {
        write(client_sockets[i].sock, buffer, strlen(buffer));
      }
    }
    pthread_mutex_unlock(&client_mutex);
    memset(buffer, 0, BUFFER_SIZE);
  }
  
  close(sock);
  return 0;
}


int main() {
  int server_socket, new_socket;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);

  for (int i = 0; i < MAX_CLIENTS; i++) {
    Client tmp;
    tmp.sock = -1;
    tmp.name = NULL;
    client_sockets[i] = tmp;
  }

  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  int bindConn = bind(server_socket, (struct sockaddr *)&address, sizeof(address));
  if (bindConn < 0) {
    perror("bind failed");
    return 1;
  }

  listen(server_socket, MAX_CLIENTS);
  printf("Server listening on port %d\n", PORT);

  while (1) {
    printf("Waiting for connection...\n");
    new_socket = accept(server_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    printf("Connection accepted\n");

    if (client_count < MAX_CLIENTS) {
      Client new_client;
      new_client.sock = new_socket;
      new_client.name = "Client";
      for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i].sock == -1) {
          client_sockets[i] = new_client;
          break;
        }
      }
      client_count++;
      //client_sockets[client_count++] = new_client;
      printClients();
    }

    pthread_t client_thread;
    if (pthread_create(&client_thread, NULL, connection_handler, (void*)&new_socket) < 0) {
      perror("could not create thread");
      return 1;
    }
    pthread_detach(client_thread);
  }

  return 0;
}
