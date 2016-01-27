#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "audio.h"
#include "contient.h"
#include "filters.h"

void die(char *s) {
    perror(s);
    exit(1);
}

int main(int argc, char *argv[]) {
    struct Firstmessage message;
    struct WavData data;
    struct WavInfos wav_infos; 
    struct Ack ack;
    int timeoutcounter = 0;
    int sample_size, sample_rate, channels;    
	int socket_client, slen;
	struct sockaddr_in addr_server;
    slen = sizeof(addr_server);
    int fd_device;
    
    if (argc < 3) {
		printf("Erreur: nombre de parametres incorrect\n");
		printf("Utilisation : audioclient server_host_name file_name filter_name [filter_parameter]\n"); 
		return 1;
	}
    
	// Creation de la socket pour le client
	if ((socket_client = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		die("Erreur lors de la création de la socket");
	}
	
	// On construit l'adresse de destination

	addr_server.sin_family = AF_INET;
	addr_server.sin_port = htons(PORT);

	// On recupere l'adresse du server (locale)
	if (inet_aton(argv[1] , &addr_server.sin_addr) == 0) {
		die("inet_aton()");
    }

	// Recuperation du nom de fichier

    strncpy(message.filename, argv[2], strlen(argv[2]));
    message.filename[strlen(argv[2])] = '\0';
    
	// Si un filtre a été entré en parametre 

	if (argv[3]) {
		strncpy(message.filter, argv[3], strlen(argv[3]));
		message.filter[strlen(argv[3])] = '\0';
	} else {
		strncpy(message.filter, '\0', 0);
	}
	
    
	// Envoi du nom de fichier et du filtre 
   
	if ((sendto(socket_client, &message, sizeof(struct Firstmessage), 0, (struct sockaddr*) &addr_server , slen)) < 0){
			die("Erreur lors de l'envoi du nom de fichier");
	}
			
	// Reception de la réponse du server
		
	if ((recvfrom(socket_client, &wav_infos, sizeof(struct WavInfos), 0, (struct sockaddr*) &addr_server, (socklen_t *) &slen)) < 0) {
			die("Erreur lors de la reception des informations sur le fichier .WAV");
	}
	
	
	// Envoi de la requete de début d'envoi 	

	ack.flag =1;
	
	if ((sendto(socket_client, &ack, sizeof(struct Ack), 0, (struct sockaddr*) &addr_server , slen)) < 0){
		die("Impossible d'envoyer la requete de début d'envoi");
	}
	
	
	/* Test du code d'erreur :
	 * 0: aucune erreur; l'envoi va commencer
	 * 1: fichier introuvable
	 * 2: filtre introuvable */
 
    if (wav_infos.errorcode == 1) {
        printf("Erreur: le fichier demandé n'existe pas\n");
        return 1;
    }
    
    if (wav_infos.errorcode == 2) {
        printf("Erreur: le filtre demandé n'existe pas\n");
        return 1;
    }

	// Sinon on récupere les informations sur le fichier .WAV

    sample_size = wav_infos.sample_size;
    sample_rate = wav_infos.sample_rate;
    channels = wav_infos.channels;	

	// Ouverture du device audio
	if ((fd_device = aud_writeinit(sample_rate, sample_size, channels)) < 0) {
		die("Impossible d'ouvrir le device audio");
	}

	// On établit un timeout de 0.5 secondes 
 	
 	int ret = 0;
	fd_set read_set;
	struct timeval timeout;
	
	
	while(1) { // Tant que la fin de fichier n'est pas atteinte 
		
		// Initialisation ou remise a zero du timeout
		timeout.tv_sec = TIMEOUT;
		timeout.tv_usec = 0;
		
		FD_ZERO(&read_set);
		FD_SET(socket_client, &read_set);
		
		ret = select(socket_client + 1, &read_set, NULL, NULL, &timeout);
		
		// Attente de la reception d'un paquet WavData, si le timeout est atteint on redemande le paquet
		if (ret<0) {
				die("select()");
			}
			
		if (ret == 0 ) {  
			// Si le nombre de timeouts a atteint le maximum 
			if (timeoutcounter==RESENDS) {
				printf("Le serveur ne répond plus, arret du transfert\n");
                return 0;
            }
            // On redemande le paquet au server
            timeoutcounter++;
            printf("Aucun paquet recu, essai numero %d\n", timeoutcounter);
            ack.flag = 0; 
            if ((sendto (socket_client, &ack, sizeof(struct Ack), 0, (struct sockaddr*) &addr_server , slen))<0) {
				die("Erreur lors de l'envoi de l'acquittement de renvoi au server");
            }
		}
            
		if (FD_ISSET(socket_client, &read_set)) { // les données sont disponibles sur la socket 
                timeoutcounter=0; 
                // Reception d'un paquet WavData
                if ((recvfrom(socket_client, &data, sizeof(struct WavData), 0, (struct sockaddr*) &addr_server, (socklen_t *) &slen)) <0) {
					die("Erreur reception paquet de données");
				}

               // Envoi du paquet d'aquittement
                ack.flag = 1;
                if ((sendto(socket_client, &ack, sizeof(struct Ack), 0, (struct sockaddr*) &addr_server, sizeof(struct sockaddr_in))) < 0) {
                    die("Erreur lors de l'envoi de l'acquittement de bonne reception au server");
                }

                
                // Verification de la fin de fichier
                if (data.endflag == 1) {
                    printf("La fin de fichier a été atteinte, Fermeture de la connexion\n");
                    close(socket_client);
                    close(fd_device);
                    return 0;
                }
                write(fd_device, data.buf, data.length);
		}
	}
	close(socket_client);
	close(fd_device);
	return 0;
}
