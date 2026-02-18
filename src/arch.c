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



//
// Convert a hex ascii string to a binary buffer, dest must be big enough
//
// return converted len
//
int
hexascii_2_bin(U8 *dest, int maxlen, char *in)
{
	int	count=0;
    int i=0;
	U8		*hexascii=(U8*)in;
    char    ch;
    char    str[3];

    if(NULL==dest)
        return(-1);

    i=0;
    while( (ch=(*hexascii++)) )
    {
        if(isxdigit(ch))
        {
            str[i++]=ch;
            if(2==i)
            {
                //flush
                str[i]=0;
                dest[count++]=	(U8)strtol((char *) str,(char **)NULL, 16);
                i=0;
            }
        }
        else
        {
            // Not hexascii anymore, flush if i!=0
            if(i)
            {
                str[i]=0;
                dest[count++]=	(U8)strtol((char *) str,(char **)NULL, 16);
                i=0;
            }
        }
        if(count>=maxlen)
            break;
    }
    // check for final flush
    if(i)
    {
        str[i]=0;
        dest[count++]=	(U8)strtol((char *) str,(char **)NULL, 16);
        i=0;
    }
    //        
    return(count);
}


//
// Make non destructive
//
void
IP_Extract(IPADDR *ip,char *ip_ascii)
{		
#if defined(WIN32) || defined(WINCE)	
	U8		*subst;
	char	*strt_p;
    char    tstr[64];
    //char    *in=tstr;

    assert(ip != NULL);
    assert(ip_ascii != NULL);

	ip->ip32=0;
    strncpy(tstr,(char*)ip_ascii,63);
    tstr[63]=0;

	// scan out the bytes.
	subst=(U8 *) strtok_r((char *) tstr,". \n",&strt_p);

	while(1)
	{
		// get 4 bytes
		if(!subst) break;
		ip->ipb1 =	(U8)atoi((char *) subst);
		subst= (U8 *) strtok_r(NULL,". \n",&strt_p);
		if(!subst) break;
		ip->ipb2 =	(U8)atoi((char *) subst);
		subst= (U8 *) strtok_r(NULL,". \n",&strt_p);
		if(!subst) break;
		ip->ipb3 =	(U8)atoi((char *) subst);
		subst= (U8 *) strtok_r(NULL,". \n",&strt_p);
		if(!subst) break;
		ip->ipb4 =	(U8)atoi((char *) subst);
		break;
	}
#else
	if(0==	inet_aton((const char *) ip_ascii, (struct in_addr *) ip )) 
		ip->ip32=0;
#endif
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

U32 ms_count(void)
{
    U32 ticks;

// Should this all be like the MAC?

#if  defined(MACOSX) || defined(__ECOS) || defined(LINUX)
    struct timeval   Tp;
    struct timezone  Tzp;
    gettimeofday( &Tp, &Tzp );     /* get timeofday */
    ticks=0;
    ticks=(Tp.tv_sec*1000);
    //printf("ticks seconds = %d\n",ticks);
    ticks=ticks + (Tp.tv_usec/1000);
    //printf("ticks ms = %d\n",(Tp.tv_usec)/1000);
    //printf("raw        %d\n",Tp.tv_usec);
#endif
#if defined(WIN32) 
    struct timeb    timebuffer;
    ftime( &timebuffer );
    ticks=0;
    ticks=(timebuffer.time*1000);
    ticks=ticks + ((timebuffer.millitm));
#endif
#if defined(WINCE)
    SYSTEMTIME tSystemtime;
    GetLocalTime(&tSystemtime);
    ticks=0;
    ticks=(tSystemtime.wSecond*1000);
    ticks=ticks+(tSystemtime.wMilliseconds);
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
			Sleep(duration/1000);
#endif

#if defined(LINUX) || defined (MACOSX)
			usleep(duration);
#endif
}



//
//
//
long file_length(char *filename)
{
#if defined (WIN32)
	struct	stat st;
#else
	struct	stat st;
#endif
	long	size;
	int		ret;
//    int        i=0;

	ret=stat(filename, &st);			// Fix for windows

	if(ret>=0)
	{
		size = st.st_size;
	}
	else
	{

#if defined(DEBUG0)
		DEBUG0("stat fail for filename %s ",filename);
		//perror("stat fail \n");
#endif
		size=0;
	}
	return(size);

}

//
// Strip Extra slashes, IE /// = / or // = /, but / = /
//
void
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

	return;
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


// Trim the beginning and end of string
void trim(char * s)
{
    char * p = s;
    int l = strlen(p);

    if (l == 0) return;

    while(l > 0 && isspace(p[l - 1])) p[--l] = 0;
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
    int             count=0;

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

int my_strlen_utf8_c(char *s) 
{
   int i = 0, j = 0;
   while (s[i]) {
     if ((s[i] & 0xc0) != 0x80) j++;
     i++;
   }
   return j;
}




/*
unsigned char bin2hex(unsigned char c)
{
  if (c >= 10) return 'a' + c - 10;
  else return '0' + c;
}

unsigned char hex2bin(unsigned char c)
{
  if (c >= 'a') return c - 'a' + 10;
  if (c >= 'A') return c - 'A' + 10;
  return c - '0';
}
*/
U8
bin2hex(U8 bin)
{
	U8	ret='0';

	if(bin<0x0a)
		ret=bin+'0';
	else if(bin<0x10)
		ret=bin+('A'-10);

	return(ret);
}

U8
hex2bin(U8 c)
{
return  
		(('0'<=(c))&&('9'>=(c)))				\
                 ?((c)-'0')                     \
                 :( (('a'<=(c))&&('f'>=(c)))    \
                    ?((c)-'a'+10)               \
                    :( (('A'<=(c))&&('F'>=(c))) \
                       ?((c)-'A'+10)            \
                       :(0xFF) ) ) ;
					   
}
/**
 * Converts a binary buffer into a hexadecimal string.
 *
 * @param buf binary buffer of len bytes size
 * @param str string of at least 2*len+1 bytes size
 * @param len length of the buffer
 * @returns -
 */
void bin2hexstr(char *buf, char *str, int len)
{
  int i;

  // check nulls
  assert( (NULL!=buf) && (NULL!=str));
  // zero it in case len is zero
  str[0]=0;

  /* convert the byte array to a hex string */
  for (i = 0; i < len; i++) 
  {
    str[i*2]	= bin2hex(  (U8)((buf[i] >> 4) & 0x0F) );
    str[i*2+1]	= bin2hex( (U8) (buf[i] & 0x0F) );\
  }
  str[2*i] = 0;			// str[i<<1] = 0;
}

/**
 * Converts a binary buffer into a hexadecimal string.
 *
 * @param buf binary buffer of len bytes size
 * @param str string of at least 3*len+1 bytes size
 * @param len length of the buffer
 * @returns -
 */
void bin2hexstrcol(char *buf, char *str, int len)
{
  int i;

  // check nulls
  assert( (NULL!=buf) && (NULL!=str));
  // zero it in case len is zero
  str[0]=0;

  /* convert the byte array to a hex string */
  for (i = 0; i < len; i++) 
  {
    str[i*3]	= bin2hex(  (U8)((buf[i] >> 4) & 0x0F) );
    str[i*3+1]	= bin2hex( (U8) (buf[i] & 0x0F) );
    if((i+1)<len) str[i*3+2]  = ':';
  }
  if(i>0) str[(3*i)-1] = 0;			// str[i<<1] = 0;
}

/**
 * Converts a binary buffer into a hexadecimal string.
 *
 * @param buf binary buffer of len bytes size
 * @param str string of at least 3*len+1 bytes size
 * @param len length of the buffer
 * @returns -
 */
void bin2hexstrlcol(char *buf, int len, char *str, int strlen)
{
  int i,j;

  // check nulls
  assert( (NULL!=buf) && (NULL!=str));

  // zero it in case strlen is zero
  str[0]=0;

  /* convert the byte array to a hex string */
  for (i = 0, j=0; i < len; i++,j=i*3) 
  {
    if(j>strlen) { str[j-1]=0; break; }
    str[j]	= bin2hex(  (U8)((buf[i] >> 4) & 0x0F) );
    if(j+1>strlen) { str[j]=0; break; }
    str[j+1]	= bin2hex( (U8) (buf[i] & 0x0F) );
    if(j+2>strlen) { str[j+1]=0; break; }

    if((i+1)<len) 
        str[i*3+2]  = ':';
    else
        str[i*3+2]  = 0;
  }
}


/**
 * Converts a hexadecimal string into a binary buffer.
 *
 * @param str string of at least 2*len+1 bytes size
 * @param buf binary buffer of len bytes size
 * @param len length of the buffer
 * @returns -
 */
void hexstr2bin(char *str, char *buf, int len)
{
  int i;//,j;

  // check nulls
  assert( (NULL!=buf) && (NULL!=str) );

  /* convert the hex string to a byte array */
  memset(buf, 0, len);
  for (i = strlen(str)-1; i >= 0; i -= 2) 
  {
    if (i > 0) 
		buf[i/2] = hex2bin(str[i-1])*16 + hex2bin(str[i]);
    else 
		buf[i/2] = hex2bin(str[i]);
  }
}

/**
 * Converts a hexadecimal string into a binary buffer.
 *
 * @param str string of at least 2*len+1 bytes size
 * @param buf binary buffer of len bytes size
 * @param len length of the buffer
 * @returns -
 */
void hexstr2bin2(char *str, char *buf, int len)
{
  int i;//,j;

  // check nulls
  assert( (NULL!=buf) && (NULL!=str) );

  /* convert the hex string to a byte array */
  memset(buf, 0, len);

  for (i = strlen(str)-1; i >= 0; i -= 2) 
  {
    if (i > 0) 
		buf[i/2] = hex2bin(str[i-1])*16 + hex2bin(str[i]);
    else 
		buf[i/2] = hex2bin(str[i]);
  }
}


//
// appends a formatted string to a buffer at i(ndex), returns bufSize when filled or overflowed
//
// if retunrs bufSize, may not be zero terminated.
//
size_t 
snprintfcat(char* buf,size_t bufSize, char const* fmt,...)
{
    va_list args;
    size_t result=0;
    size_t len=0;

    // we should assert on null buf
    assert(NULL!=buf);

    // calculate current buffer size
    len= strlen(buf);

    // we should assert on null buf
    assert(NULL!=buf);

    // malloc what we can fill +++ ,ole fix

    if(len<bufSize-1)
    {
        va_start( args, fmt);
        // append it up to max len
        result = vsnprintf( buf + len, bufSize - len, fmt, args);
        va_end( args);
    }

    return result + len;
}

#if defined(LINUX) || defined(MACOSX)
int kbhit(void)
{
	struct timeval tv;
	fd_set read_fd;

	tv.tv_sec=0;
	tv.tv_usec=20000;

	FD_ZERO(&read_fd);
	FD_SET(0,&read_fd);

	if(select(1, &read_fd, NULL, NULL, &tv) == -1)
		return 0;

	if(FD_ISSET(0,&read_fd))
		return 1;

	return 0;
}
#endif

int
readln_from_a_file(FILE *fp, char *line, int size)
{
	char *p;

    do
      p = fgets( line, size, fp );
    while( ( p != NULL ) && ( *line == '#') );

    if( p == NULL )
		return( 0 );

    if (strchr(line, '\n'))
          *strchr(line, '\n') = '\0';
    if (strchr(line, '\r'))
          *strchr(line, '\r') = '\0';
    return( 1 );
}

// Returns buffer to next line, line copied in line returns pointer to next line or NULL if no more in buffer
char *
readln_from_a_buffer(char* buffer, char *line, int size)
{
	char *p=buffer;
    char *l=line;
    int i;
    
    *line=0;
    if( NULL == p )
        return (0);

    // Skip Comments
    while( ( *p != 0 ) && ( *p == '#') )
    {
        // flush this line
        while( ( '\r' != *p )  && ( '\n' != *p ) && ( 0 != *p ) )
            p++;
        while(('\r'==*p)|| ( '\n'==*p ))
            p++;
    }
    //
    if( *p == 0 )
		return( NULL );
    // Copy Line
    i=0;
    while( ( '\r' != *p )&&( '\n' != *p )&&( 0 != *p ) )
    {
        // exit if we've reached size (IE size of 1 we would never store, because we have to do terminator)
        i++;
        if(i>=size)
            break;
        *l++=*p++;
    }
    // terminate line
    *l=0;
    // flush rest of line this line if we still have non end/ret
    if(( '\r' != *p )&&( '\n' != *p )&&( 0 != *p ) )
    {
        while( ( '\r' != *p )&&( '\n' != *p )&&( 0 != *p ) )
            p++;
    }
    // flush returns
    while(('\r'==*p) || ( '\n'==*p ))
        p++;

    // Return new pointer to next line
    if(*p)
        return(p);
    else
        return(NULL);
}
