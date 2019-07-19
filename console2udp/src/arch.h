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




/*! \fn void ysleep_seconds(U16 duration);

    \brief Platform independent sleep in seconds								
	
	\param U16 duration.

*/
void ysleep_seconds(U16 duration);

void ysleep_usec(U32 duration);

/*! \fn long file_length(char *filename);

    \brief Get the filelength of a file								
	
	\param char * filename.

*/
long long file_length(char *filename);


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


void UID_Extract(U8 *uid,U8 *uid_ascii);

int readln_from_a_file(FILE *fp, char *line, int size);

int isDirectoryNotEmpty(char *dirname) ;
int DeleteDirectroyFiles(char *dirname);







void trim(char * s);
void strtolower(char *string);
int  str_char_replace(char *s, const char orig_char, const char replace_char);
char *strtok_y(char *str, const char *sep, char **lasts);
void *memcasemem(void *haystack, size_t haystack_len,void *needle, size_t needle_len);

int strip_slash(char *buffer);
void strip_crlf(char *d);





#if defined(WIN32)
char *strcasestr(const char *haystack, const char *needle);
struct tm *gmtime_r(time_t *_clock, struct tm *_result);
struct tm *localtime_r(time_t *_clock, struct tm *_result);

#endif

#endif

