// this file is the client side of the chat application
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SZ 2048
#define NAME_LEN 32


void leaveRoom(int sockfd){
    char message[BUFFER_SZ] = {};
    char buffer[BUFFER_SZ + NAME_LEN] = {};

    sprintf(buffer, "%s has left the chat\n", NAME_LEN);
    send(sockfd, buffer, strlen(buffer), 0);
    printf("You have left the chat.\n");
}

void sendMessage(int sockfd, char *name){
    char message[BUFFER_SZ] = {};
    char buffer[BUFFER_SZ + NAME_LEN] = {};

    while(1){
        fgets(message, BUFFER_SZ, stdin);
        message[strcspn(message, "\n")] = 0;

        if (strcmp(message, "exit") == 0){
            break;
        }
        sprintf(buffer, "%s: %s\n", name, message);
        send(sockfd, buffer, strlen(buffer), 0);
        bzero(message, BUFFER_SZ);
        bzero(buffer, BUFFER_SZ + NAME_LEN);
    }
    leaveRoom(2);
}

void receiveMessage(int sockfd){
    char message[BUFFER_SZ] = {};
    while(1){
        int receive = recv(sockfd, message, BUFFER_SZ, 0);
        if (receive > 0){
            printf("%s", message);
        } else if (receive == 0){
            break;
        }
        bzero(message, BUFFER_SZ);
    }
}

int main(int argc, char **argv) {
    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char $ip = "127.0.0.1";
    int port = atoi(argv[1]);

    printf("Please enter your name: ");
    char name[NAME_LEN];
    fgets(name, NAME_LEN, stdin);
    name[strlen(name) - 1] = '\0';


    if (strlen(name) > 32 || strlen(name) < 2){
        printf("Name must be less than 32 and more than 2 characters.\n");
        return EXIT_FAILURE;
    }





}