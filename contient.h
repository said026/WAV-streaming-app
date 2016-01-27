#ifndef _CONTIENT_ 
#define _CONTIENT_ 

#define BUFLEN 1024  // Le dernier octet determine la fin de fichier (vaut '1' dans ce cas la )
#define PORT 9930     // Le port d'ecoute
#define NB_MAX 100    // Nombre de fichier .wav maximum dans le repertoire local
#define LENGTH 30
#define SERVER "127.0.0.1"
#define TIMEOUT 2
#define RESENDS 3 // nombre maximum de demandes de renvoi


struct Firstmessage {
    char filename[LENGTH];
    char filter[LENGTH];
};

struct WavInfos {
	int channels, sample_size, sample_rate, errorcode; 
};

struct WavData {
    char buf[BUFLEN];
    int length, endflag;
};

struct Ack {
    int flag;
};


#endif /* _CONTIENT_ */
