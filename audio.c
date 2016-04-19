/* audio.[ch]
 *
 * part of the Systems Programming 2005 assignment
 * (c) Vrije Universiteit Amsterdam, BSD License applies
 * contact info : wdb -_at-_ few.vu.nl
 * */

/* To make audio playback work on modern Linux systems:
   - Start your audio client with "padsp audioclient" instead of just "audioclient"
   - Or set $LD_PRELOAD to libpulsedsp.so
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define AUDIODEV	"/dev/dsp"
#define swap_short(x)		(x)
#define swap_long(x)		(x)

#include <stdint.h>
#include <sys/soundcard.h>
#include "audio.h"

/* Definitions for the WAVE format */

#define RIFF		"RIFF"	
#define WAVEFMT		"WAVEfmt"
#define DATA		0x61746164
#define PCM_CODE	1

typedef struct _waveheader {
  char		main_chunk[4];	/* 'RIFF' */
  uint32_t	length;		/* filelen */
  char		chunk_type[7];	/* 'WAVEfmt' */
  uint32_t	sc_len;		/* length of sub_chunk, =16 */
  uint16_t	format;		/* should be 1 for PCM-code */
  uint16_t	chans;		/* 1 Mono, 2 Stereo */
  uint32_t	sample_fq;	/* frequence of sample */
  uint32_t	byte_p_sec;
  uint16_t	byte_p_spl;	/* samplesize; 1 or 2 bytes */
  uint16_t	bit_p_spl;	/* 8, 12 or 16 bit */ 

  uint32_t	data_chunk;	/* 'data' */
  uint32_t	data_length;	/* samplecount */
} WaveHeader;

int aud_readinit (char *filename, int *sample_rate, 
		  int *sample_size, int *channels ) 
{
  /* Sets up a descriptor to read from a wave (RIFF). 
   * Returns file descriptor if successful*/
  int fd;
  WaveHeader wh;
		  
  if (0 > (fd = open (filename, O_RDONLY))){
    fprintf(stderr,"unable to open the audiofile\n");
    return -1;
  }

  read (fd, &wh, sizeof(wh));

  if (0 != bcmp(wh.main_chunk, RIFF, sizeof(wh.main_chunk)) || 
      0 != bcmp(wh.chunk_type, WAVEFMT, sizeof(wh.chunk_type)) ) {
    fprintf (stderr, "not a WAVE-file\n");
    errno = 3;// EFTYPE;
    return -1;
  }
  if (swap_short(wh.format) != PCM_CODE) {
    fprintf (stderr, "can't play non PCM WAVE-files\n");
    errno = 5;//EFTYPE;
    return -1;
  }
  if (swap_short(wh.chans) > 2) {
    fprintf (stderr, "can't play WAVE-files with %d tracks\n", wh.chans);
    return -1;
  }
	
  *sample_rate = (unsigned int) swap_long(wh.sample_fq);
  *sample_size = (unsigned int) swap_short(wh.bit_p_spl);
  *channels = (unsigned int) swap_short(wh.chans);	
	
  fprintf (stderr, "%s chan=%u, freq=%u bitrate=%u format=%hu\n", 
	   filename, *channels, *sample_rate, *sample_size, wh.format);
  return fd;
}

int aud_writeinit (int sample_rate, int sample_size, int channels) 
{
  /* Sets up the audio device params. 
   * Returns device file descriptor if successful*/
  int audio_fd, error;
  char *devicename;

  printf("requested chans=%d, sample rate=%d sample size=%d\n", 
	 channels, sample_rate, sample_size);
  
  if (NULL == (devicename = getenv("AUDIODEV")))
    devicename = AUDIODEV;
    	
  if ((audio_fd = open (devicename, O_WRONLY, 0)) < 0) {
    perror ("setparams : open ") ;
    fprintf(stderr,"\n*******************************************************\n*** AVEZ-VOUS LANCE VOTRE PROGRAMME AVEC padsp?     ***\n*** PAR EXAMPLE AU LIEU DE LANCER ./client test.wav ***\n*** IL FAUT LANCER padsp ./client test.wav          ***\n*******************************************************\n\n");
    return -1;
  } 
  if (ioctl (audio_fd, SNDCTL_DSP_RESET, 0) != 0) {
    perror ("setparams : reset ") ;
    close(audio_fd);
    return -1;	
  } 
	
  if ((error = ioctl (audio_fd, SNDCTL_DSP_SAMPLESIZE, &sample_size)) != 0) {
    perror ("setparams : bitwidth ") ;
    close(audio_fd);
    return -1;
  } 

  if (ioctl (audio_fd, SNDCTL_DSP_CHANNELS, &channels) != 0) {
    perror ("setparams : channels ") ;
    close(audio_fd);
    return -1;
  } 

  if ((error = ioctl (audio_fd, SNDCTL_DSP_SPEED, &sample_rate)) != 0) {
    perror ("setparams : sample rate ") ;
    close(audio_fd);
    return -1;
  } 

  if ((error = ioctl (audio_fd, SNDCTL_DSP_SYNC, 0)) != 0) {
    perror ("setparams : sync ") ;
    close(audio_fd);
    return -1;
  } 
  printf("set chans=%d, sample rate=%d sample size=%d\n",
	 channels,sample_rate, sample_size);
  return audio_fd;
}



