#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define LENGTH 2048 // Taille maximale des messages
#define PORT 1023   // Port utilisé pour la connexion

volatile sig_atomic_t flag = 0; // Indicateur pour quitter le programme
int sockfd = 0;                // Descripteur de socket
char name[32];                 // Nom de l'utilisateur

// Fonction pour afficher le prompt de l'utilisateur
void str_overwrite_stdout()
{
    printf("%s", "Vous> ");
    fflush(stdout);
}

// Fonction pour supprimer le caractère de fin de ligne d'une chaîne
void str_trim_lf(char *arr, int length)
{
    int i;
    for (i = 0; i < length; i++)
    {
        if (arr[i] == '\n')
        {
            arr[i] = '\0';
            break;
        }
    }
}

// Gestion du signal SIGINT (CTRL+C) pour quitter proprement
void catch_ctrl_c_and_exit(int sig)
{
    flag = 1;
}

// Fonction pour obtenir l'heure actuelle sous forme de chaîne
char *get_current_time(char *buffer)
{
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, 80, "%H:%M", timeinfo); // Format de l'heure (HH:MM)
    return buffer;
}

// Thread pour envoyer des messages au serveur
void send_msg_handler()
{
    char message[LENGTH] = {};
    char buffer[LENGTH + 32] = {};
    char time_buffer[80];

    while (1)
    {
        str_overwrite_stdout();
        fgets(message, LENGTH, stdin); // Lecture du message depuis l'entrée standard
        str_trim_lf(message, LENGTH);

        if (strcmp(message, "exit") == 0) // Commande pour quitter le chat
        {
            break;
        }
        else if (strcmp(message, "/history") == 0) // Demande de l'historique des messages
        {
            send(sockfd, message, strlen(message), 0);
        }
        else // Envoi d'un message standard
        {
            sprintf(buffer, "[%s] %s: %s\n", get_current_time(time_buffer), name, message);
            send(sockfd, buffer, strlen(buffer), 0);
        }

        bzero(message, LENGTH);
        bzero(buffer, LENGTH + 32);
    }
    catch_ctrl_c_and_exit(2); // Indique au programme de quitter
}

// Thread pour recevoir les messages du serveur
void recv_msg_handler()
{
    char message[LENGTH] = {};

    while (1)
    {
        int receive = recv(sockfd, message, LENGTH, 0);
        if (receive > 0) // Message reçu avec succès
        {
            printf("\033[2K\r"); // Efface la ligne en cours pour afficher le message
            printf("%s\n", message);
            str_overwrite_stdout();
        }
        else if (receive == 0) // Déconnexion du serveur
        {
            break;
        }
        memset(message, 0, sizeof(message));
    }
}

int main(int argc, char **argv)
{
    char *ip = "127.0.0.1"; // Adresse IP du serveur

    // Gestion de l'interruption SIGINT
    signal(SIGINT, catch_ctrl_c_and_exit);

    // Demande du nom de l'utilisateur
    printf("Please enter your name: ");
    fgets(name, 32, stdin);
    str_trim_lf(name, strlen(name));

    // Vérifie la longueur du nom
    if (strlen(name) > 32 || strlen(name) < 2)
    {
        printf("Name must be less than 30 and more than 2 characters.\n");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;

    // Configuration de la socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = PORT;

    // Connexion au serveur
    int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err == -1)
    {
        printf("ERROR: connect\n");
        return EXIT_FAILURE;
    }

    // Envoi du nom de l'utilisateur au serveur
    send(sockfd, name, 32, 0);

    printf("=== WELCOME TO THE CHATROOM ===\n");
    printf("-- Pour afficher l'historique des messages envoyer /history --\n");

    // Création du thread pour l'envoi des messages
    pthread_t send_msg_thread;
    if (pthread_create(&send_msg_thread, NULL, (void *)send_msg_handler, NULL) != 0)
    {
        printf("ERROR: pthread\n");
        return EXIT_FAILURE;
    }

    // Création du thread pour la réception des messages
    pthread_t recv_msg_thread;
    if (pthread_create(&recv_msg_thread, NULL, (void *)recv_msg_handler, NULL) != 0)
    {
        printf("ERROR: pthread\n");
        return EXIT_FAILURE;
    }

    // Boucle principale pour attendre la déconnexion
    while (1)
    {
        if (flag) // Quitter si le signal d'arrêt est activé
        {
            printf("\nBye\n");
            break;
        }
    }

    close(sockfd); // Fermeture de la socket

    return EXIT_SUCCESS;
}
