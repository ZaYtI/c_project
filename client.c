#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUFFER_SZ 2048
#define NAME_LEN 32

int sockfd;
struct sockaddr_in server_addr;
char buffer[BUFFER_SZ];

void leaveRoom(int sockfd) {
    char buffer[BUFFER_SZ + NAME_LEN] = {};

    sprintf(buffer, "User has left the chat\n");
    send(sockfd, buffer, strlen(buffer), 0);
    printf("You have left the chat.\n");
}

void sendMessage(int sockfd, char *name) {
    char message[BUFFER_SZ] = {};
    char buffer[BUFFER_SZ + NAME_LEN] = {};

    while(1) {
        fgets(message, BUFFER_SZ, stdin);
        message[strcspn(message, "\n")] = 0;

        if (strcmp(message, "exit") == 0) {
            break;
        }

        sprintf(buffer, "%s: %s\n", name, message);
        send(sockfd, buffer, strlen(buffer), 0);
        bzero(message, BUFFER_SZ);
        bzero(buffer, BUFFER_SZ + NAME_LEN);
    }
    leaveRoom(sockfd);
    close(sockfd); // Fermer la socket après avoir quitté la salle
    exit(0); // Quitter l'application proprement
}

void *receiveMessage(void *sockfd) {
    char message[BUFFER_SZ] = {};
    int sock = *((int *)sockfd);

    while(1) {
        int receive = recv(sock, message, BUFFER_SZ, 0);
        if (receive > 0) {
            printf("%s", message);
        } else if (receive == 0) {
            break; // La connexion a été fermée
        }
        bzero(message, BUFFER_SZ);
    }

    return NULL;
}

int main( char **argv) {

    pthread_t t_send, t_receive;
    char *ip = "127.0.0.1";  // Correction : Utilisation correcte de l'adresse IP en C
    int port = atoi(argv[1]);

    printf("Please enter your name: ");
    char name[NAME_LEN];
    fgets(name, NAME_LEN, stdin);
    name[strcspn(name, "\n")] = 0;  // Supprimer le saut de ligne

    if (strlen(name) > 32 || strlen(name) < 2) {
        printf("Name must be less than 32 and more than 2 characters.\n");
        return EXIT_FAILURE;
    }

    // Création de la socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erreur lors de la création de la socket");
        exit(1);
    }

    // Configuration du serveur
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    // Connexion au serveur
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erreur lors de la connexion");
        exit(1);
    }

    printf("Connecté au serveur\n");

    if (pthread_create(&t_receive, NULL, receiveMessage, (void*)&sockfd) != 0) {
        perror("Erreur lors de la création du thread de réception");
        exit(1);
    }

    sendMessage(sockfd, name);

    pthread_join(t_receive, NULL);

    close(sockfd);
    return 0;
}
