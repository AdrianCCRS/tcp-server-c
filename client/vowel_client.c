
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[]){
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <server_ip> <server_port> <text>\n", argv[0]);
        fprintf(stderr, "Example: %s 127.0.0.1 5050 hello\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    char *text = argv[3];

    // Validate that text contains only alphabet characters
    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
            fprintf(stderr, "Error: Text must contain only alphabet characters (a-z, A-Z)\n");
            exit(EXIT_FAILURE);
        }
    }

    int sock;
    struct sockaddr_in server_addr;
    char buffer[1024];

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if(inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address or port or address not supported");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Send message with newline (server expects newline-terminated messages)
    char message[1024];
    snprintf(message, sizeof(message), "%s\n", text);
    
    printf("Enviando mensaje: '%s'\n", text);
    send(sock, message, strlen(message), 0);
    
    printf("Respuesta del servidor:\n");

    // Read server response
    int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';  // Null terminate
        printf("%s", buffer);
    } else if (bytes_received < 0) {
        perror("recv");
    } else {
        printf("Servidor cerró la conexión\n");
    }

    close(sock);
    return 0;
}