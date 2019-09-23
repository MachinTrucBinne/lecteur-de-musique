2019-09-21

		♫ MUSIQUE ♫

######################################################################
######################################################################

		DESCRIPTION

C'est un lecteur de musique.

Le but est de garder ça simple, sans détours, sans rien pour distraire, sans images.
L'interface est inspirée de celle des vieux iPod mini, iPod classique, etc., où :
• on est dans une liste d'artistes,
• on clique sur un artiste
• on est alors dans une liste d'albums
• on clique sur un album
• on est alors dans une liste de morceaux
• on clique sur un morceau
• le morceau joue
• les morceaux suivants jouent jusqu'à la fin de l'album.

Fonctionne aussi en plein écran.

Ceci dit, l'application est gourmande en énergie.

Codé en juin 2017.
Déposé sur GitHub en septembre 2019.

######################################################################
######################################################################

		COMMANDES

• Arrière : remonter soit du dossier des tounes au dossier des albums, soit du dossier des albums au dossier des artistes
• Entrer : entrer dans le dossier, jouer la toune sélectionnée
• Esc : quitter
• Espace : jouer / pause

• ABC...Z : va à l'artiste qui commence par la lettre

• CMD+L : aller au dossier de l'album où la toune joue (comme dans iTunes)
• CMD+Q : quitter

• Gauche : recommence la toune, ou joue la toune qui précède
• Droite : joue la toune suivante
• Haut/Bas : pour se déplacer dans les dossiers

• Cliquer : entrer dans le dossier, jouer la toune sélectionnée
• Scroller : scroller

######################################################################
######################################################################

		PRÉREQUIS

Il y a deux versions :
• Une version exécutable
• Une version application

Le code est pour macOS, mais ça devrait pouvoir se porter à d'autres OS assez aisément.

Le code est en C++.
Il utilise SDL2 pour l'interface graphique.
Il utilise lavformat de ffmpeg pour lire les infos des tounes.
Il utilise afplay pour jouer la musique.

Après avoir installé Brew et les Command Line Tools, il faut installer plusieurs choses :
	brew install SDL2
	brew install SDL2_ttf
	brew install ffmpeg

######################################################################
######################################################################

		COMPILATION

Pour compiler la version exécutable :

g++ $(sdl2-config --cflags) -Wall -o main  main.cpp $(sdl2-config --libs) -lSDL2_ttf -lavformat

Ça devrait marcher.

Pour compiler la version application il faut Xcode.
De mon côté ça marche pour Xcode 9.0 sur macOS Mojave 10.14.6.

######################################################################
######################################################################

		UNE FOIS COMPILÉ

POUR QUE ÇA MARCHE :

• L'exécutable marche bien, peu importe son directory mais il doit être dans le même dossier que le fichier "afplay.command". Aussi, si on ne fait que cliquer sur l'exécutable depuis le Finder pour le faire rouler, les artistes seront là mais la musique ne jouera pas. Pour que ça marche, il faut ouvrir un Terminal et aller dans le directory de l'exécutable et faire ./main de sorte qu'il trouve le fichier afplay.command.
• L'application elle devrait marcher peu importe son directory mais si elle ne marche pas, la mettre dans le directory /Applications. Le fichier "afplay.command" est inclus dans ses ressources donc on a pas à y penser.
• Il faut que la musique soit ordonnée selon la structure d'iTunes dans le dossier :
	~/Music/iTunes/iTunes\ Media/Music

BUGS :

• Si on laisse une toune sur "pause" pendant trop longtemps et qu'on refait "jouer", elle peut ne pas jouer à nouveau. Il faut alors commencer une autre toune.
• La fonction std::sort met les majuscules avant les minuscules, donc la liste d'artistes peut parfois être un peu pas complètement en ordre alphabétique.

AUSSI :

• La manière de faire jouer la musique est un peu maladroite. Le programme invoque un fichier "afplay.command" qui lui va invoquer afplay. Dans une ancienne version plus propre j'utilisais SDL2_mixer, mais il ne gère pas les mp3, alors qu'afplay oui. Aussi, le code pourrait appeler directement afplay sans passer par un fichier .command, mais alors il faudrait que le code continue de rouler sans attendre qu'afplay "termine" son processus.
• L'avantage de la version application est qu'il n'y a pas toujours un terminal d'ouvert en arrière plan.

ENFIN :

• La différence entre les deux fichiers .cpp de la version exécutable et de la version application est la modification de ces lignes :
1 :
< C'est un lecteur de musique, version application.
---
> C'est un lecteur de musique, version exécutable.
2 :
< #include <SDL2_ttf/SDL_ttf.h>
< //#include <SDL2/SDL_ttf.h>
---
> //#include <SDL2_ttf/SDL_ttf.h>
> #include <SDL2/SDL_ttf.h>
3 :
< std::string command = "cd Applications/Musique.app/Contents/Resources; ./afplay.command \"" + morceaux_path + "/" + morceau_nom + "\"";
---
> std::string command = "./afplay.command \"" + morceaux_path + "/" + morceau_nom + "\"";

######################################################################
######################################################################

Noé Aubin-Cadot, 2019.