#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_CLIENTS 100      // Nombre maximum de clients
#define MAX_MESSAGES 100     // Taille maximale de l'historique des messages
#define BUFFER_SZ 2048       // Taille maximale des messages
#define PORT 1023            // Port utilisé par le serveur

// Variables globales
static _Atomic unsigned int cli_count = 0;  // Nombre de clients connectés
static int uid = 10;                        // ID unique pour chaque client

/* Structure représentant un client */
typedef struct {
    struct sockaddr_in address; // Adresse du client
    int sockfd;                 // Descripteur de socket
    int uid;                    // Identifiant unique
    char name[32];              // Nom du client
} client_t;

client_t *clients[MAX_CLIENTS];  // Liste des clients connectés

/* Structure pour l'historique des messages */
typedef struct {
    char messages[MAX_MESSAGES][BUFFER_SZ]; // Tableau contenant les messages
    int message_count;                     // Nombre de messages enregistrés
} message_history_t;

message_history_t message_history;

// Mutex pour synchronisation
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t history_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Fonction pour afficher le prompt */
void str_overwrite_stdout() {
    printf("\r%s", "> ");
    fflush(stdout);
}

/* Supprime le caractère de fin de ligne d'une chaîne */
void str_trim_lf(char *arr, int length) {
    for (int i = 0; i < length; i++) {
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }
}

/* Affiche l'adresse IP d'un client */
void print_client_addr(struct sockaddr_in addr) {
    printf("%d.%d.%d.%d",
           addr.sin_addr.s_addr & 0xff,
           (addr.sin_addr.s_addr & 0xff00) >> 8,
           (addr.sin_addr.s_addr & 0xff0000) >> 16,
           (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

/* Ajoute un client à la liste des clients */
void queue_add(client_t *cl) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!clients[i]) {
            clients[i] = cl;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

/* Supprime un client de la liste en fonction de son UID */
void queue_remove(int uid) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] && clients[i]->uid == uid) {
            clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

/* Ajoute un message à l'historique (avec gestion du débordement) */
void add_message_to_history(char *message) {
    pthread_mutex_lock(&history_mutex);
    if (message_history.message_count < MAX_MESSAGES) {
        strncpy(message_history.messages[message_history.message_count], message, BUFFER_SZ);
        message_history.message_count++;
    } else {
        for (int i = 1; i < MAX_MESSAGES; i++) {
            strncpy(message_history.messages[i - 1], message_history.messages[i], BUFFER_SZ);
        }
        strncpy(message_history.messages[MAX_MESSAGES - 1], message, BUFFER_SZ);
    }
    pthread_mutex_unlock(&history_mutex);
}

/* Envoie un message à tous les clients sauf l'expéditeur */
void send_message(char *s, int uid) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] && clients[i]->uid != uid) {
            if (write(clients[i]->sockfd, s, strlen(s)) < 0) {
                perror("ERROR: write to descriptor failed");
                break;
            }
        }
    }
    add_message_to_history(s);
    pthread_mutex_unlock(&clients_mutex);
}

/* Envoie l'historique des messages à un client */
void send_message_history(int sockfd) {
    pthread_mutex_lock(&history_mutex);
    for (int i = 0; i < message_history.message_count; i++) {
        if (write(sockfd, message_history.messages[i], strlen(message_history.messages[i])) < 0) {
            perror("ERROR: Unable to send message history");
            break;
        }
        if (write(sockfd, "\n", 1) < 0) {
            perror("ERROR: Unable to send newline");
            break;
        }
    }
    pthread_mutex_unlock(&history_mutex);
}

/* Récupère l'heure actuelle sous forme de chaîne */
char *get_current_time(char *buffer) {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, 80, "%H:%M", timeinfo);
    return buffer;
}

/* Thread pour gérer un client */
void *handle_client(void *arg) {
    char buff_out[BUFFER_SZ];
    char name[32];
    int leave_flag = 0;
    char time_buffer[80];

    cli_count++;
    client_t *cli = (client_t *)arg;

    // Récupère le nom du client
    if (recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) < 2 || strlen(name) >= 32 - 1) {
        printf("Didn't enter the name.\n");
        leave_flag = 1;
    } else {
        strcpy(cli->name, name);
        sprintf(buff_out, "[%s] %s has joined", get_current_time(time_buffer), cli->name);
        printf("%s\n", buff_out);
        send_message(buff_out, cli->uid);
    }

    while (!leave_flag) {
        int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
        if (receive > 0 && strlen(buff_out) > 0) {
            str_trim_lf(buff_out, strlen(buff_out));
            if (strcmp(buff_out, "/history") == 0) 
                send_message_history(cli->sockfd); // Commande spéciale pour afficher l'historique
            else {
                send_message(buff_out, cli->uid);
                printf("%s\n", buff_out);
            }
        } else leave_flag = 1;
        bzero(buff_out, BUFFER_SZ);
    }

    // Fermeture de la connexion client
    close(cli->sockfd);
    queue_remove(cli->uid);
    free(cli);
    cli_count--;
    pthread_detach(pthread_self());
    return NULL;
}

/* Fonction principale */
int main(int argc, char **argv) {
    char *ip = "127.0.0.1"; // Adresse IP du serveur
    int option = 1;
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    pthread_t tid;

    /* Configuration de la socket */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = PORT;

    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&option, sizeof(option)) < 0) {
        perror("ERREUR : Échec de la configuration des options de socket");
        return EXIT_FAILURE;
    }

    /* Liaison de la socket */
    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR: Socket binding failed");
        return EXIT_FAILURE;
    }

    /* Mise en écoute */
    if (listen(listenfd, 10) < 0) {
        perror("ERROR: Socket listening failed");
        return EXIT_FAILURE;
    }

    printf("=== SERVEUR CHATROOM ===\n");

    while (1) {
        socklen_t clilen = sizeof(cli_addr);
        connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &clilen);

        // Vérifie si le nombre maximum de clients est atteint
        if ((cli_count + 1) == MAX_CLIENTS) {
            printf("Max clients reached. Rejected: ");
            print_client_addr(cli_addr);
            printf(":%d\n", cli_addr.sin_port);
            close(connfd);
            continue;
        }

        /* Crée une structure pour le nouveau client */
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->address = cli_addr;
        cli->sockfd = connfd;
        cli->uid = uid++;

        /* Ajoute le client à la liste et crée un thread */
        queue_add(cli);
        pthread_create(&tid, NULL, &handle_client, (void *)cli);

        /* Réduit l'utilisation CPU */
        sleep(1);
    }

    return EXIT_SUCCESS;
}
