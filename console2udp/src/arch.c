/*!																www.yoics.com			
 *---------------------------------------------------------------------------
 *! \file arch.c
 *  \brief Platform dependent code for yoics system, contains abstaracted 
 *		patform independent interface interface.
 *																			
 *---------------------------------------------------------------------------
 * Version                                                                  -
 *		0.1 Original Version June 3, 2006									-        
 *
 *---------------------------------------------------------------------------    
 * Version                                                                  -
 * 0.1 Original Version August 31, 2006     							    -
 *																			-
 * (c)2006 Yoics Inc. All Rights Reserved									-
 *---------------------------------------------------------------------------
 *
 */

#include	"config.h"
#include	"mytypes.h"
#include	"arch.h"
#include	"debug.h"

static int		yrandom_init=0;

#if defined(WIN32) || defined(WINCE)
#define		timeb	_timeb
#define		ftime	_ftime
#endif







int
readln_from_a_file(FILE *fp, char *line, int size)
{
	char *p;

    do
    {
        p = fgets( line, size, fp );
        if(p)trim(line);
    }while( ( p != NULL ) && ( *line == '#') );

    if( p == NULL )
		return( 0 );

    if (strchr(line, '\n'))
          *strchr(line, '\n') = '\0';
    if (strchr(line, '\r'))
          *strchr(line, '\r') = '\0';
    return( 1 );
}



//
// Convert a UID in asc format to binary (ie this format 00:00:2D:4A:7B:3E:1F:8B)
//
// Should this be non distructive?
//
void
UID_Extract(U8 *uid,U8 *uid_ascii)
{
	int	i;
	char	*subst,*strt_p;
	U8		*in=uid_ascii;

	// remove whitespace
	while(('\n'==*in) || ('\r'==*in) || (' '==*in))
		in++;

	// scan out the bytes.
	subst=strtok_r( (char *)in,": \0\n",&strt_p);

	// Loop for 8 bytes
	if(subst)
	{
		for(i=0;i<8;i++)
		{
			uid[i]=	(U8)strtol((char *) subst,(char **)NULL, 16);
			subst= strtok_r(NULL,": \n\0",&strt_p);
			if(NULL==subst)
				break;
		}
	}
	return;
}


//
// Returns a 32bit seconds count
//
U32 second_count(void)
{
	int	seconds;
	// grab a time value and return a 16 bit 10ms count used for timestamping - 1000=1second
	//#if 0
	//ftime() is obsolete everywhere except maybe embedded Linux?  
	//OS X didn't get ftime() until OS X 10.4.  So we do this...
	// BUT it doesn't work with Mike's code.  It does now....  defined(LINUX) ||
#if  defined(MACOSX) || defined(__ECOS)
	struct timeval   Tp;
	struct timezone  Tzp;
    gettimeofday( &Tp, &Tzp );     /* get timeofday */
	seconds=0;
	seconds=(U32)(Tp.tv_sec);
#endif
#if defined(LINUX) || defined(WIN32)
	struct timeb    timebuffer;
	ftime( &timebuffer );
	seconds=0;
	seconds=(U32)(timebuffer.time);
#endif
#if defined(WINCE)
	SYSTEMTIME tSystemtime;
	GetLocalTime(&tSystemtime);
	seconds=(U32)tSystemtime.wSecond;
#endif
	return(seconds);
}

//
// hund_ms_count(void) - returns a 16 bit 100ms tick count.
//
U16	hund_ms_count(void)
{	
	U16	ticks;
	// grab a time value and return a 16 bit 10ms count used for timestamping - 1000=1second
	//#if 0
	//ftime() is obsolete everywhere except maybe embedded Linux?  
	//OS X didn't get ftime() until OS X 10.4.  So we do this...
	// BUT it doesn't work with Mike's code.  It does now....  defined(LINUX) ||

// Should this all be like the MAC?

#if  defined(MACOSX) || defined(__ECOS) || defined(LINUX)
	struct timeval   Tp;
	struct timezone  Tzp;
    gettimeofday( &Tp, &Tzp );     /* get timeofday */
	ticks=0;
	ticks=(U16)(Tp.tv_sec*10);
	ticks=ticks + ((Tp.tv_usec)/100000);
#endif
#if defined(WIN32) 
	struct timeb    timebuffer; 
	ftime( &timebuffer );
	ticks=0;
	ticks=(U16)(timebuffer.time*10);
	ticks=ticks + ((timebuffer.millitm)/100);
#endif
#if defined(WINCE)
	SYSTEMTIME tSystemtime;
	GetLocalTime(&tSystemtime);
	ticks=0;
	ticks=(U16)(tSystemtime.wSecond*10);
	ticks=ticks+(tSystemtime.wMilliseconds/100);
#endif
	return(ticks);
}



