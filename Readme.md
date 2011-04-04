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
Ce server est spécifique au systèmes GNU/Linux, il est compatible avec les version noyau antérieures a la 2.6.28,
toutefois pour de meilleures performance un système avec un noyau supérieur a 2.6.28 est recommandé

### Compilation
* make : permet de compiler le programme, la compatibilité noyaux < 2.6.28 se fait automatiquement si un tel système est détecté
* make clean : permet de purger les .o et l'exécutable

### Format du catalogue
Le catalogue est un fichier texte utilisant les marques de fin de ligne CRLF,
les deux premières ligne indiquent l'adresse du serveur et le port sur lequel on peut demander le catalogue. Ensuite chacune des lignes est le nom d'un fichier flux situé dans le répertoire data.
Pour ajouter un flux il suffit de créer son fichier flux dans le répertoire data et d'ajouter le nom du fichier dans le catalogue.
####Un fichier flux se décompose ainsi
1. ID=0 //Id du flux
2. Name=NomDuFlux //Le nom du flux qui aparaitra dans le catalogue
3. Type=JPEG //Le type d'image (JPEG ou BMP)
4. Adress=192.168.1.44 //Adresse du serveur qui contient ce flux
5. Port=8081 //Le port sur lequel on peut demander ce flux
6. Protocol=TCP_PULL //Le protocole d'accès ace flux (TCP_PULL, TPC_PUSH, UDP_PULL, UDP_PUSH, MCAST_PUSH)
7. IPS=3 //Le nombre d'images par secondes du flux
8. Images/Numbers/img1.jpeg //Première image du flux
9. Images/Numbers/img2.jpeg // ...
10. ...

### Lancement du serveur et commandes
Le serveur comprend quelques commandes de base: 
* exit / quit / q : quitter
