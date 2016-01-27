# Makefile PROJET SYR2

all: audioclient audioserver

audioclient: audioclient.o audio.o	
	gcc -o audioclient audioclient.o audio.o
	
audioserver: audioserver.o filters.o audio.o	
	gcc -o audioserver audioserver.o audio.o filters.o	
	
audioserver.o: audioserver.c audio.h filters.h 
	gcc -c -Wall audioserver.c audio.c filters.c

audioclient.o: audioclient.c audio.h 
	gcc -c -Wall audioclient.c audio.c
	
audio.o: audio.c audio.h
	gcc -c -Wall audio.c	

filters.o: filters.c filters.h 
	gcc -c -Wall filters.c	
	
depend:
	makedepend audioclient.c audioserver.c

clean:
	rm -rf *.o

