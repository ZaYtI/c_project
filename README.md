## Identification des éxigences fonctionnelles et éxigences non fonctionnelles

### Exigences Fonctionnelles

1. **Connexion Client-Serveur :**
   - Le serveur doit être capable de gérer plusieurs connexions simultanées de clients.
   - Chaque client doit pouvoir se connecter à une salle de chat centralisée via une adresse IP et un port spécifiés.

2. **Gestion des Messages :**
   - Les utilisateurs doivent pouvoir envoyer des messages texte à tous les autres utilisateurs connectés à la salle de chat.
   - Les messages envoyés par un client doivent être reçus et affichés par tous les autres clients en temps réel.

3. **Affichage en Temps Réel :**
   - Les clients doivent voir immédiatement les messages envoyés par eux-mêmes ainsi que par les autres utilisateurs dans l'interface de la salle de chat.

4. **Déconnexion des Clients :**
   - Un client doit pouvoir se déconnecter du serveur à tout moment, et les autres utilisateurs doivent être informés de cette déconnexion.

5. **Historique des Messages :**
   - La mémoire partagée doit stocker une liste des messages récents qui doivent être accessibles aux nouveaux clients rejoignant la salle de chat.

6. **Nom d'Utilisateur :**
   - Chaque client doit entrer un nom d'utilisateur lors de la connexion, qui sera affiché avec leurs messages.

### Exigences Non Fonctionnelles

1. **Concurrence et Gestion des Threads :**
   - Le serveur doit être capable de gérer plusieurs clients simultanément grâce à l'utilisation de threads.
   - Chaque connexion client doit être gérée dans un thread distinct.

2. **Synchronisation des Accès à la Mémoire Partagée :**
   - L'accès à la mémoire partagée (où sont stockés les messages) doit être correctement synchronisé à l’aide de mutex pour éviter les conditions de course (race conditions).
   - Aucun blocage ne doit se produire lors de l'accès concurrent à la mémoire partagée par plusieurs threads.

3. **Performance et Scalabilité :**
   - Le système doit être capable de supporter un nombre raisonnable de clients (par exemple, 10 à 20 utilisateurs) sans perte significative de performance.
   - La mémoire partagée doit être suffisamment optimisée pour éviter une consommation excessive des ressources.

4. **Robustesse et Tolérance aux Erreurs :**
   - Le serveur doit gérer les déconnexions inattendues des clients sans planter.
   - Le système doit prévenir les fuites de mémoire et gérer les erreurs comme la perte de connexion au réseau ou la saturation des ressources du serveur.

5. **Interface Utilisateur Simple :**
   - L'interface en ligne de commande doit être intuitive et réactive pour permettre une utilisation fluide par les utilisateurs.
   - Chaque client doit afficher correctement les messages reçus sans attendre que l'utilisateur envoie un message.

6. **Sécurité des Données :**
   - Les messages échangés entre les clients doivent être sécurisés contre les accès non autorisés, par exemple en limitant l'accès à la mémoire partagée aux threads autorisés.

7. **Compatibilité et Portabilité :**
   - L'application doit être portable sur différents systèmes Unix/Linux.
   - Le code doit être compatible avec les bibliothèques standard POSIX pour les threads (pthread) et la mémoire partagée.
