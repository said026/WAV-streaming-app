/* audio.[ch]
 *
 * part of the Systems Programming 2005 assignment
 * (c) Vrije Universiteit Amsterdam, BSD License applies
 * contact info : wdb -_at-_ few.vu.nl
 * */

/** open a WAV-file for reading
 *
 * this function checks whether the file pointed to by filename exists,
 * and if it is of the right type. Note that not all WAV filetypes are
 * supported. Only the simplest uncompressed PCM streams can be read.
 *
 * the function writes metadata about the opened file in the integers pointed
 * to by the parameters.
 *
 * @param filename	a non-NULL pointer to a string denoting a local file
 * @param sample_rate	the number of samples per second, e.g., 44kHz
 * @param sample_size	the precision of the wave-approximations, e.g., 16bit
 * @param channels	the number of channels. No THX or dobly, we only accept mono and stereo
 *
 * @return 0 on success, <0 on failure
 */
int aud_readinit (char *filename, int *sample_rate, int*sample_size, int* channels );

/** write an uncompressed PCM stream to the speaker
 *
 * this function opens the speaker device as a regular file. You can use the returned
 * file descriptor as if it was a normal file.
 *
 * @params sample_rate, sample_size, channels: see aud_readinit above
 * @return a new descriptor or <0 if an error occurred. NB: 0 itself is a valid descriptor number!
 */
int aud_writeinit (int sample_rate, int sample_size, int channels); 

