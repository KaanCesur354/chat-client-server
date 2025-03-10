#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

char name[256];
int sockfd;

void *receive_messages(void *arg) {
char buffer[1024];

    while (1) {
        int bytes_received = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            printf("Server connection closed.\n");
            exit(EXIT_SUCCESS);
            break;
        }

        buffer[bytes_received] = '\0';
        if (strcmp(buffer, "You are kicked") == 0)
        {
            printf("%s\n", buffer);
            close(sockfd);
            exit(EXIT_SUCCESS);
            break;
        }
        
        printf("> ");
        printf("\33[2K");
        printf("%s\n", buffer);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <ip_address> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *ip_address = argv[1];
    int port = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(ip_address);
    server_address.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Error connecting to server");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Get the client name
    printf("Enter your name: ");
    fgets(name, 256, stdin);
    name[strlen(name) - 1] = '\0'; // Remove the newline character

    send(sockfd, name, strlen(name), 0);

    pthread_t thread;
    if (pthread_create(&thread, NULL, receive_messages, NULL) != 0) {
        perror("Error creating thread");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("\n---------------------------\n");
    printf("+ Public Message: <message>\n");
    printf("+ Private Message: @<user_name> <message>\n");
    printf("+ Create room: /create <roomname> \n");
    printf("+ Join room: /join <roomname> \n");
    printf("+ Leave room: /leave <roomname> \n");
    printf("+ Message room: /room <roomname> <message> \n");
    printf("+ Exit the Chat: exit \n");
    printf("---------------------------\n");

    while (1) {

        char message[1024];
        // printf("> ");
        fgets(message, 1024, stdin);
        message[strlen(message) - 1] = '\0'; // Remove the newline character

        if (strcmp(message, "exit") == 0) {
            printf("Exiting...\n");
            close(sockfd);
            exit(EXIT_SUCCESS);
        }

        send(sockfd, message, strlen(message), 0);
    }

    close(sockfd);
    return 0;
}