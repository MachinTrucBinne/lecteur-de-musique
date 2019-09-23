/*
Noé Aubin-Cadot.
Codé en juin 2017.
Déposé sur GitHub en septembre 2019.
C'est un lecteur de musique, version application.
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Compiler avec :
// g++ -o main main.cpp -framework SDL2 -framework SDL2_ttf -lavformat
// ou encore, avec :
// g++ $(sdl2-config --cflags) -Wall -o main  main.cpp $(sdl2-config --libs) -lSDL2_ttf -lavformat
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SDL2 et SDL2_TTF sont dans //library/frameworks (à ajouter à Xcode le cas échant)
// lavformat.dylib est une librairie qui vient avec ffmpeg (via brew install ffmpeg)
// lavformat.dylib se trouve en : //usr/local/lib (à ajouter à Xcode le cas échéant)
////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Pour C++
#include <iostream> // pour std::cout
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <math.h>
#include <cmath>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>   // pour manipuler des std::strings
#include <dirent.h> // pour regarder les fichiers dans un directory, i.e. pour opendir()
#include <signal.h>
#include <iterator>

#include <sys/types.h> // pour gérer les processus zombies
#include <sys/wait.h>  // pour gérer les processus zombies

// Pour SDL
#include <SDL2/SDL.h>
#include <SDL2_ttf/SDL_ttf.h>
//#include <SDL2/SDL_ttf.h>

extern "C" {
#include <libavformat/avformat.h>
}

// Création d'une fenêtre et d'un renderer
SDL_Window *fenetre;
SDL_Renderer *renderer;

unsigned char isFile    =0x8;
unsigned char isFolder  =0x4;

// fonction qui retourne la position des lettres dans un array de 26 int
// si la lettre n'est pas présente, on met position -1
std::vector<int> PositionDesLettres(std::string* liste_de_string)
{
    int code_premiere_lettre_temporaire=0;
    int code_premiere_lettre_previous_temporaire=0;
    int code_nombre_de_lettres_sautees;
    std::vector<int> position_des_lettres_temporaire;
    // On cherche la première lettre
    int lettre_trouvee = 0;
    int position_premiere_lettre = -1;
    while(!lettre_trouvee)
    {
        position_premiere_lettre = position_premiere_lettre + 1;
        code_premiere_lettre_temporaire = (int)(liste_de_string[position_premiere_lettre].at(0)) - 65;
        if ((-1<code_premiere_lettre_temporaire) && (code_premiere_lettre_temporaire<26))
        {
            lettre_trouvee = 1;
        }
    }
    if (position_premiere_lettre == -1)
    {
        std::cout<<"\nERREUR : aucune premiere lettre trouvee";
    }
    else
    {
        int code_premiere_lettre_dernier_trouve=0;
        position_des_lettres_temporaire.push_back(position_premiere_lettre);
        // Ensuite on cherche la position des caractères
        for (int i=position_premiere_lettre; !liste_de_string[i].empty(); i++) // on continue tant que liste_de_string[i] est non vide
        {
            code_premiere_lettre_temporaire = (int)(liste_de_string[i].at(0)) - 65;
            if ((code_premiere_lettre_temporaire > code_premiere_lettre_previous_temporaire) && (-1<code_premiere_lettre_temporaire) && (code_premiere_lettre_temporaire<26))
            {
                code_nombre_de_lettres_sautees = code_premiere_lettre_temporaire - code_premiere_lettre_previous_temporaire;
                for (int j=0;j<code_nombre_de_lettres_sautees;j++)
                {
                    position_des_lettres_temporaire.push_back(i); // on met la valeur i dans le vecteur
                    code_premiere_lettre_dernier_trouve = i;
                }
            }
            code_premiere_lettre_previous_temporaire = code_premiere_lettre_temporaire;
        }
        // Puis on regarde la dernière lettre trouvée et on y met la dernière valeur
        for (int i=position_des_lettres_temporaire.size()-1;i<26;i++)
        {
            position_des_lettres_temporaire.push_back(code_premiere_lettre_dernier_trouve);
        }
    }
    return position_des_lettres_temporaire;
}

// Cette fonction vide une texture
void ViderTexture(SDL_Texture* &texture_textbox)
{
    SDL_SetRenderTarget(renderer, texture_textbox); // on modifie la texture_textbox_input et non plus le renderer
    SDL_SetRenderDrawColor(renderer,0,0,0,0);
    SDL_RenderClear(renderer);  // on vide le renderer (i.e. la texture). Ça remplit de noir car je viens de mettre la couleur de dessin noir (dernière ligne)
    SDL_RenderDrawRect(renderer,NULL);
    SDL_SetRenderTarget(renderer, NULL);// Dorénavent, on modifie à nouveau le renderer et non plus la texture target
}

// Cette fonction ajoute dans la texture_textbox un rectangle coloré et d'un texte (à une position donnée)
void AjouterBoutonAvecTexte(SDL_Texture* &texture_textbox,std::string Texte, SDL_Rect rectangle,TTF_Font* fonte,SDL_Color couleur_texte,SDL_Color couleur_texte_box)
{
    // On initialise le bouton
    SDL_SetRenderTarget(renderer, texture_textbox); // on modifie la texture_textbox_input et non plus le renderer
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);  // on met texture_textbox_input en mode blend
    // On met un rectangle en texture_textbox_input :
    SDL_SetRenderDrawColor(renderer,couleur_texte_box.r,couleur_texte_box.g,couleur_texte_box.b,couleur_texte_box.a);    // les 3 premiers param sont la couleur et le dernier param. est l'opacité
    SDL_RenderFillRect(renderer, &rectangle); // on met le rectangle dans texture_textbox_input
    SDL_SetRenderDrawColor(renderer,0,0,0,0);    // les 3 premiers param sont la couleur et le dernier param. est l'opacité
    // On met du texte en texture_textbox_input :
    if (Texte != "")
    {
        SDL_Surface* surface_texte_temporaire = nullptr;
        SDL_Texture* texture_text_temporaire = nullptr;
        surface_texte_temporaire = TTF_RenderUTF8_Blended(fonte,Texte.c_str(),couleur_texte);
        if (surface_texte_temporaire == NULL){std::cout<<"\n\nERREUR : création de la surface_texte_temporaire a échoué"<<std::endl;} // un message d'erreur si jamais
        SDL_SetSurfaceAlphaMod(surface_texte_temporaire,couleur_texte.a);
        texture_text_temporaire = SDL_CreateTextureFromSurface(renderer, surface_texte_temporaire);
        SDL_Rect rectangle_temporaire = rectangle;
        SDL_QueryTexture(texture_text_temporaire, NULL, NULL, &rectangle_temporaire.w, &rectangle_temporaire.h); // utile pour avoir la bonne taille de texte dans la boîte
        SDL_RenderCopy(renderer, texture_text_temporaire, NULL, &rectangle_temporaire);
        // On vide la mémoire
        SDL_FreeSurface(surface_texte_temporaire);
        surface_texte_temporaire = nullptr;
        SDL_DestroyTexture(texture_text_temporaire);
        texture_text_temporaire = nullptr;
    }
    // On rend le renderer indépendant du texture_textbox_input
    SDL_SetRenderTarget(renderer, NULL);
}

char ToucheVersChar(int InputKey)
{
    char Character = '\0';
    switch(InputKey)
    {
        case SDL_SCANCODE_A:
            Character = 'A';
            break;
        case SDL_SCANCODE_B:
            Character = 'B';
            break;
        case SDL_SCANCODE_C:
            Character = 'C';
            break;
        case SDL_SCANCODE_D:
            Character = 'D';
            break;
        case SDL_SCANCODE_E:
            Character = 'E';
            break;
        case SDL_SCANCODE_F:
            Character = 'F';
            break;
        case SDL_SCANCODE_G:
            Character = 'G';
            break;
        case SDL_SCANCODE_H:
            Character = 'H';
            break;
        case SDL_SCANCODE_I:
            Character = 'I';
            break;
        case SDL_SCANCODE_J:
            Character = 'J';
            break;
        case SDL_SCANCODE_K:
            Character = 'K';
            break;
        case SDL_SCANCODE_L:
            Character = 'L';
            break;
        case SDL_SCANCODE_M:
            Character = 'M';
            break;
        case SDL_SCANCODE_N:
            Character = 'N';
            break;
        case SDL_SCANCODE_O:
            Character = 'O';
            break;
        case SDL_SCANCODE_P:
            Character = 'P';
            break;
        case SDL_SCANCODE_Q:
            Character = 'Q';
            break;
        case SDL_SCANCODE_R:
            Character = 'R';
            break;
        case SDL_SCANCODE_S:
            Character = 'S';
            break;
        case SDL_SCANCODE_T:
            Character = 'T';
            break;
        case SDL_SCANCODE_U:
            Character = 'U';
            break;
        case SDL_SCANCODE_V:
            Character = 'V';
            break;
        case SDL_SCANCODE_W:
            Character = 'W';
            break;
        case SDL_SCANCODE_X:
            Character = 'X';
            break;
        case SDL_SCANCODE_Y:
            Character = 'Y';
            break;
        case SDL_SCANCODE_Z:
            Character = 'Z';
            break;
    }
    return Character;
}

std::string home_directory()
{
    // On trouve le chemin ~/ i.e. //Users/NAC
    const char *homeDir = getenv("HOME");
    if (!homeDir)
    {
        struct passwd* pwd = getpwuid(getuid());
        if (pwd){homeDir = pwd->pw_dir;}
    }
    return std::string(homeDir);
}

std::string artists_directory()
{
    return "/" + home_directory() + "/Music/iTunes/iTunes Media/Music";
}

void charger_les_artistes(int &nombre_dartistes, std::string artists_path, std::string *artistes_noms)
{
    std::string string_buffer = "";
    DIR *directory = opendir (artists_path.c_str());
    if (directory == NULL){std::cout<<"\n\nERREUR : Le directory d'artistes n'a pu être ouvert. Le programme quitte.";exit(EXIT_FAILURE);}
    
    nombre_dartistes = 0;
    struct dirent *entries;
    while ((entries = readdir (directory)) != NULL)
    {
        if (entries->d_type == isFolder) // isFolder pour dossiers et isFile pour fichiers
        {
            string_buffer = entries->d_name;
            if ( (string_buffer.compare(".")) && (string_buffer.compare("..")) && (string_buffer.compare(".sync")))
            {
                artistes_noms[nombre_dartistes] = entries->d_name;
                nombre_dartistes = nombre_dartistes + 1;
            }
        }
    }
    closedir (directory);
    std::sort(artistes_noms,artistes_noms + nombre_dartistes); // pour ordonner les artistes (car readdir n'ordonne pas en ordre alphabétique mais selon la structure du disque)
}

void charger_les_albums(int numero_artiste, int &nombre_dalbums, std::string artists_path, std::string &album_path, std::string *artistes_noms, std::string *albums_noms)
{
    album_path = artists_path + "/" + artistes_noms[numero_artiste];
    std::string string_buffer = "";
    DIR *directory = opendir (album_path.c_str());
    if (directory == NULL){std::cout<<"\n\nERREUR : Le directory d'album n'a pu être ouvert. Bye bye.";exit(EXIT_FAILURE);}
    
    nombre_dalbums = 0;
    struct dirent *entries;
    for (int i=0;i<100;i++){albums_noms[i] = "";}
    while ((entries = readdir (directory)) != NULL)
    {
        if (entries->d_type == isFolder) // isFolder pour dossiers et isFile pour fichiers
        {
            string_buffer = entries->d_name;
            if ( (string_buffer.compare(".")) && (string_buffer.compare("..")) && (string_buffer.compare(".sync")))
            {
                albums_noms[nombre_dalbums] = entries->d_name;
                //std::cout<<"\n"<<albums_noms[nombre_dalbums]<<"\t\t"<<nombre_dalbums;
                nombre_dalbums = nombre_dalbums + 1;
            }
        }
    }
    closedir (directory);
    std::sort(albums_noms,albums_noms + nombre_dalbums); // pour ordonner les albums (car readdir n'ordonne pas en ordre alphabétique mais selon la structure du disque)
}

void charger_les_morceaux(int numero_album, int &nombre_de_morceaux, std::string artists_path, std::string album_path, std::string &morceaux_path, std::string *artistes_noms, std::string *albums_noms, std::string *morceaux_noms)
{
    morceaux_path = album_path + "/" + albums_noms[numero_album];
    std::string string_buffer = "";
    DIR *directory = opendir (morceaux_path.c_str());
    if (directory == NULL){std::cout<<"\n\nERREUR : Le directory de morceaux n'a pu être ouvert. Bye bye.";exit(EXIT_FAILURE);}
    
    nombre_de_morceaux = 0;
    struct dirent *entries;
    for (int i=0;i<10000;i++){morceaux_noms[i] = "";}
    while ((entries = readdir (directory)) != NULL)
    {
        if (entries->d_type == isFile) // isFolder pour dossiers et isFile pour fichiers
        {
            string_buffer = entries->d_name;
            if ( (string_buffer.compare(".")) && (string_buffer.compare("..")) && (string_buffer.compare(".sync")))
            {
                morceaux_noms[nombre_de_morceaux] = entries->d_name;
                //std::cout<<"\n"<<morceaux_noms[nombre_de_morceaux]<<"\t\t"<<nombre_de_morceaux;
                nombre_de_morceaux = nombre_de_morceaux + 1;
            }
        }
    }
    closedir (directory);
    std::sort(morceaux_noms,morceaux_noms + nombre_de_morceaux); // pour ordonner les morceaux (car readdir n'ordonne pas en ordre alphabétique mais selon la structure du disque)
}

bool isRunning(std::string name)
{
    std::string command = "pgrep " + name + " > /dev/null";
    return (0 == system(command.c_str()));
}

int PID_of_process(std::string name)
{
    std::string string_buffer = "(pgrep " + name + ")";
    char path[PATH_MAX];
    FILE *fp = popen(string_buffer.c_str(), "r");
    fgets(path, PATH_MAX, fp);
    int PID_number;
    sscanf(path, "%i", &PID_number);
    return PID_number;
}

double duree_toune(std::string morceaux_path, std::string morceau_nom)
{
    std::string filename = morceaux_path + "/" + morceau_nom;
    av_register_all();
    AVFormatContext* pFormatCtx = avformat_alloc_context();
    avformat_open_input(&pFormatCtx, filename.c_str(), NULL, NULL);
    avformat_find_stream_info(pFormatCtx,NULL);
    int64_t duration = pFormatCtx->duration;
    avformat_close_input(&pFormatCtx);
    avformat_free_context(pFormatCtx);
    return duration/1000000.0;
}

void play_toune(std::string player_nom, std::string morceaux_path, std::string morceau_nom)
{
    // Pour la version application c'est :
    std::string command = "cd Applications/Musique.app/Contents/Resources; ./afplay.command \"" + morceaux_path + "/" + morceau_nom + "\"";
    // Pour la version exécutable c'est :
    //std::string command = "./afplay.command \"" + morceaux_path + "/" + morceau_nom + "\"";
    
    std::cout<<"\nCommand = "<<command<<std::endl;
    popen(command.c_str(), "r");
}

void pause_player(std::string player_nom)
{
    int PID_number = PID_of_process(player_nom);
    kill(PID_number, SIGSTOP);
}

void resume_player(std::string player_nom)
{
    int PID_number = PID_of_process(player_nom);
    kill(PID_number, SIGCONT);
}

void quit_player(std::string player_nom)
{
    int terminating_try_number = 0;
    while(isRunning(player_nom))
    {
        int PID_number = PID_of_process(player_nom);
        std::cout<<"\nKill AFPLAY's PID : "<<PID_number<<std::endl;
        kill(PID_number, SIGTERM);
        terminating_try_number = terminating_try_number + 1;
        //if (terminating_try_number > 5){kill(PID_number, SIGKILL);}
    }
}

// Création de deux entiers qui ont pour valeur le nombre de pixels en largeur et en hauteur de la fenêtre à l'ouverture
int gl_w = 800;
int gl_h = 500;

int main(int argc, char* argv[])
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////       Initialisation de diverses choses      //////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    // INITIALISATION SDL2 et TTF
    if(SDL_Init(SDL_INIT_VIDEO) != 0){std::cout<<"Erreur d'initialisation de SDL_Init :\n"<<SDL_GetError();exit(EXIT_FAILURE);}
    if(TTF_Init() != 0){std::cout<<"Erreur d'initialisation de TTF_Init :\n"<<TTF_GetError();exit(EXIT_FAILURE);}
    // CRÉATION D'UNE FENÊTRE
    fenetre = SDL_CreateWindow("♫ Musique ♫",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,gl_w,gl_h, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (fenetre == NULL){std::cout<<"La création de la fenêtre SDL a échoué : "<<SDL_GetError();exit(EXIT_FAILURE);}
    SDL_DisplayMode current_display_mode;
    if (SDL_GetDesktopDisplayMode(0, &current_display_mode) != 0){SDL_Log("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());exit(EXIT_FAILURE);}     // message d'erreur si SDL échoue
    // CRÉATION D'UN RENDERER
    renderer = SDL_CreateRenderer(fenetre,-1,SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_GL_GetDrawableSize(fenetre,&gl_w,&gl_h); // ici on met les valeurs en gl_w et en gl_h
    // remarque : il faut mettre à jour gl_w et gl_h si la fenêtre est redimensionnée
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////       Pour se promener dans les dossiers de musique      /////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    int nombre_delements = 0;
    int nombre_dartistes = 0;
    int nombre_dalbums = 0;
    int nombre_de_morceaux = 0;
    int nombre_de_morceaux_lecture = 0;
    
    int nombre_delements_affiches = 20; // nombre d'éléments affichés dans la liste d'éléments
    
    std::string artistes_noms[10000];       // 10,000 artistes maximum
    std::string albums_noms[100];           // 100 albums maximum par artiste
    std::string albums_noms_lecture[100];   // 100 albums maximum par artiste
    std::string morceaux_noms[10000];       // 10,000 morceaux par album maximum
    std::string morceaux_noms_lecture[10000];  // 10,000 morceaux par album maximum
    std::string string_buffer = "";
    
    std::string artists_path = artists_directory(); //  "//Users/NAC/Music/iTunes/iTunes Media/Music"
    std::string album_path = "";
    std::string morceaux_path = "";
    std::string morceaux_path_lecture = "";
    std::string chanson_path = "";
    
    charger_les_artistes(nombre_dartistes, artists_path, artistes_noms);
    nombre_delements = nombre_dartistes;
    
    std::vector<int> position_des_lettres;
    position_des_lettres = PositionDesLettres(artistes_noms);
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////       Création de textures etc.      //////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    // DÉCLARATION D'UNE FONTE (choix de fonte + taille en pixels)
    std::string fonte_string_symboles = "fontes/Andale-Mono.ttf"; // bon pour les flèches et emoji de musique, mais pas pour les accents
    std::string fonte_string_texte = "fontes/Courier.dfont"; // les accents sont bon, mais les flèches et l'emoji musique ne marchent pas
    int taille_pixels_fonte = (int) gl_w * (14.0/16.0) / (2.4*nombre_delements_affiches);
    TTF_Font* fonte_texte = TTF_OpenFont(fonte_string_texte.c_str(), taille_pixels_fonte); // avec 100 c'est pixelisé vintage mais c'est super rapide et le processeur roule à 11% au lieu de 99%
    TTF_Font* fonte_symboles = TTF_OpenFont(fonte_string_symboles.c_str(), taille_pixels_fonte); // avec 100 c'est pixelisé vintage mais c'est super rapide et le processeur roule à 11% au lieu de 99%
    if(fonte_texte == NULL){std::cout<<"\n\nERREUR : fonte de texte non chargée\n"<<std::endl;exit(EXIT_FAILURE);} // On vérifie si la fonte a été chargée
    if(fonte_symboles == NULL){std::cout<<"\n\nERREUR : fonte de symboles non chargée\n"<<std::endl;exit(EXIT_FAILURE);} // On vérifie si la fonte a été chargée
    
    int taille_pixels_fonte_fleche = (int) gl_w * (14.0/16.0) / (1.2*nombre_delements_affiches);
    TTF_Font* fonte_fleche = TTF_OpenFont(fonte_string_symboles.c_str(), taille_pixels_fonte_fleche); // avec 100 c'est pixelisé vintage mais c'est super rapide et le processeur roule à 11% au lieu de 99%
    if(fonte_fleche == NULL){std::cout<<"\n\nERREUR : fonte_fleche non chargée\n"<<std::endl;exit(EXIT_FAILURE);} // On vérifie si la fonte a été chargée
    
    // Les "numero_element_..." servent autant aux artistes, aux albums qu'aux morceaux.
    int numero_element_du_haut   = 0; // numero de l'élément du haut (dans l'affichage)
    int numero_element_vise      = 0; // numero de l'élément surligné (entre 0 (inclusif) et nombre_delements_affiches (exclusif)). Ce viseur sert à chaque viseur.
    int numero_element_liste     = 0; // variable utile pour l'affichage
    
    int numero_artiste_du_haut   = 0; // numéro de l'artiste au haut de la liste affichée
    int numero_artiste_vise      = 0; // numéro de l'artiste visé (ce nombre commence à 0). Le numéro de l'artiste est numero_artiste_du_haut + numero_artiste vise
    int numero_artiste_lecture   = 0;
    
    int numero_album_du_haut     = 0; // numéro de l'album du haut
    int numero_album_vise        = 0; // numéro de l'album visé (le numéro absolu de l'album = numero_album_du_haut + numero_album_vise)
    int numero_album_lecture     = 0; // numéro absolu de l'album en lecture
    
    int numero_morceau_du_haut   = 0;
    int numero_morceau_vise      = 0; // numéro du morceau visé
    int numero_morceau_lecture   = 0;
    
    int nombre_de_pixels_entre_les_elements = 4; // utile pour l'affichage (nombre de pixels entre les boîtes d'éléments de la liste d'éléments)
    
    // On crée une boîte pour le titre en haut
    SDL_Rect position_textbox_contexte   = {(int)gl_w/16.0,(int)gl_h/32.0,(int)gl_w * (14.0/16.0),(int)gl_h/16.0}; // x, y, w, h
    SDL_Color couleur_texte_contexte     = {0,255,255,155};
    SDL_Color couleur_texte_box_contexte = {0,100,100,150};
    std::string texte_contexte = "Artistes";
    
    // On crée une boîte pour l'info de ce qui joue en bas
    SDL_Rect position_textbox_info   = {(int)gl_w/16.0,(int)gl_h*(29.0/32.0),(int)gl_w * (14.0/16.0),(int)gl_h/16.0};
    SDL_Color couleur_texte_info     = {0,255,255,155};
    SDL_Color couleur_texte_box_info = {0,100,100,150};
    std::string texte_info = "";
    
    // On crée une boîte pour les éléments (artistes, albums, morceaux)
    SDL_Rect position_textbox_elements   = {(int)gl_w/16.0,(int)gl_h/8.0,(int)gl_w * (14.0/16.0),(int)gl_h*(6.0/8.0)};
    SDL_Color couleur_texte_elements     = {0,255,255,155};
    SDL_Color couleur_texte_box_elements = {0,150,100,100};
    std::string texte_elements = "";
    
    // position et couleur du rectangle coloré du viseur
    SDL_Rect position_viseur            = { 0,
        (int)numero_element_vise * (position_textbox_elements.h /nombre_delements_affiches),
        position_textbox_elements.w,
        (int)position_textbox_elements.h/(nombre_delements_affiches) - nombre_de_pixels_entre_les_elements};
    SDL_Color couleur_viseur_texte      = {100,255,100,200};
    SDL_Color couleur_viseur_box        = {255,100,100,150};
    
    // rectangle utile pour générer les éléments de la liste
    SDL_Rect position_texte     = {0,(int)numero_element_liste * (position_textbox_elements.h /nombre_delements_affiches),position_textbox_elements.w,(int)position_textbox_elements.h/(nombre_delements_affiches) - nombre_de_pixels_entre_les_elements};
    
    // On crée une boîte pour la flèche vers le haut et vers le bas
    SDL_Rect position_textbox_fleche_haut_droit     = {(int)gl_w*(15.0/16.0),(int)gl_h*(3/32.0),(int)gl_w/16.0,(int)gl_h/16.0};
    SDL_Rect position_textbox_fleche_haut_gauche    = {(int)gl_w*(1.0/32.0),(int)gl_h*(3/32.0),(int)gl_w/16.0,(int)gl_h/16.0};
    SDL_Rect position_textbox_fleche_bas_droit      = {(int)gl_w*(15.0/16.0),(int)gl_h/8.0 + (nombre_delements_affiches - 2) * (position_textbox_elements.h /nombre_delements_affiches),(int)gl_w/16.0,(int)gl_h/16.0};
    SDL_Rect position_textbox_fleche_bas_gauche     = {(int)gl_w*(1.0/32.0),(int)gl_h/8.0 + (nombre_delements_affiches - 2) * (position_textbox_elements.h /nombre_delements_affiches),(int)gl_w/16.0,(int)gl_h/16.0};
    SDL_Color couleur_texte_fleche     = {0,255,255,155};
    SDL_Color couleur_texte_box_fleche = {0,0,0,150};
    std::string texte_fleche_haut = "↑";
    std::string texte_fleche_bas  = "↓";
    
    // On crée une boîte pour le temps restant
    SDL_Rect position_textbox_duree_restante   = {(int)gl_w*(27.0/32.0),(int)gl_h*(30.0/32.0),(int)gl_w * (3.0/32.0),(int)gl_h/32.0};
    SDL_Rect position_textbox_duree_restante_interieur   = {0,0,position_textbox_duree_restante.w,position_textbox_duree_restante.h};
    SDL_Color couleur_texte_duree_restante     = {0,255,255,155};
    SDL_Color couleur_texte_box_duree_restante = {0,100,100,150};
    std::string morceau_duree_restante_texte = "";
    
    // On crée une boîte pour l'indicateur de ce qui joue
    SDL_Rect position_indicateur            = { (int)(position_textbox_elements.w)*(63.0/64.0),
        (int)numero_morceau_lecture  * (position_textbox_elements.h /nombre_delements_affiches),
        (int)position_textbox_elements.w / 64.0,
        (int)position_textbox_elements.h/(nombre_delements_affiches) - nombre_de_pixels_entre_les_elements};
    SDL_Color couleur_texte_indicateur      = {255,255,100,200};
    SDL_Color couleur_texte_box_indicateur  = {0,0,0,0};
    std::string texte_indicateur = "♫";

    // CRÉATION DE TEXTURES
    // Texture pour le titre en haut et l'info en bas :
    SDL_Texture* texture_textbox_titre             = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,gl_w,gl_h);
    // Texture qui indique le temps restant
    SDL_Texture* texture_textbox_duree_restante    = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,position_textbox_duree_restante.w,position_textbox_duree_restante.h);
    // Texture pour les éléments affichés de la liste :
    SDL_Texture* texture_textbox_elements          = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,position_textbox_elements.w,position_textbox_elements.h);
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////       On met les choses dans la fenêtre pour une première fois      //////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    // INITIALISATION DES TEXTURES
    // On vide les textures
    ViderTexture(texture_textbox_titre);
    ViderTexture(texture_textbox_duree_restante);
    ViderTexture(texture_textbox_elements);
    // Initialisation de la boîte titre
    AjouterBoutonAvecTexte(texture_textbox_titre,texte_contexte, position_textbox_contexte,fonte_texte,couleur_texte_contexte,couleur_texte_box_contexte);
    AjouterBoutonAvecTexte(texture_textbox_titre,texte_info, position_textbox_info,fonte_texte,couleur_texte_info,couleur_texte_box_info);
    // Initialisation de la boîte de temps restant
    AjouterBoutonAvecTexte(texture_textbox_duree_restante,morceau_duree_restante_texte, position_textbox_duree_restante_interieur,fonte_texte,couleur_texte_duree_restante,couleur_texte_box_duree_restante);
    // Initialisation de la boîte avec les entrées
    
    int contexte = 0; // 0 = artistes, 1 = albums, 2 = morceaux
    numero_element_du_haut = numero_artiste_du_haut;
    for (int i=0;i<nombre_delements_affiches;i++)
    {
        position_texte.y = (int)i * (position_textbox_elements.h /nombre_delements_affiches);
        if (contexte == 0){texte_elements = artistes_noms[i + numero_element_du_haut];}
        if (contexte == 1){texte_elements = albums_noms[  i + numero_element_du_haut];}
        if (contexte == 2){texte_elements = morceaux_noms[i + numero_element_du_haut];}
        if ( i != numero_element_vise ){AjouterBoutonAvecTexte(texture_textbox_elements,texte_elements, position_texte,fonte_texte,couleur_texte_elements,couleur_texte_box_elements);}
        else
        {
            AjouterBoutonAvecTexte(texture_textbox_elements,texte_elements, position_viseur,fonte_texte,couleur_viseur_texte,couleur_viseur_box);
        }
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////       Déclaration de variables utiles      //////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Déclaration de variables utiles dans la boucle
    
    int delay = 16;
    int delay_actif = 1; // mieux vaut laisser à 1 pour la fluidité
    
    int event_activation=0; // variable utile pour l'optimisation du code (quand on fait un truc ça active event_activation et active l'affichage)
    
    SDL_Point mouse_position; // variable qui encodera la pposition de la souris
    
    int play = 0; // 0 = pause, 0 = play
    
    std::string morceau_nom_lecture = "";
    std::string player_nom = "afplay";
    int afplay_actif = isRunning(player_nom);
    int chanson_actif = 0; // ça dit si une chanson est active (i.e. soit play soit sur pause)
    double morceau_duree_totale;
    double morceau_duree_restante;
    int nombre_de_morceaux_restants = 0;
    int bouton_left_temps_restant = 0;
    
    int currentTime; // j'ai besoin du temps pour savoir il reste combien de temps à la toune
    int lastTime = 0;
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////       Début de la boucle principale      //////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // On commence les événements
    
    SDL_Event event;
    bool quit = false;
    while (!quit)
    {
        if (delay_actif == 1){SDL_Delay(delay);}
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                quit = true;
            }
            if (event.type == SDL_WINDOWEVENT)
            {
                event_activation = 1;
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) // si on change la taille de la fenêtre
                {
                    // On met les nouvelles dimension de la fenêtre en gl_w et gl_h
                    SDL_GL_GetDrawableSize(fenetre,&gl_w,&gl_h);
                    // On reload la fonte (car taille de la police doit changer)
                    taille_pixels_fonte = (int) gl_w * (14.0/16.0) / (2.4*nombre_delements_affiches);
                    fonte_texte = TTF_OpenFont(fonte_string_texte.c_str(), taille_pixels_fonte);
                    fonte_symboles = TTF_OpenFont(fonte_string_symboles.c_str(), taille_pixels_fonte);
                    if(fonte_texte == NULL){std::cout<<"\n\nERREUR : fonte texte non chargée\n"<<std::endl;exit(EXIT_FAILURE);}
                    if(fonte_symboles == NULL){std::cout<<"\n\nERREUR : fonte symboles non chargée\n"<<std::endl;exit(EXIT_FAILURE);}
                    taille_pixels_fonte_fleche = (int) gl_w * (14.0/16.0) / (nombre_delements_affiches);
                    fonte_fleche = TTF_OpenFont(fonte_string_symboles.c_str(), taille_pixels_fonte_fleche);
                    if(fonte_fleche == NULL){std::cout<<"\n\nERREUR : fonte_fleche non chargée\n"<<std::endl;exit(EXIT_FAILURE);}
                    // On recalcule les dimensions des boîtes
                    position_textbox_contexte   = (SDL_Rect){(int)gl_w/16.0,(int)gl_h/32.0,(int)gl_w * (14.0/16.0),(int)gl_h/16.0};
                    position_textbox_info       = (SDL_Rect){(int)gl_w/16.0,(int)gl_h*(29.0/32.0),(int)gl_w * (14.0/16.0),(int)gl_h/16.0};
                    position_textbox_elements   = (SDL_Rect){(int)gl_w/16.0,(int)gl_h/8.0,(int)gl_w * (14.0/16.0),(int)gl_h*(6.0/8.0)};
                    position_texte              = (SDL_Rect){0,(int)numero_element_liste * (position_textbox_elements.h /nombre_delements_affiches),position_textbox_elements.w,(int)position_textbox_elements.h/(nombre_delements_affiches) - nombre_de_pixels_entre_les_elements};
                    position_viseur             = (SDL_Rect){0,(int)numero_element_vise  * (position_textbox_elements.h /nombre_delements_affiches),position_textbox_elements.w,(int)position_textbox_elements.h/(nombre_delements_affiches) - nombre_de_pixels_entre_les_elements};
                    position_textbox_fleche_haut_droit          = (SDL_Rect){(int)gl_w*(15.0/16.0),(int)gl_h*(3/32.0),(int)gl_w/16.0,(int)gl_h/16.0};
                    position_textbox_fleche_haut_gauche         = (SDL_Rect){(int)gl_w*(1.0/32.0),(int)gl_h*(3/32.0),(int)gl_w/16.0,(int)gl_h/16.0};
                    position_textbox_fleche_bas_droit           = (SDL_Rect){(int)gl_w*(15.0/16.0),(int)gl_h/8.0 + (nombre_delements_affiches - 2) * (position_textbox_elements.h /nombre_delements_affiches),(int)gl_w/16.0,(int)gl_h/16.0};
                    position_textbox_fleche_bas_gauche          = (SDL_Rect){(int)gl_w*(1.0/32.0),(int)gl_h/8.0 + (nombre_delements_affiches - 2) * (position_textbox_elements.h /nombre_delements_affiches),(int)gl_w/16.0,(int)gl_h/16.0};
                    position_textbox_duree_restante           = (SDL_Rect){(int)gl_w*(27.0/32.0),(int)gl_h*(30.0/32.0),(int)gl_w * (3.0/32.0),(int)gl_h/32.0};
                    position_textbox_duree_restante_interieur = (SDL_Rect){0,0,position_textbox_duree_restante.w,position_textbox_duree_restante.h};
                    position_indicateur    = (SDL_Rect){(int)(position_textbox_elements.w)*(63.0/64.0),
                        (int)numero_morceau_lecture  * (position_textbox_elements.h /nombre_delements_affiches),
                        (int)position_textbox_elements.w / 64.0,
                        (int)position_textbox_elements.h/(nombre_delements_affiches) - nombre_de_pixels_entre_les_elements};
                    // on détruit les textures
                    SDL_DestroyTexture(texture_textbox_titre);
                    SDL_DestroyTexture(texture_textbox_elements);
                    SDL_DestroyTexture(texture_textbox_duree_restante);
                    // on recrée les textures
                    texture_textbox_titre                = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,gl_w,gl_h);
                    texture_textbox_elements             = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,position_textbox_elements.w,position_textbox_elements.h);
                    texture_textbox_duree_restante       = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,position_textbox_duree_restante.w,position_textbox_duree_restante.h);
                    ViderTexture(texture_textbox_titre);
                    ViderTexture(texture_textbox_elements);
                    ViderTexture(texture_textbox_duree_restante);
                    // Texture titre
                    AjouterBoutonAvecTexte(texture_textbox_titre,texte_contexte, position_textbox_contexte,fonte_texte, couleur_texte_contexte, couleur_texte_box_contexte);
                    AjouterBoutonAvecTexte(texture_textbox_titre,texte_info,     position_textbox_info,    fonte_texte, couleur_texte_info,     couleur_texte_box_info    );
                    // Texture du temps restant
                    AjouterBoutonAvecTexte(texture_textbox_duree_restante,morceau_duree_restante_texte, position_textbox_duree_restante_interieur,fonte_texte,couleur_texte_duree_restante,couleur_texte_box_duree_restante);
                    // Texture éléments
                    for (int i=0;i<nombre_delements_affiches;i++)
                    {;
                        position_texte.y = (int) i * (position_textbox_elements.h /nombre_delements_affiches);
                        if (contexte == 0){texte_elements = artistes_noms[i + numero_element_du_haut];}
                        if (contexte == 1){texte_elements = albums_noms[  i + numero_element_du_haut];}
                        if (contexte == 2){texte_elements = morceaux_noms[i + numero_element_du_haut];}
                        if ( i != numero_element_vise ){AjouterBoutonAvecTexte(texture_textbox_elements,texte_elements, position_texte,fonte_texte,couleur_texte_elements,couleur_texte_box_elements);}
                        else
                        {
                            AjouterBoutonAvecTexte(texture_textbox_elements,texte_elements, position_viseur,fonte_texte,couleur_viseur_texte,couleur_viseur_box);
                        }
                    }
                    if (chanson_actif == 1)
                    {
                        if (contexte == 0)
                        {
                            // mettre l'indicateur
                            position_indicateur.y = (int)numero_artiste_lecture  * (position_textbox_elements.h /nombre_delements_affiches);
                            AjouterBoutonAvecTexte(texture_textbox_elements,texte_indicateur,  position_indicateur,fonte_symboles,couleur_texte_indicateur,couleur_texte_box_indicateur);
                        }
                        if ( (contexte == 1) && (numero_artiste_du_haut + numero_artiste_vise == numero_artiste_lecture) )
                        {
                            // mettre l'indicateur
                            position_indicateur.y = (int)numero_album_lecture  * (position_textbox_elements.h /nombre_delements_affiches);
                            AjouterBoutonAvecTexte(texture_textbox_elements,texte_indicateur,  position_indicateur,fonte_symboles,couleur_texte_indicateur,couleur_texte_box_indicateur);
                        }
                        if ( (contexte == 2) && (numero_artiste_du_haut + numero_artiste_vise == numero_artiste_lecture) && (numero_album_du_haut + numero_album_vise == numero_album_lecture) )
                        {
                            // mettre l'indicateur
                            position_indicateur.y = (int)numero_morceau_lecture  * (position_textbox_elements.h /nombre_delements_affiches);
                            AjouterBoutonAvecTexte(texture_textbox_elements,texte_indicateur,  position_indicateur,fonte_symboles,couleur_texte_indicateur,couleur_texte_box_indicateur);
                        }
                    }
                }
            }
            
            // Si on bouge la souris
            if (event.type == SDL_MOUSEMOTION)
            {
                SDL_GetMouseState(&mouse_position.x,&mouse_position.y);
                if ((mouse_position.x > position_textbox_elements.x/2.0) && (mouse_position.x < (position_textbox_elements.x + position_textbox_elements.w)/2.0))
                {
                    if ((mouse_position.y > position_textbox_elements.y/2.0) && (mouse_position.y < (position_textbox_elements.y + position_textbox_elements.h)/2.0))
                    {
                        int mouse_element_vise = nombre_delements_affiches * (mouse_position.y - position_textbox_elements.y/2.0) / (position_textbox_elements.h/2.0);
                        if (mouse_element_vise < nombre_delements)
                        {
                            event_activation = 1;
                            numero_element_vise = mouse_element_vise;
                            if (contexte == 0){numero_artiste_vise = numero_element_vise;}
                            if (contexte == 1){numero_album_vise   = numero_element_vise;}
                            if (contexte == 2){numero_morceau_vise = numero_element_vise;}
                        }
                    }
                }
            }
            
            // Si on scroll
            if (event.type == SDL_MOUSEWHEEL)
            {
                event_activation = 1;
                if (event.wheel.y > 0) // scrolling up
                {
                    if (numero_element_du_haut>0)
                    {
                        numero_element_du_haut = numero_element_du_haut - 1;
                        if (contexte == 0){numero_artiste_du_haut = numero_element_du_haut;}
                        if (contexte == 1){numero_album_du_haut   = numero_element_du_haut;}
                        if (contexte == 2){numero_morceau_du_haut = numero_element_du_haut;}
                    }
                }
                if (event.wheel.y < 0) // scrolling down
                {
                    if (numero_element_du_haut<nombre_delements - nombre_delements_affiches)
                    {
                        numero_element_du_haut = numero_element_du_haut + 1;
                        if (contexte == 0){numero_artiste_du_haut = numero_element_du_haut;}
                        if (contexte == 1){numero_album_du_haut   = numero_element_du_haut;}
                        if (contexte == 2){numero_morceau_du_haut = numero_element_du_haut;}
                    }
                }
            }
                        
            // Si on clique
            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                event_activation = 1;
                SDL_GetMouseState(&mouse_position.x,&mouse_position.y);
                if ((mouse_position.x > position_textbox_elements.x/2.0) && (mouse_position.x < (position_textbox_elements.x + position_textbox_elements.w)/2.0))
                {
                    //int mouse_element_vise = nombre_delements_affiches * (mouse_position.y - position_textbox_elements.y/2.0) / (position_textbox_elements.h/2.0);
                    if (contexte == 0) // si on arrive dans le dossier d'albums
                    {
                        contexte                = contexte + 1;
                        numero_element_vise     = 0;
                        numero_element_du_haut  = 0;
                        numero_album_vise       = 0;
                        charger_les_albums(numero_artiste_vise + numero_artiste_du_haut, nombre_dalbums, artists_path, album_path, artistes_noms, albums_noms);
                        nombre_delements = nombre_dalbums;
                        texte_contexte = artistes_noms[numero_artiste_vise + numero_artiste_du_haut] + "/Albums";
                    }
                    else if (contexte == 1) // si on arrive dans le dossier de morceaux
                    {
                        contexte                = contexte + 1;
                        numero_element_vise     = 0;
                        numero_element_du_haut  = 0;
                        numero_morceau_vise     = 0;
                        charger_les_morceaux(numero_album_vise + numero_album_du_haut, nombre_de_morceaux, artists_path, album_path, morceaux_path, artistes_noms, albums_noms, morceaux_noms);
                        nombre_delements = nombre_de_morceaux;
                        texte_contexte = artistes_noms[numero_artiste_vise + numero_artiste_du_haut] + "/" + albums_noms[numero_album_vise + numero_album_du_haut] + "/Morceaux";
                    }
                    else if (contexte == 2) // si on clique sur une chanson pour la faire jouer
                    {
                        // On garde l'info sur ce qui joue :
                        numero_artiste_lecture      = numero_artiste_vise + numero_artiste_du_haut;
                        numero_album_lecture        = numero_album_vise + numero_album_du_haut;
                        numero_morceau_lecture      = numero_morceau_vise + numero_morceau_du_haut;
                        nombre_de_morceaux_lecture  = nombre_de_morceaux;
                        nombre_de_morceaux_restants = nombre_de_morceaux_lecture - numero_morceau_lecture -1;
                        morceaux_path_lecture = morceaux_path;
                        // On enregistre les albums de l'artiste qui joue
                        for (int i=0;i<100;i++){albums_noms_lecture[i] = "";}
                        for (int i = numero_album_lecture;i<nombre_dalbums;i++){albums_noms_lecture[i] = albums_noms[i];}
                        // On enregistre les morceaux de l'album qui joue
                        for (int i=0;i<10000;i++){morceaux_noms_lecture[i] = "";}
                        for (int i = 0;i<nombre_de_morceaux_lecture;i++){morceaux_noms_lecture[i] = morceaux_noms[i];}
                        // On fait jouer la toune
                        quit_player(player_nom);
                        morceau_nom_lecture = morceaux_noms_lecture[numero_morceau_lecture];
                        //std::cout<<"\nMorceau_nom = "<<morceau_nom_lecture<<std::endl;
                        play_toune(player_nom, morceaux_path_lecture, morceau_nom_lecture);
                        morceau_duree_totale = duree_toune(morceaux_path_lecture, morceau_nom_lecture);
                        morceau_duree_restante = morceau_duree_totale;
                        afplay_actif = isRunning(player_nom); // afplay devrait être maintenant actif
                        chanson_actif = 1;
                        play = 1;
                        // On affiche l'info :
                        texte_info = std::string("[") + artistes_noms[numero_artiste_lecture] + "]~[" + albums_noms_lecture[numero_album_lecture]  + "]~[" + morceau_nom_lecture + "]";
                    }
                }
            }
            
            // Si on enfonce une touche du clavier
            if (event.type == SDL_KEYDOWN)
            {
                event_activation = 1;
                const Uint8 *state = SDL_GetKeyboardState(NULL);
                
                // ESCAPE = quitter
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    afplay_actif = isRunning(player_nom);
                    if (afplay_actif){quit_player(player_nom);}
                    quit = true;
                }
                
                // BACKSPACE = revenir en arrière
                if (event.key.keysym.scancode == SDL_SCANCODE_BACKSPACE)
                {
                    if (contexte == 1) // si on arrive dans la liste d'artistes
                    {
                        contexte                = contexte - 1;
                        numero_element_vise     = numero_artiste_vise;
                        numero_element_du_haut  = numero_artiste_du_haut;
                        nombre_delements        = nombre_dartistes;
                        texte_contexte = "Artistes";
                    }
                    else if (contexte == 2) // si on arrive dans la liste d'albums
                    {
                        contexte                = contexte - 1;
                        numero_element_vise     = numero_album_vise;
                        numero_element_du_haut  = numero_album_du_haut;
                        nombre_delements        = nombre_dalbums;
                        charger_les_albums(numero_artiste_vise + numero_artiste_du_haut, nombre_dalbums, artists_path, album_path, artistes_noms, albums_noms);
                        nombre_delements = nombre_dalbums;
                        texte_contexte = artistes_noms[numero_artiste_vise + numero_artiste_du_haut] + "/Albums";
                    }
                }
                
                // RETURN = entrer
                if (event.key.keysym.scancode == SDL_SCANCODE_RETURN)
                {
                    if (contexte == 0) // quand on arrive dans la liste d'albums
                    {
                        contexte                = contexte + 1;
                        numero_element_vise     = 0;
                        numero_element_du_haut  = 0;
                        numero_album_vise       = 0;
                        charger_les_albums(numero_artiste_vise + numero_artiste_du_haut, nombre_dalbums, artists_path, album_path, artistes_noms, albums_noms);
                        nombre_delements = nombre_dalbums;
                        texte_contexte = artistes_noms[numero_artiste_vise + numero_artiste_du_haut] + "/Albums";
                    }
                    else if (contexte == 1) // quand on arrive dans la liste de morceaux
                    {
                        contexte                = contexte + 1;
                        numero_element_vise     = 0;
                        numero_element_du_haut  = 0;
                        numero_morceau_vise     = 0;
                        charger_les_morceaux(numero_album_vise + numero_album_du_haut, nombre_de_morceaux, artists_path, album_path, morceaux_path, artistes_noms, albums_noms, morceaux_noms);
                        nombre_delements = nombre_de_morceaux;
                        texte_contexte = artistes_noms[numero_artiste_vise + numero_artiste_du_haut] + "/" + albums_noms[numero_album_vise + numero_album_du_haut] + "/Morceaux";
                    }
                    else if (contexte == 2) // quand on fait enter sur une chanson
                    {
                        // On garde l'info sur ce qui joue :
                        numero_artiste_lecture     = numero_artiste_vise + numero_artiste_du_haut;
                        numero_album_lecture       = numero_album_vise + numero_album_du_haut;
                        numero_morceau_lecture     = numero_morceau_vise + numero_morceau_du_haut;
                        nombre_de_morceaux_lecture = nombre_de_morceaux;
                        nombre_de_morceaux_restants = nombre_de_morceaux_lecture - numero_morceau_lecture -1;
                        morceaux_path_lecture = morceaux_path;
                        // On enregistre les albums de l'artiste qui joue
                        for (int i=0;i<100;i++){albums_noms_lecture[i] = "";}
                        for (int i = numero_album_lecture;i<nombre_dalbums;i++){albums_noms_lecture[i] = albums_noms[i];}
                        // On enregistre les morceaux de l'album qui joue
                        for (int i=0;i<10000;i++){morceaux_noms_lecture[i] = "";}
                        for (int i = 0;i<nombre_de_morceaux_lecture;i++){morceaux_noms_lecture[i] = morceaux_noms[i];}
                        // On fait jouer la toune
                        quit_player(player_nom);
                        morceau_nom_lecture = morceaux_noms_lecture[numero_morceau_lecture];
                        //std::cout<<"\nMorceau_nom = "<<morceau_nom_lecture<<std::endl;
                        play_toune(player_nom, morceaux_path_lecture, morceau_nom_lecture);
                        morceau_duree_totale = duree_toune(morceaux_path_lecture, morceau_nom_lecture);
                        morceau_duree_restante = morceau_duree_totale;
                        afplay_actif = isRunning(player_nom); // afplay devrait être maintenant actif
                        chanson_actif = 1;
                        play = 1;
                        // On affiche l'info :
                        texte_info = std::string("[") + artistes_noms[numero_artiste_lecture] + "]~[" + albums_noms_lecture[numero_album_lecture]  + "]~[" + morceau_nom_lecture + "]";
                    }
                }
                
                // SPACE BAR = pour pause / play
                if (event.key.keysym.scancode == SDL_SCANCODE_SPACE)
                {
                    if (chanson_actif == 1)
                    {
                        if (play == 0) // si ça ne joue pas
                        {
                            resume_player(player_nom);
                            play = 1;
                        }
                        else if (play == 1) // si ça joue
                        {
                            pause_player(player_nom);
                            play = 0;
                        }
                    }
                    
                }
                
                // DOWN = descendre
                if (event.key.keysym.scancode == SDL_SCANCODE_DOWN)
                {
                    if ((numero_element_vise<nombre_delements_affiches-1) && (numero_element_vise < nombre_delements-1) )
                    {
                        numero_element_vise = numero_element_vise + 1;
                        if (contexte == 0){numero_artiste_vise = numero_element_vise;}
                        if (contexte == 1){numero_album_vise   = numero_element_vise;}
                        if (contexte == 2){numero_morceau_vise = numero_element_vise;}
                    }
                    else if (numero_element_du_haut<nombre_delements - nombre_delements_affiches)
                    {
                        numero_element_du_haut = numero_element_du_haut + 1;
                        if (contexte == 0){numero_artiste_du_haut = numero_element_du_haut;}
                        if (contexte == 1){numero_album_du_haut   = numero_element_du_haut;}
                        if (contexte == 2){numero_morceau_du_haut = numero_element_du_haut;}
                    }
                }
                
                // UP = monter
                if (event.key.keysym.scancode == SDL_SCANCODE_UP)
                {
                    if (numero_element_vise>0)
                    {
                        numero_element_vise = numero_element_vise - 1;
                        if (contexte == 0){numero_artiste_vise = numero_element_vise;}
                        if (contexte == 1){numero_album_vise   = numero_element_vise;}
                        if (contexte == 2){numero_morceau_vise = numero_element_vise;}
                    }
                    else if (numero_element_du_haut>0)
                    {
                        numero_element_du_haut = numero_element_du_haut - 1;
                        if (contexte == 0){numero_artiste_du_haut = numero_element_du_haut;}
                        if (contexte == 1){numero_album_du_haut   = numero_element_du_haut;}
                        if (contexte == 2){numero_morceau_du_haut = numero_element_du_haut;}
                    }
                }
                
                // RIGHT = next song
                if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT)
                {
                    afplay_actif = isRunning(player_nom);
                    if ( (chanson_actif == 1) && (afplay_actif == 1) && (nombre_de_morceaux_restants > 0) )
                    {
                        nombre_de_morceaux_restants = nombre_de_morceaux_restants - 1;
                        numero_morceau_lecture      = numero_morceau_lecture + 1;
                        // On fait jouer la toune
                        quit_player(player_nom);
                        morceau_nom_lecture = morceaux_noms_lecture[numero_morceau_lecture];
                        //std::cout<<"\nMorceau_nom = "<<morceau_nom_lecture<<std::endl;
                        play_toune(player_nom, morceaux_path_lecture, morceau_nom_lecture);
                        play = 1;
                        morceau_duree_totale = duree_toune(morceaux_path_lecture, morceau_nom_lecture);
                        morceau_duree_restante = morceau_duree_totale;
                        afplay_actif = isRunning(player_nom); // afplay devrait être maintenant actif
                        texte_info = std::string("[") + artistes_noms[numero_artiste_lecture] + "]~[" + albums_noms_lecture[numero_album_lecture]  + "]~[" + morceau_nom_lecture + "]";
                    }
                }
                
                // LEFT = previous song
                if (event.key.keysym.scancode == SDL_SCANCODE_LEFT)
                {
                    afplay_actif = isRunning(player_nom);
                    if ( (chanson_actif == 1) && (afplay_actif == 1) )
                    {
                        // on met la toune au début de la chanson
                        quit_player(player_nom);
                        play_toune(player_nom, morceaux_path_lecture, morceau_nom_lecture);
                        play = 1;
                        morceau_duree_totale = duree_toune(morceaux_path_lecture, morceau_nom_lecture);
                        morceau_duree_restante = morceau_duree_totale;
                        afplay_actif = isRunning(player_nom); // afplay devrait être maintenant actif
                        if ( (bouton_left_temps_restant > 0) && (numero_morceau_lecture > 0) )
                        {
                            nombre_de_morceaux_restants = nombre_de_morceaux_restants + 1;
                            numero_morceau_lecture      = numero_morceau_lecture - 1;
                            quit_player(player_nom);
                            morceau_nom_lecture = morceaux_noms_lecture[numero_morceau_lecture];
                            play_toune(player_nom, morceaux_path_lecture, morceau_nom_lecture);
                            morceau_duree_totale = duree_toune(morceaux_path_lecture, morceau_nom_lecture);
                            morceau_duree_restante = morceau_duree_totale;
                            afplay_actif = isRunning(player_nom); // afplay devrait être maintenant actif
                            texte_info = std::string("[") + artistes_noms[numero_artiste_lecture] + "]~[" + albums_noms_lecture[numero_album_lecture]  + "]~[" + morceau_nom_lecture + "]";
                        }
                        bouton_left_temps_restant = 500; // 500 ms
                    }
                }
                
                // not-CMD
                if (!state[SDL_SCANCODE_LGUI] && !state[SDL_SCANCODE_RGUI])
                {
                    char Character = ToucheVersChar(event.key.keysym.scancode);
                    int Character_Int = (int)Character - 65;
                    if ((-1<Character_Int) && (Character_Int<26))
                    {
                        if (contexte == 0)
                        {
                            if (position_des_lettres[Character_Int] < nombre_delements - nombre_delements_affiches)
                            {
                                // on change les numéros du haut (relativement au menu principal)
                                numero_artiste_du_haut  = position_des_lettres[Character_Int];
                                numero_album_du_haut    = 0;
                                numero_morceau_du_haut  = 0;
                                numero_element_du_haut  = position_des_lettres[Character_Int];
                                // On change les numéros visés (relativement à où on va être)
                                numero_artiste_vise     = 0;
                                numero_album_vise       = 0;
                                numero_morceau_vise     = 0;
                                numero_element_vise     = 0;
                            }
                            else
                            {
                                // on change les numéros du haut (relativement au menu principal)
                                numero_artiste_du_haut  = nombre_delements - nombre_delements_affiches;
                                numero_album_du_haut    = 0;
                                numero_morceau_du_haut  = 0;
                                numero_element_du_haut  = nombre_delements - nombre_delements_affiches;
                                // On change les numéros visés (relativement à où on va être)
                                numero_artiste_vise     = position_des_lettres[Character_Int] - numero_artiste_du_haut;
                                numero_album_vise       = 0;
                                numero_morceau_vise     = 0;
                                numero_element_vise     = position_des_lettres[Character_Int] - numero_artiste_du_haut;
                            }
                        }
                    }
                }
                
                // CMD
                if (state[SDL_SCANCODE_LGUI] || state[SDL_SCANCODE_RGUI])
                {
                    // CMD + L = on va à l'artiste en lecture
                    if (state[SDL_SCANCODE_L])
                    {
                        if(chanson_actif == 1)
                        {
                            contexte = 2;
                            // on change les numéros du haut (relativement au menu principal)
                            numero_artiste_du_haut  = numero_artiste_lecture;
                            numero_album_du_haut    = numero_album_lecture;
                            numero_morceau_du_haut  = 0;
                            numero_element_du_haut  = 0;
                            // On change les numéros visés (relativement à où on va être)
                            numero_artiste_vise     = 0;
                            numero_album_vise       = 0;
                            numero_morceau_vise     = 0;
                            numero_element_vise     = 0;
                            // On change le numéro de l'élément visé
                            nombre_delements        = nombre_de_morceaux_lecture;
                            nombre_de_morceaux      = nombre_de_morceaux_lecture;
                            // On réinitialise le nom des morceaux
                            for (int i=0;i<100;i++){albums_noms[i] = "";}
                            for (int i = numero_album_lecture;i<nombre_dalbums;i++){albums_noms[i] = albums_noms_lecture[i];}
                            // On réinitialise le nom des morceaux
                            for (int i=0;i<10000;i++){morceaux_noms[i] = "";}
                            for (int i = 0;i<nombre_de_morceaux_lecture;i++){morceaux_noms[i] = morceaux_noms_lecture[i];}
                            texte_contexte = artistes_noms[numero_artiste_vise + numero_artiste_du_haut] + "/" + albums_noms[numero_album_vise + numero_album_du_haut] + "/Morceaux";
                        }
                    }
                }
            }            
        }
        
        currentTime = SDL_GetTicks(); // on prend l'heure pour le taux d'affichage
        if(currentTime > lastTime + delay) // pause avant l'affichage en millisecondes (16 = optimal car écran à 60Hz, sinon 100 donne truc intéressant)
        {
            if ( (play == 1) && (afplay_actif == 1) && (chanson_actif == 1))
            {
                // Pour le temps restant de la chanson qui joue
                morceau_duree_restante = morceau_duree_restante - (currentTime - lastTime)/1000.0;
                morceau_duree_restante_texte = std::to_string(morceau_duree_restante);
                morceau_duree_restante_texte = morceau_duree_restante_texte.substr(0,morceau_duree_restante_texte.size() - 3);
                ViderTexture(texture_textbox_duree_restante);
                AjouterBoutonAvecTexte(texture_textbox_duree_restante,morceau_duree_restante_texte, position_textbox_duree_restante_interieur,fonte_texte,couleur_texte_duree_restante,couleur_texte_box_duree_restante);
                // Pour le bouton left
                if (bouton_left_temps_restant > 0)
                {
                    bouton_left_temps_restant = bouton_left_temps_restant - delay;
                }
                else
                {
                    bouton_left_temps_restant = 0;
                }
                
            }
            lastTime = currentTime;
        }
        
        if ( (morceau_duree_restante < 0) && (chanson_actif == 1) && (play == 1) )
        {
            event_activation = 1;
            if (nombre_de_morceaux_restants > 0)
            {
                nombre_de_morceaux_restants = nombre_de_morceaux_restants - 1;
                numero_morceau_lecture      = numero_morceau_lecture + 1;
                // On fait jouer la toune
                quit_player(player_nom);
                morceau_nom_lecture = morceaux_noms_lecture[numero_morceau_lecture];
                //std::cout<<"\nMorceau_nom = "<<morceau_nom_lecture<<std::endl;
                play_toune(player_nom, morceaux_path_lecture, morceau_nom_lecture);
                morceau_duree_totale = duree_toune(morceaux_path_lecture, morceau_nom_lecture);
                morceau_duree_restante = morceau_duree_totale;
                afplay_actif = isRunning(player_nom); // afplay devrait être maintenant actif
                texte_info = std::string("[") + artistes_noms[numero_artiste_lecture] + "]~[" + albums_noms_lecture[numero_album_lecture]  + "]~[" + morceau_nom_lecture + "]";
            }
            else
            {
                chanson_actif = 0;
                play = 0;
                quit_player(player_nom);
                afplay_actif = isRunning(player_nom);
                texte_info = "";
            }
        }
        
        // Si on a activé un truc on met à jour les textures
        if (event_activation == 1)
        {
            ViderTexture(texture_textbox_titre);
            ViderTexture(texture_textbox_elements);
            // Texture titre
            AjouterBoutonAvecTexte(texture_textbox_titre,texte_contexte, position_textbox_contexte,fonte_texte,couleur_texte_contexte,couleur_texte_box_contexte);
            AjouterBoutonAvecTexte(texture_textbox_titre,texte_info, position_textbox_info,fonte_texte,couleur_texte_info,couleur_texte_box_info);
            // Flèches
            if (numero_element_du_haut > 0) // mettre si il y a des éléments non affichés
            {
                AjouterBoutonAvecTexte(texture_textbox_titre,texte_fleche_haut, position_textbox_fleche_haut_droit,fonte_fleche,couleur_texte_fleche,couleur_texte_box_fleche);
                AjouterBoutonAvecTexte(texture_textbox_titre,texte_fleche_haut, position_textbox_fleche_haut_gauche,fonte_fleche,couleur_texte_fleche,couleur_texte_box_fleche);
            }
            if (numero_element_du_haut + nombre_delements_affiches < nombre_delements) // mettre si il y a des éléments non affichés
            {
                AjouterBoutonAvecTexte(texture_textbox_titre,texte_fleche_bas, position_textbox_fleche_bas_droit,fonte_fleche,couleur_texte_fleche,couleur_texte_box_fleche);
                AjouterBoutonAvecTexte(texture_textbox_titre,texte_fleche_bas, position_textbox_fleche_bas_gauche,fonte_fleche,couleur_texte_fleche,couleur_texte_box_fleche);
            }
            // Texture éléments
            position_viseur.y = (int) numero_element_vise * (position_textbox_elements.h /nombre_delements_affiches);
            for (int i=0;i<nombre_delements_affiches;i++)
            {
                position_texte.y = (int) i * (position_textbox_elements.h /nombre_delements_affiches);
                if (contexte == 0){texte_elements = artistes_noms[i + numero_element_du_haut];}
                if (contexte == 1){texte_elements = albums_noms[  i + numero_element_du_haut];}
                if (contexte == 2){texte_elements = morceaux_noms[i + numero_element_du_haut];}
                if ( i != numero_element_vise ){AjouterBoutonAvecTexte(texture_textbox_elements,texte_elements, position_texte,fonte_texte,couleur_texte_elements,couleur_texte_box_elements);}
                else
                {
                    AjouterBoutonAvecTexte(texture_textbox_elements,texte_elements, position_viseur,fonte_texte,couleur_viseur_texte,couleur_viseur_box);
                }
            }
            if (chanson_actif == 1)
            {
                if (contexte == 0)
                {
                    // mettre l'indicateur
                    position_indicateur.y = (int)(numero_artiste_lecture - numero_artiste_du_haut)  * (position_textbox_elements.h /nombre_delements_affiches);
                    AjouterBoutonAvecTexte(texture_textbox_elements,texte_indicateur,  position_indicateur,fonte_symboles,couleur_texte_indicateur,couleur_texte_box_indicateur);
                }
                if ( (contexte == 1) && (numero_artiste_du_haut + numero_artiste_vise == numero_artiste_lecture) )
                {
                    // mettre l'indicateur
                    position_indicateur.y = (int)(numero_album_lecture - numero_album_du_haut)  * (position_textbox_elements.h /nombre_delements_affiches);
                    AjouterBoutonAvecTexte(texture_textbox_elements,texte_indicateur,  position_indicateur,fonte_symboles,couleur_texte_indicateur,couleur_texte_box_indicateur);
                }
                if ( (contexte == 2) && (numero_artiste_du_haut + numero_artiste_vise == numero_artiste_lecture) && (numero_album_du_haut + numero_album_vise == numero_album_lecture) )
                {
                    // mettre l'indicateur
                    position_indicateur.y = (int)(numero_morceau_lecture - numero_morceau_du_haut)  * (position_textbox_elements.h /nombre_delements_affiches);
                    AjouterBoutonAvecTexte(texture_textbox_elements,texte_indicateur,  position_indicateur,fonte_symboles,couleur_texte_indicateur,couleur_texte_box_indicateur);
                }
            }
            event_activation = 0;
        }
                
        // On met les textures dans le renderer
        SDL_RenderClear(renderer);  // on claire le renderer
        SDL_RenderCopy(renderer, texture_textbox_titre, NULL, NULL);
        SDL_RenderCopy(renderer, texture_textbox_elements, NULL, &position_textbox_elements);
        if (chanson_actif == 1)
        {
            SDL_RenderCopy(renderer, texture_textbox_duree_restante, NULL, &position_textbox_duree_restante);
        }
        SDL_RenderPresent(renderer); // on affiche le contenu du renderer à l'écran  
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////       On termine le programme      ///////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Bye bye
    std::cout<<"\n\nAu revoir !\n\n";
    // On termine le programme (vider la mémoire vive utilisée)
    SDL_DestroyRenderer(renderer);renderer = NULL; // on détruit le renderer_background
    SDL_DestroyWindow(fenetre);fenetre = NULL; // On ferme la fenêtre
    SDL_DestroyTexture(texture_textbox_titre);texture_textbox_titre = NULL; // on détruit la texture_textbox_titre
    SDL_DestroyTexture(texture_textbox_elements);texture_textbox_elements = NULL; // on détruit la texture_textbox_titre
    SDL_DestroyTexture(texture_textbox_duree_restante);texture_textbox_duree_restante = NULL; // on détruit la texture_textbox_duree_restante
    TTF_CloseFont(fonte_texte);fonte_texte=NULL; // on vide la mémoire de la fonte texte
    TTF_CloseFont(fonte_symboles);fonte_symboles=NULL; // on vide la mémoire de la fonte symboles
    TTF_CloseFont(fonte_fleche);fonte_fleche=NULL; // on vide la mémoire de la fonte_fleche
    SDL_Quit(); // On quitte SDL
    TTF_Quit(); // On quitte TTF
    
    return 0;
}