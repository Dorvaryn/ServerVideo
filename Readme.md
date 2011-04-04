ServerVideo
=============

Un serveur video streaming en C utilisant epoll

Auteurs
-------
* Benjamin Augustin <benjamin@odai.homelinux.net>
* Thibaut Patel <thibaut.patel@gmail.com>

Manuel utilisateur
------------------

### Systèmes d'exploitation compatibles
GNU/Linux, compatible avec les version noyau antérieures a la 2.6.28, toutefois pour de meilleures performance un système avec un noyau supérieur a 2.6.28 est recommandé

### Compilation
make : permet de compiler le programme, la compatibilité noyaux < 2.6.28 se fait automatiquement si un tel système est détecté
make clean : permet de purger les .o et l'exécutable

### Format du catalogue
dummy

### Lancement du serveur et commandes
Le serveur comprend quelques commandes de base
exit / quit / q : quitter
