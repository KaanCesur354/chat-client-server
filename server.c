#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>


#define MAX_CLIENTS 12
#define MAX_ROOMS 10

// Variables for server and client sockets
int sockfd, newsockfd, portno, n;
char buffer[1024];
struct sockaddr_in serv_addr;

// Structure to represent a client
struct client_t {
    int sockfd;                    
    struct sockaddr_in address;    
    char nickname[256];            
    int user_id;                   
};

struct client_t clients[MAX_CLIENTS]; // Array to hold all connected clients

// Structure to represent a chat room
struct room_t {
    char name[256];                   // Name of the room
    int room_id;                      // Unique ID of the room
    struct client_t *clients[MAX_CLIENTS]; // List of clients in the room
    int client_count;                 // Current number of clients in the room
};

struct room_t rooms[MAX_ROOMS]; // Array to hold all chat rooms
int room_count = 0;             // Counter for the number of created rooms

// Utility function to print a message to the console
void print_message(const char *message) {
    printf("%s\n", message);
}

// Function to handle errors and terminate the program
void error(char *msg) {
    perror(msg);
    exit(1);
}

// Function to send a message to a specific client
void SendMessageToClient(int sockfd, const char *message) {
    int msg = send(sockfd, message, strlen(message), 0);
    if (msg == -1) {
        error("Sending error occurred");
    }
}

// Function to broadcast a message to all connected clients, excluding the sender
void send_message_to_all_clients(const char *message, int exclude_uid) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].user_id != -1 && clients[i].user_id != exclude_uid) {
            SendMessageToClient(clients[i].sockfd, message);
        }
    }
}

// Function to send a message to all clients in a specific room
void send_message_to_room(int sender_index, const char *message, const char *room_name) {

    for (int i = 0; i < room_count; ++i) {

        if (strcmp(rooms[i].name, room_name) == 0) { // Check if the room exists

            for (int j = 0; j < rooms[i].client_count; ++j) {
                // Avoid sending the message back to the sender
                if (rooms[i].clients[j]->user_id != clients[sender_index].user_id) {
                    printf("%s MESSAGE/ ", room_name);
                    char formatted_message[1024];
                    sprintf(formatted_message, "From Room %s: %s", clients[sender_index].nickname, message);
                    SendMessageToClient(rooms[i].clients[j]->sockfd, formatted_message);
                }
            }
            return;
        }
    }
    // Notify the sender if the room was not found
    SendMessageToClient(clients[sender_index].sockfd, "Room not found!");
}


// Function to add a client to a room
void join_room(int client_index, const char *room_name) {

    for (int i = 0; i < room_count; ++i) {
        if (strcmp(rooms[i].name, room_name) == 0) { // Check if the room exists
            // Check if the client is already in the room
            for (int j = 0; j < rooms[i].client_count; ++j) {
                if (rooms[i].clients[j]->user_id == clients[client_index].user_id) {
                    // Notify the client if they are already in the room
                    SendMessageToClient(clients[client_index].sockfd, "You are already in this room!");
                    return;
                }
            }

            // Check if the room is full
            if (rooms[i].client_count >= MAX_CLIENTS) {
                SendMessageToClient(clients[client_index].sockfd, "Room is full!");
                return;
            }

            // Add the client to the room
            rooms[i].clients[rooms[i].client_count++] = &clients[client_index];
            printf("'%s' has joined '%s'.\n", clients[client_index].nickname, room_name);
            SendMessageToClient(clients[client_index].sockfd, "Welcome to the room!");
            return;
        }
    }
    // Notify the client if the room does not exist
    SendMessageToClient(clients[client_index].sockfd, "Room not found!");
}


// Function to create a new room
void create_room(int client_index, const char *room_name) {
    if (room_count >= MAX_ROOMS) {
        printf("Maximum room number has been reached!\n");
        return;
    }
    for (int i = 0; i < room_count; ++i) {
        if (strcmp(rooms[i].name, room_name) == 0) {
            printf("This room already exists: %s\n", room_name);
            SendMessageToClient(clients[client_index].sockfd, "This room already exists!");
            return;
        }
    }
    struct room_t *room = &rooms[room_count];
    strcpy(room->name, room_name);
    room->room_id = room_count;
    room->client_count = 0;
    room_count++;
    printf("Room '%s' has been created. (ID: %d)\n", room_name, room->room_id);
    join_room(client_index, room_name); // Add the creator to the room

}

// Function to remove a client from a room
void leave_room(int client_index, const char *room_name) {

    for (int i = 0; i < room_count; ++i) {

        if (strcmp(rooms[i].name, room_name) == 0) {

            for (int j = 0; j < rooms[i].client_count; ++j) {

                if (rooms[i].clients[j]->user_id == clients[client_index].user_id) {

                    // Remove the client from the room
                    for (int k = j; k < rooms[i].client_count - 1; ++k) {
                        rooms[i].clients[k] = rooms[i].clients[k + 1];
                    }
                    rooms[i].client_count--;
                    char leave_message[1024];
                    sprintf(leave_message, "%s left the room: %s", clients[client_index].nickname, room_name);
                    send_message_to_room(client_index, leave_message, room_name);
                    printf("'%s' left '%s'.\n", clients[client_index].nickname, room_name);
                    return;
                }
            }
            // Notify the client if they are not part of the room
            SendMessageToClient(clients[client_index].sockfd, "You are not in the room!");
            return;
        }
    }
    // Notify the client if the room does not exist
    SendMessageToClient(clients[client_index].sockfd, "Room not found!");
}