//
// threadswitch() - force a threadswitch
//
void
threadswitch()
{
#if defined(WIN32) || defined(WINCE)
			Sleep(0);
#endif

#if defined(MACOSX)
			usleep(100);
			sched_yield();
#endif

#if defined(LINUX)
			sched_yield();

#endif
#if defined(__ECOS)
			cyg_thread_delay(1);		
#endif
}


//
// Second Sleep Platform Independent
//
void ysleep_seconds(U16 duration)
{
#if defined(WIN32) || defined(WINCE)
			Sleep(duration*1000);  
#endif

#if defined(LINUX) || defined (MACOSX)
			sleep(duration);
#endif
#if defined(__ECOS)
			cyg_thread_delay(duration*1000);		
#endif
}

//
// Second Sleep Platform Independent
//
void ysleep_usec(U32 duration)
{
#if defined(WIN32) || defined(WINCE)
			Sleep(duration/100);  
#endif

#if defined(LINUX) || defined (MACOSX)
			usleep(duration);
#endif
}

//
//
//
long long file_length(char *filename)
{
	long long	size;
	int		ret;

#if defined (WIN32)
	struct	_stat64 st;
	ret=_stat64(filename, &st);			// Fix for windows
#else
	struct	stat st;
	ret=stat(filename, &st);			
#endif

	if(ret>=0)
	{
		size = st.st_size;
	}
	else
	{

#if defined(DEBUG0)
		DEBUG0("(file len) stat fail for filename %s ",filename);
		//perror("stat fail \n");
#endif
		size=0;
	}
	return(size);

}



/*
//
// Doesn't work in windows
//
U32 file_length(char *filename)
{
FILE *fp;
U32 pos;
U32 end;
	
	if(NULL == (fp = fopen( (char *) filename, "r")) )
	{
		return(0);
	}

	pos = ftell (fp);
	fseek (fp, 0, SEEK_END);
	end = ftell (fp);
	fseek (fp, pos, SEEK_SET);
	fclose(fp);

	return end;
}
*/

//
// Random number Generator
//
#if defined(BADRAND)
static unsigned long next = 1;

/* RAND_MAX assumed to be 32767 */
#define MY_RAND_MAX	32767
int myrand(void) 
{
  int t;
  next = next * 1103515245L + 12345L;
  t=(unsigned int)((next >> 16) & 0x7fff);
  DEBUG1("myrand = %d  next %ld\n",t,next);
  return (t); 

//   next = next * 1103515245 + 12345;
//   return((unsigned)(next/65536) % 32768);
}

void mysrand(unsigned seed) 
{
	DEBUG1("seed = %d\n",seed);
   next = seed;
}

#endif


#if defined(BADRAND)

void yrand_seed(long seed)
{
	mysrand(second_count()^seed);
	yrand(10);
}

int yrand(int max)
{
	if(yrandom_init!=1)
		yrand_seed(hund_ms_count()+second_count());
//return(1 + (int) (max * (myrand() / (RAND_MAX + 1.0))));
	return(myrand()%max);
}


#else

void yrand_seed(long seed)
{
	yrandom_init=1;
	srand((hund_ms_count()+second_count())^seed);
	yrand(10);
}

int yrand(int max)
{
	if(yrandom_init!=1)
		yrand_seed(hund_ms_count());
	
	return(1 + (int) (max * (rand() / (RAND_MAX + 1.0))));
}

#endif


//
// isDirectoryNotEmpty(char *dirname)
//
// Returns 1 if directory not empty.
// Returns 0 if directory empty
// Returns -1 if not a directory or doesn't exist
//
int 
isDirectoryNotEmpty(char *dirname) 
{
  int n = 0;

#if defined(LINUX)
  struct dirent *d;

  DIR *dir = opendir(dirname);

  if (dir == NULL) //Not a directory or doesn't exist
    return -1;

  while ((d = readdir(dir)) != NULL) 
  {
	// Likely we should verify that there are files, not just directories
    if(++n > 2)
      break;
  }
  closedir(dir);
  
  if (n > 2) //Directory Empty
  {
	  return 1;
  }
  else
  {
	  return 0;
  }
#endif
#if defined(WIN32)
  return -1;
#endif
}


