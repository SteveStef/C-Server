
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CLIENTS 100 // Max clients
#define PORT 8080 // Port number
#define BUFFER_SIZE 5000 // Buffer size

int client_sockets[MAX_CLIENTS]; // Client sockets
int client_count = 0; // Client count
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // Mutex lock

void printClients() {
  printf("Clients: [ ");
  for (int i = 0; i < MAX_CLIENTS; i++) { // Loop through clients
    if (client_sockets[i] != -1) { // Check if client socket is not -1
      printf("%d ", client_sockets[i]); // Print client socket
     }
  }
  printf("]\n");
}

void addClient(int client) {
  pthread_mutex_lock(&lock); // Lock mutex
  for (int i = 0; i < MAX_CLIENTS; i++) { // Loop through clients
    if (client_sockets[i] == -1) { // Check if client socket is -1
      client_sockets[i] = client; // Set client socket
      client_count++; // Increment client count
      break; // Break loop
    }
  }
  pthread_mutex_unlock(&lock); // Unlock mutex
}

void removeClient(int client) { // Remove client
  pthread_mutex_lock(&lock); // Lock mutex
  for (int i = 0; i < MAX_CLIENTS; i++) { // Loop through clients
    if (client_sockets[i] == client) { // Check if client socket is equal to client
      client_sockets[i] = -1; // Set client socket to -1
      break; // Break loopk
    }
  }
  client_count--; // Decrement client count
  pthread_mutex_unlock(&lock); // Unlock mutex
}

void sendToAllClients(char *message, int socket) { // Send message to all clients
  for (int i = 0; i < MAX_CLIENTS; i++) { // Loop through clients
    if(client_sockets[i] != -1 && client_sockets[i] != socket) { // Check if client socket is not -1 and not equal to socket
      write(client_sockets[i], message, strlen(message)); // Write message to client socket
    }
  }
}

void *socketListener(void *socket_desc) {
  int sock = *(int*)socket_desc; // Get socket descriptor
  char buffer[BUFFER_SIZE] = {0}; // Initialize buffer

  while (1) {
    int readingData = read(sock, buffer, BUFFER_SIZE); // Read data from socket

    if (readingData < 0) { // Check if reading data failed
      removeClient(sock); // Remove client
      printf("Client disconnected\n"); // Print message
      printClients(); // Print clients
      break; // Break loop
    }

    printf("Received message: %s\n", buffer); // Print message
    sendToAllClients(buffer, sock); // Send message to all clients
    memset(buffer, 0, BUFFER_SIZE); // Clear buffer
  }

  close(sock); // Close socket
  pthread_exit(0); // Exit thread
}


int main() {
  int sockfd, new_socket; // Socket file descriptors
  struct sockaddr_in serv_addr; // Server address struct

  for (int i = 0; i < MAX_CLIENTS; i++) { // Loop through clients
    client_sockets[i] = -1; // Initialize client sockets to -1
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0); // Create socket
  if (sockfd == 0) { // Check if socket creation failed
    perror("socket failed"); // Print error message
    exit(1); // Exit on error
  }

  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET; // Set address family
  serv_addr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any address
  serv_addr.sin_port = htons(PORT); // Convert port to network byte order

  if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { // Bind socket to address
    perror("bind failed"); // Print error message
    exit(1); // Exit on error
  }

  listen(sockfd, MAX_CLIENTS); // Listen for connections
  printf("Server listening on port %d\n", PORT);

  while (1) {
    printf("Waiting for connection...\n");
    if (client_count < MAX_CLIENTS) { // Check if max clients reached
      int addrlen = sizeof(serv_addr); // Get address length
      new_socket = accept(sockfd, (struct sockaddr *)&serv_addr, (socklen_t*)&addrlen); // Accept connection

      if (new_socket < 0) {
        perror("accept failed");
        continue; // Continue to next iteration
      }

      printf("Connection accepted\n");
      addClient(new_socket); // Add client to list
      printClients(); // Print clients

      pthread_t client_thread;
      int *new_socket_ptr = malloc(sizeof(int));
      *new_socket_ptr = new_socket;

      if(pthread_create(&client_thread, NULL, socketListener, (void*)new_socket_ptr) < 0) {
        perror("could not create thread");
        free(new_socket_ptr); // Free memory on fail
        removeClient(new_socket);
        close(new_socket);
        continue; // Continue to next iteration
      }

      // Detach thread to avoid memory leak because we don't need to join it
      pthread_detach(client_thread);

    } else {
      printf("Max clients reached\n");
    }
  }
  return 0;
}

