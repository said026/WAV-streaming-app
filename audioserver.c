#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <dirent.h>
#include "audio.h"
#include "contient.h"
#include "filters.h"

void die(char *s) {
    perror(s);
    exit(1);
}
 
int main(int argc, char *argv[]) {
    struct sockaddr_in addr_server, addr_client;
    int socket_server;
    int slen = sizeof(addr_server);
   	struct Firstmessage message;  
	struct WavInfos wav_infos;
	struct WavData data;
	struct Ack ack;
	int timeoutcounter = 0;
	int sample_size, sample_rate, channels;  
	char *filename;
	char *filter;
    int fd_wav;
	
	if (argc!=1) {
        printf("Le programme ne prends pas d'arguments en parametre");
        return 1;
    }
     
    // creation du socket pour le server
    if ((socket_server=socket(AF_INET, SOCK_DGRAM, 0)) == -1)  {
        die("socket");
    }
     
    // construction de l'adresse du server 
    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(PORT);
    addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
     
	// on lie le socket au port
    if( bind(socket_server , (struct sockaddr*) &addr_server, sizeof(addr_server) ) == -1) {
        die("bind");
    }
    printf("----------------------------\n");
	printf("Serveur Streaming Audio SYR2\n");
	printf("----------------------------\n");
	printf("En attente de requetes...\n");
		
	// Reception du premier paquet  

	if ((recvfrom(socket_server, &message, sizeof(struct Firstmessage), 0, (struct sockaddr*) &addr_client, (socklen_t *) &slen))<0) {
		die("Erreur lors de la reception de la demande du client");
	}
		
	filename = message.filename;
	filter = message.filter;

	// Ouverture du fichier .wav
	if ((fd_wav = aud_readinit (filename, &sample_rate, &sample_size, &channels)) < 0) {
		die("Erreur lors de l'ouverture du fichier ");
	}
		
	printf("Demande du fichier : %s \n", filename);
		
	/* On teste si le fichier a bien été trouvé:
	 * 0: Aucune erreur 
	 * 1: Fichier introuvable
	 * 2: Filtre introuvable
	 */


	// Verifications pour le fichier 

	if(fd_wav < 0) {
        printf("Le fichier %s est introuvable\n",filename);
        wav_infos.errorcode=1;
        if ((sendto(socket_server, &wav_infos, sizeof(struct WavInfos), 0, (struct sockaddr*) &addr_client, slen)) < 0) {
			die("Impossible d'envoyer la réponse au client");
		}
    }
    
    // Verifications pour le filtre 
    
    if(strcmp(filter,"")==0){
        printf("Pas de filtre demandé\n");
    } else if(strcmp(filter,"reverse")==0){
		printf("Filtre reverse selectioné\n");
	} else if(strcmp(filter,"bip")==0) {
		printf("Filtre beep selectioné\n");
	} else if(strcmp(filter,"mono")==0) {
		printf("Filtre mono selectioné\n");
	} else {
		wav_infos.errorcode=2;
		if ((sendto(socket_server, &wav_infos, sizeof(struct WavInfos), 0, (struct sockaddr*) &addr_client, slen)) < 0) {
			die("Impossible d'envoyer la réponse au client");
		}
        printf("Le filte %s n'est pas pris en charge \n", filter);
        return 1;
	}
		
	
	printf("Succes de l'ouverture de %s\n",filename);
	
// On prépare la réponse avec les informations necessaires
	
	// Si le filtre mono a été demandé on l'applique directement
	if(strcmp(filter,"mono")==0) {
		wav_infos.channels = 1;
	} else {
		wav_infos.channels = channels;
		 wav_infos.sample_rate = sample_rate;
	}
	
    wav_infos.sample_rate = sample_rate;
    wav_infos.sample_size = sample_size;
    wav_infos.errorcode = 0;

	// Envoi des informations concernant le .wav
    if ((sendto(socket_server, &wav_infos, sizeof(struct WavInfos), 0, (struct sockaddr*) &addr_client, slen)) < 0) {
		die("Impossible d'envoyer la réponse au client");
	}
	
	// On attend que le client soit pret pour la reception
	
	if ((recvfrom(socket_server, &ack, sizeof(struct Ack), 0, (struct sockaddr*) &addr_client, (socklen_t *) &slen))<0) {
			die("Erreur lors de la demande de début d'envoi");
	}
		
	int ret = 0;
	fd_set read_set;
	struct timeval timeout;
	
		
	int bytesread;
    data.endflag = 0;
    bytesread = read(fd_wav, data.buf, sizeof(data.buf));
        
        while (bytesread > 0){
            
			// Initialisation du timeout
			
			timeout.tv_sec = TIMEOUT;
			timeout.tv_usec = 0;
			
			FD_ZERO(&read_set);
			FD_SET(socket_server, &read_set);
            
			// On applique le filtre au buffer courant (si demandé)
			if(strcmp(filter,"reverse")==0){
				reverse_filter(data.buf);
			} else if(strcmp(filter,"bip")==0) {
				bip_filter(data.buf);
			}
			
            // On recupere le nombre d'octets lus
            data.length = bytesread;
            if ((sendto(socket_server, &data, sizeof(struct WavData), 0, (struct sockaddr*) &addr_client, slen))<0) { 
                die("Erreur lors de l'envoi du buffer");
            }

            // Attente de l'aquittement, renvoi si le timeout est atteint
            ret = select(socket_server + 1, &read_set, NULL, NULL, &timeout);
            if (ret<0) {
                die("select()");
            }
            if (ret==0) {
                // Si le nombre de timeouts a atteint le maximum 
                if (timeoutcounter==RESENDS) {
                    printf("Le client ne répond plus, arret du transfert\n");
                    return 0; 
                }
                // Renvoi du paquet au client
                timeoutcounter+=1;
                printf("Aucune réponse de la part du client, renvoi numero %d\n", timeoutcounter);
            }
            if(FD_ISSET(socket_server, &read_set)) {
                timeoutcounter=0;
                // Attente de l'aquittement
                if ((recvfrom(socket_server, &ack, sizeof(struct Ack), 0, (struct sockaddr*) &addr_client, (socklen_t *) &slen))<0) {
                    die("Erreur reception paquet d'aquittement"); 
                }
                // Si le dernier paquet envoyé n'a pas été recu par le client 
                if (ack.flag == 0) {
                    printf("Renvoi du dernier message");
                    // le server revient a la boucle while et renvoi le paquet
                } else { 
                    bytesread = read(fd_wav, data.buf, sizeof(data.buf));
                }
			} 
		}
     // Fin de fichier atteinte
     data.endflag = 1;
     if ((sendto(socket_server, &data, sizeof(struct WavData), 0, (struct sockaddr*) &addr_client, slen))) {
		die("Impossible d'envoyer le paquet de fin de fichier");
     }

    // Fermeture de la connexion	
    close(socket_server);
    close(fd_wav);
    return 0;
    
}

