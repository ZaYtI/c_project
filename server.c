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

#define CLIENT_LIMIT 100
#define MESSAGE_SIZE 2048
#define MESSAGE_HISTORY_LIMIT 100

static _Atomic unsigned int active_clients = 0;
static int identifier = 10;

// Structure pour un message
typedef struct
{
    char content[MESSAGE_SIZE];
    char username[32];
} message_t;

message_t message_history[MESSAGE_HISTORY_LIMIT];
int message_count = 0;

// Structure pour un client
typedef struct
{
    struct sockaddr_in addr_info;
    int socket_fd;
    int id;
    char username[32];
} client_info;

client_info *client_array[CLIENT_LIMIT];

pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t message_mutex = PTHREAD_MUTEX_INITIALIZER;

// Afficher le prompt
void display_prompt()
{
    printf("\r%s", "> ");
    fflush(stdout);
}

// Enlever la nouvelle ligne
void remove_newline(char *str, int length)
{
    for (int i = 0; i < length; i++)
    {
        if (str[i] == '\n')
        {
            str[i] = '\0';
            break;
        }
    }
}

// Afficher l'adresse du client
void show_client_address(struct sockaddr_in addr)
{
    printf(
        "%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

// Ajouter un client à la queue
void add_to_queue(client_info *client)
{
    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < CLIENT_LIMIT; ++i)
    {
        if (!client_array[i])
        {
            client_array[i] = client;
            break;
        }
    }
    pthread_mutex_unlock(&client_mutex);
}

// Retirer un client de la queue
void remove_from_queue(int id)
{
    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < CLIENT_LIMIT; ++i)
    {
        if (client_array[i])
        {
            if (client_array[i]->id == id)
            {
                client_array[i] = NULL;
                break;
            }
        }
    }
    pthread_mutex_unlock(&client_mutex);
}

// Afficher la liste des messages
void broadcast_message(char *msg, int id)
{
    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < CLIENT_LIMIT; ++i)
    {
        if (client_array[i])
        {
            if (client_array[i]->id != id)
            {
                if (write(client_array[i]->socket_fd, msg, strlen(msg)) < 0)
                {
                    perror("ERREUR : Échec de l'écriture dans le descripteur");
                    break;
                }
            }
        }
    }
    pthread_mutex_unlock(&client_mutex);
}

// Ajouter un message à l'historique
void add_message_to_history(const char *content, const char *username)
{
    pthread_mutex_lock(&message_mutex);
    if (message_count < MESSAGE_HISTORY_LIMIT)
    {
        strcpy(message_history[message_count].content, content);
        strcpy(message_history[message_count].username, username);
        message_count++;
    }
    else
    {
        for (int i = 1; i < MESSAGE_HISTORY_LIMIT; i++)
        {
            message_history[i - 1] = message_history[i];
        }
        strcpy(message_history[MESSAGE_HISTORY_LIMIT - 1].content, content);
        strcpy(message_history[MESSAGE_HISTORY_LIMIT - 1].username, username);
    }
    pthread_mutex_unlock(&message_mutex);
}

// Afficher l'historique des messages pour un client
void send_message_history(int client_socket)
{
    pthread_mutex_lock(&message_mutex);
    for (int i = 0; i < message_count; i++)
    {
        char buffer[MESSAGE_SIZE];
        snprintf(buffer, sizeof(buffer), "%s: %s\n", message_history[i].username, message_history[i].content);
        write(client_socket, buffer, strlen(buffer));
    }
    pthread_mutex_unlock(&message_mutex);
}

// Méthode qui tourne pour le thread client
void *client_handler(void *arg)
{
    char buffer[MESSAGE_SIZE];
    char user[32];
    int exit_flag = 0;

    active_clients++;
    client_info *client = (client_info *)arg;

    if (recv(client->socket_fd, user, 32, 0) <= 0 || strlen(user) < 2 || strlen(user) >= 32 - 1)
    {
        printf("Nom d'utilisateur invalide.\n");
        exit_flag = 1;
    }
    else
    {
        strcpy(client->username, user);
        sprintf(buffer, "%s s'est connecté\n", client->username);
        printf("%s", buffer);
        broadcast_message(buffer, client->id);

        send_message_history(client->socket_fd);
    }

    bzero(buffer, MESSAGE_SIZE);

    while (1)
    {
        if (exit_flag)
        {
            break;
        }

        int received = recv(client->socket_fd, buffer, MESSAGE_SIZE, 0);
        if (received > 0)
        {
            if (strlen(buffer) > 0)
            {
                broadcast_message(buffer, client->id);
                add_message_to_history(buffer, client->username);
                remove_newline(buffer, strlen(buffer));
                printf("%s -> %s\n", buffer, client->username);
            }
        }
        else if (received == 0 || strcmp(buffer, "exit") == 0)
        {
            sprintf(buffer, "%s s'est déconnecté\n", client->username);
            printf("%s", buffer);
            broadcast_message(buffer, client->id);
            exit_flag = 1;
        }
        else
        {
            printf("ERREUR : Problème de connexion\n");
            exit_flag = 1;
        }

        bzero(buffer, MESSAGE_SIZE);
    }

    close(client->socket_fd);
    remove_from_queue(client->id);
    free(client);
    active_clients--;
    pthread_detach(pthread_self());

    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *address = "127.0.0.1";
    int port_num = atoi(argv[1]);
    int option = 1;
    int listening_socket = 0, client_socket = 0;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    pthread_t thread_id;

    listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listening_socket < 0) {
        perror("ERREUR : Échec de la création de la socket");
        return EXIT_FAILURE;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);
    server_addr.sin_port = htons(port_num);

    signal(SIGPIPE, SIG_IGN);

    if (setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0)
    {
        perror("ERREUR : Échec de la configuration des options de socket");
        return EXIT_FAILURE;
    }

    if (bind(listening_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("ERREUR : Échec de la liaison de la socket");
        return EXIT_FAILURE;
    }

    if (listen(listening_socket, 10) < 0)
    {
        perror("ERREUR : Échec de l'écoute");
        return EXIT_FAILURE;
    }

    printf("Serveur en attente de connexions...\n");

    while (1)
    {
        socklen_t addr_len = sizeof(client_addr);
        client_socket = accept(listening_socket, (struct sockaddr *)&client_addr, &addr_len);

        if (client_socket < 0) {
            perror("ERREUR : Échec de l'acceptation de la connexion");
            continue;
        }

        if ((active_clients + 1) == CLIENT_LIMIT)
        {
            printf("Limite de clients atteinte. Rejeté : ");
            show_client_address(client_addr);
            printf(":%d\n", client_addr.sin_port);
            close(client_socket);
            continue;
        }

        client_info *new_client = (client_info *)malloc(sizeof(client_info));
        new_client->addr_info = client_addr;
        new_client->socket_fd = client_socket;
        new_client->id = identifier++;

        add_to_queue(new_client);
        pthread_create(&thread_id, NULL, &client_handler, (void *)new_client);

        sleep(1);
    }

    return EXIT_SUCCESS;
}