// Function to handle a client joining the server
void handle_client_join(int client_index) {

    char join_message[1024];
    printf("JOIN/ ");
    sprintf(join_message, "%s joined", clients[client_index].nickname);
    send_message_to_all_clients(join_message, clients[client_index].user_id);
    print_message(join_message);
}

// Function to handle a client disconnecting
void handle_client_exit(int client_index) {

    char exit_message[1024];
    printf("EXIT/ ");
    sprintf(exit_message, "%s exited", clients[client_index].nickname);
    send_message_to_all_clients(exit_message, clients[client_index].user_id);
    print_message(exit_message);

    clients[client_index].user_id = -1; // Mark the client as inactive
    close(clients[client_index].sockfd);
}

// Function to handle messages received from a client
void handle_client_message(int sender_index, const char *message) {

    // Handle direct private messages
    if (message[0] == '@') {

        char temp[1024];
        strcpy(temp, message);
        char *receiver_name = strtok(temp + 1, " ");
        for (int i = 0; i < MAX_CLIENTS; ++i) {

            if (clients[i].user_id != -1 && strcmp(clients[i].nickname, receiver_name) == 0) {

                char private_message[1024];
                printf("PRIVATE MESSAGE/ from @%s: ", clients[sender_index].nickname);
                print_message(message);
                sprintf(private_message, "(private) @%s: %s", clients[sender_index].nickname, message);
                SendMessageToClient(clients[i].sockfd, private_message);
                return;
            }
        }
        // Notify sender if the recipient is not found
        SendMessageToClient(clients[sender_index].sockfd, "User not found");
        printf("ERROR/ User not found when @%s sending a private message\n", clients[sender_index].nickname);
    } 
    else if (strncmp(message, "/room ", 6) == 0) {

        // Handle messages directed to a specific room
        char *room_name = strtok((char *)(message + 6), " ");
        char *msg = strtok(NULL, "\0");
        if (room_name && msg) {
            send_message_to_room(sender_index, msg, room_name);
        } else {
            SendMessageToClient(clients[sender_index].sockfd, "Invalid message format!");
        }
    } else if (strncmp(message, "/create ", 8) == 0) {

        // Handle room creation
        create_room(sender_index, message + 8);
    } else if (strncmp(message, "/join ", 6) == 0) {

        // Handle joining a room
        join_room(sender_index, message + 6);
    } else if (strncmp(message, "/leave ", 7) == 0) {

        // Handle leaving a room
        leave_room(sender_index, message + 7);
    } else {

        // Broadcast general messages to all clients
        char general_message[1024];
        printf("MESSAGE/ ");
        sprintf(general_message, "@%s: %s", clients[sender_index].nickname, message);
        send_message_to_all_clients(general_message, clients[sender_index].user_id);
        print_message(general_message);
    }
}

// Function to handle communication with a client in a separate thread

void *handle_client(void *arg) {
    int client_index = *((int *)arg);
    free(arg);
    char buffer[1024];
    while (1) {
        int bytes_received = recv(clients[client_index].sockfd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {

            // Handle client disconnection
            handle_client_exit(client_index);
            pthread_exit(NULL);
        }

        buffer[bytes_received] = '\0'; // Null-terminate the received message
        handle_client_message(client_index, buffer); // Process the message
    }
}

// Driver function to start the server
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Port not provided. Program terminated\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // Create a socket
    if (sockfd < 0) {
        error("Error opening socket.");
    }
    bzero((char *)&serv_addr, sizeof(serv_addr)); // Initialize the server address
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        error("Binding failed.");
    }

    listen(sockfd, 5); // Start listening for incoming connections
    print_message("Server started. Waiting for connections...");

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        clients[i].user_id = -1; // Mark all clients as inactive initially
    }

    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);
        int client_socket = accept(sockfd, (struct sockaddr *)&client_address, &client_address_len);
        if (client_socket == -1) {
            perror("Error accepting connection");
            continue;
        }

        int uid = -1;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i].user_id == -1) { // Find an available slot for the client
                uid = i;
                break;
            }
        }

        if (uid == -1) {
            close(client_socket); // Close the socket if the server is full
            continue;
        }
        clients[uid].user_id = uid;
        clients[uid].sockfd = client_socket;
        clients[uid].address = client_address;

        // Receive the client's nickname
        char client_name[1024];
        int bytes_received = recv(client_socket, client_name, sizeof(client_name), 0);
        if (bytes_received <= 0) {
            close(client_socket); // Close the connection if no name is received
            continue;
        }

        client_name[bytes_received] = '\0'; // Null-terminate the name
        strcpy(clients[uid].nickname, client_name); // Store the client's nickname

        // Send the client their unique ID
        char uid_message[1024];
        sprintf(uid_message, "%d", uid);
        SendMessageToClient(client_socket, uid_message);

        // Create a new thread to handle the client
        pthread_t thread;
        int *client_index = (int *)malloc(sizeof(int));
        *client_index = uid;

        if (pthread_create(&thread, NULL, handle_client, (void *)client_index) != 0) {

            perror("Error creating thread");
            close(client_socket);
            continue;
        }
        pthread_detach(thread); // Detach the thread to allow it to run independently
    }

    close(newsockfd); // Close the new socket
    close(sockfd);    // Close the main socket
    return 0;
}