//
// DeleteDirectroyFiles(char *dirname)
//
// Deletes all files in a directory (in linux only files, no symlinks etc..)
//
// Returns >0 number of files deleted.
// Returns 0 if directory empty
// Returns -1 if not a directory or doesn't exist
//
int DeleteDirectroyFiles(char *dirname)
{
  int count = 0;

  #if defined(LINUX)
  struct dirent *d;
  char		tmp_path[PATH_MAX];


  DIR *dir = opendir(dirname);

  struct stat buf;

  if (dir == NULL) //Not a directory or doesn't exist
  {
		DEBUG0("DIR NULL\n");
		return -1;
  }
  //
  // Loop Through files and unlink them
  //
  while ((d = readdir(dir)) != NULL) 
  {
	  // If not directory delete
	  if(d->d_type==DT_REG)
	  {
		  sprintf(tmp_path,"%s/%s",dirname,d->d_name);
		  DEBUG0("unlink file %s - DT_REG method\n",tmp_path);
		  count++;
		  unlink(tmp_path);
	  } 
	  else if (d->d_type==DT_UNKNOWN )
	  {
		  // Use stat method
		  if(0==lstat(d->d_name, &buf))
		  {
			  if((buf.st_mode & S_IFMT) == S_IFREG )
			  {
				  sprintf(tmp_path,"%s/%s",dirname,d->d_name);
				  DEBUG0("unlink file %s - LSTAT method\n",tmp_path);
				  count++;
				  unlink(tmp_path);
			  }
		  }
	  }

  }
  closedir(dir);
#endif

#if defined(WIN32)
  count=-1; // not implemented yet
  /*
  // Use stat method
	if(0==_stat(dirp->d_name, &buf))
	{
		if(buf.st_mode==S_ISREG)
		{
			DEBUG0("unlink file %d - _stat method\n",d->d_name);
			count++;
			unlink(d->d_name);
		}
	}
	*/
#endif

  // Return the number of files deleted
  return(count);
}


#if defined(WIN32)
struct tm *gmtime_r(time_t *_clock, struct tm *_result)
{
  struct tm *p = gmtime(_clock);

  if (p)
    *(_result) = *p;

  return p;
}

struct tm *localtime_r(time_t *_clock, struct tm *_result)
{
  struct tm *p = localtime(_clock);

  if (p)
    *(_result) = *p;

  return p;
}
#endif // win32




//////////////////////////////
// String Utilities
//////////////////////////////
void
strtolower(char *string)
{
	int i;

	if(string)
	{
		for (i = 0; string[i]; i++)
			string[i] = tolower(string[i]);
	}
}

// Trim the beginning and end of string
void trim(char * s) 
{
    char * p = s;
    int l = strlen(p);

    while((l>0) && isspace(p[l - 1])) p[--l] = 0;

    while(* p && isspace(* p)) ++p, --l;
    memmove(s, p, l + 1);
}
//
// Replace all characters of value orig_char with replace_char in a string, returns the number of chars replaced.
//
int
str_char_replace(char *s, const char orig_char, const char replace_char)
{
	char * p = s;
	int		count=0;

	while(*p)
	{
		if(orig_char==*p)
		{
			*p=replace_char;
			count++;
		}
		p++;
	}
	return(count);
}


// Yoics Strtok_r equiv
char *
strtok_y(char *str, const char *sep, char **lasts)
{
	char *p;
	if (str == NULL) 
	{
		str = *lasts;
	}
	if (str == NULL) 
	{
		return NULL;
	}
	str += strspn(str, sep);
	if ((p = strpbrk(str, sep)) != NULL) 
	{
		*lasts = p + 1;
		*p = '\0';
	} 
	else 
	{
		*lasts = NULL;
	}
	return str;
}

// This is a hybred beteen strcasestr and memmem, compairs 2 memory regions, but is string centric
void
*memcasemem(void *haystack, size_t haystack_len,void *needle, size_t needle_len)
{
	int		j;
	char	*tptr=NULL;
	char	c1,c2;
    char    *hay=(char*)haystack;
    char    *need=(char*)needle;
    size_t  i;


	j=0;
	for(i=0;i<haystack_len;i++)
	{
		c1=hay[i];
		c2=need[j];
		if (toupper(c1) == toupper(c2))
		{
            j++;
			if(j==needle_len)
			{	
				tptr=&hay[(i+1)-needle_len];
				break;
			}
		}
		else
			j=0;
	}

	return(tptr);
}


#if defined(WIN32)
char *strcasestr(const char *haystack, const char *needle)
{
    size_t	nlen = strlen(needle);
	size_t	hlen = strlen(haystack);
    
    return((char*)memcasemem((void*)haystack, hlen, (void*)needle, nlen));
}

#endif

//
// Strip Extra slashes, IE /// = / or // = /, but / = /
//
int
strip_slash(char *buffer)
{
	char *d=buffer;
	char *s=buffer;

	while(*s)
	{
		if('/'==*s)
		{
			*d++=*s++;
			while('/'==*s)
				s++;
			continue;
		}
		*d++=*s++;
	}
	*d=*s;

	return(0);
}

void
strip_crlf(char *d)
{
	if(d==0)
		return;

	// chop off any \r\n at the end of a string
	while(strlen(d) && (('\n'==d[strlen(d)-1]) || ('\r'==d[strlen(d)-1]) ) )
		d[strlen(d)-1]=0;
}



