#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Initialisation de WinSock
#ifdef _WIN32
// WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#else
// GNU \ LINUX
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#define LISTENING_PORT 8081
#define PENDING_QUEUE_MAXLENGTH 1

int main(void)
{
#ifdef _WIN32
    WSADATA wsa;

    if (WSASTARTUP(MAKEWORD(2, 2)), &wsa != 0)
    {
        fprintf(stderr, "(SERVEUR) Echec  d'initialisation  de WinSock\n");
        exit(1);
    }
#endif

    // Création du socket
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);

    if (socketFD == -1)
    {
        fprintf(stderr, "(SERVEUR) Echec  d'initialisation  du Socket\n");
        exit(1);
    }

    // Configuration du socket
    struct sockaddr_in socketAdress;

    socketAdress.sin_family = AF_INET;
    socketAdress.sin_port = LISTENING_PORT;
    socketAdress.sin_addr.s_addr = INADDR_ANY;

    int socketAddressLength = sizeof(socketAdress);
    int bindReturnCode = bind(socketFD, (struct sockaddr *)&socketAdress, socketAddressLength);

    if (bindReturnCode == -1)
    {
        fprintf(stderr, "(SERVEUR) Echec de liaison pour le socket\n");
        exit(1);
    }

    // Attente de nouvelle connexion
    if (listen(socketFD, PENDING_QUEUE_MAXLENGTH) == -1)
    {
        fprintf(stderr, "(SERVEUR) Echec  de démarrage de l'écoute des connexions entrantes\n");
        exit(1);
    }

    puts("En attente  de nouvelles connexions ...");

    int connectedSocketFd = accept(socketFD, (struct sockaddr *)&socketAdress,
                                   (socklen_t *)&socketAddressLength);

    if (connectedSocketFd == -1)
    {
        fprintf(stderr, "(SERVEUR) Echec  d'etablissement de la connexion\n");
    }

#ifdef _WIN32
    WSACleanup()
#else
#endif
        return 0;
}
