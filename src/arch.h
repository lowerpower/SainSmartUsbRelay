#ifndef __ARCH_H__
#define __ARCH_H__
/*! \file arch.h
    \brief Architecture specific code
*/

#include	"mytypes.h"



/*! \fn U32 second_count(void);

    \brief returns the current second count								
	
	\return second count.
*/
U32 second_count(void);

/*! \fn  U16 hund_ms_count(void);

    \brief returns a counter in incerment of 100ms per inc								
	
	\return second count.
*/
U16	hund_ms_count(void);

/*! \fn  U16 hund_ms_count(void);
 *
 *  \brief returns a counter in incerment of 100ms per inc                              
 *         
 *  \return second count.
 *             
 */
U32 ms_count(void);


/*! \fn void ysleep_seconds(U16 duration);

    \brief Platform independent sleep in seconds								
	
	\param U16 duration.

*/
void ysleep_seconds(U16 duration);

void ysleep_usec(U32 duration);

void threadswitch(void);


/*! \fn long file_length(char *filename);

    \brief Get the filelength of a file								
	
	\param char * filename.

*/
long file_length(char *filename);


/*! \fn void yrand_seed(long seed)

    \brief set the seed for random number generator							
	
	\param long seed

*/
void yrand_seed(long seed);

/*! \fn int	yrand(int max);

    \brief gets a random number							
	
	\param maximum random number

*/
int yrand(int max);


//
void UID_Extract(U8 *uid,U8 *uid_ascii);


int  isDirectoryNotEmpty(char *dirname);
int DeleteDirectroyFiles(char *dirname);


// converts hexascii string to binary represntation.  delemiters ok IE AB:CD:01 is the same as ABCD01
int hexascii_2_bin(U8 *dest, int maxlen, char *in);


void bin2hexstr(char *buf, char *str, int len);
void hexstr2bin(char *str, char *buf, int len);
U8 hex2bin(U8 c);

// same as bin2hexstr except put : inbetween bytes
void bin2hexstrcol(char *buf, char *str, int len);
// same as above but with buffer checking
void bin2hexstrlcol(char *buf, int len, char *str, int strlen);

void trim(char * s);
void strip_crlf(char *d);
void strip_slash(char *d);
void strtolower(char *string);
size_t snprintfcat(char* buf,size_t bufSize, char const* fmt,...);

int str_char_replace(char *s, const char orig_char, const char replace_char);

int my_strlen_utf8_c(char *s);

int readln_from_a_file(FILE *fp, char *line, int size);
char *readln_from_a_buffer(char* buffer, char *line, int size);

#if defined(LINUX) || defined(MACOSX)
int kbhit(void);
#endif

#endif

