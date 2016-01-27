#include "filters.h"
#include "contient.h"

/*
 * Filtre qui inverse le buffer 
 */
  
void reverse_filter(char* buffer) {
	int i;
    char rev[BUFLEN];
    // On calcule l'inverse et on le met dans rev
    for(i=0; i<BUFLEN; i++) {
		rev[i] = buffer[BUFLEN-i-1];
    }
    buffer = rev;
}

/*
 * Ajoute un bip en un interval de temps régulier
 */
  
void bip_filter(char* buffer) {
    int i;
    for(i=0;i<BUFLEN;i+=5) {
		buffer[i] = 100;
    }
} 


 
