//--------------------------------------------------------------------------
// printlog.c - Generic Log File Routines developed for the SVN.NET tools.
//
// Version 0.1 alpha by Mike Johnson, (c)1998 svn.net
//
// Modified for yoics 2007,2008 for UDP logging also.
//
//--------------------------------------------------------------------------
#include "config.h"
//#include "yoicsd.h"
#include "log.h"
#include "arch.h"
#include "debug.h"


#if defined(LOGGING)

#define MAX_PRINTF_BUFFER	256
#define MAX_LOG_NAME		64
#define MAX_LOG_ID			32

static char				logname[MAX_LOG_NAME];
static char				log_id[MAX_LOG_ID];
static char				printf_buffer[MAX_PRINTF_BUFFER];  
static int				init=0;
static U32				lmask=0;
static U16				logport=0;
static SOCKET			logsocket=0;

int
setlogname(U32 logmask, const char *filename)
{
        if(strlen(filename)>(MAX_LOG_NAME-1))
                return -1;

		// set logfilename
		strcpy(logname,filename);
		
		// set logmask
		lmask=logmask;

        init=1;

		DEBUG1("Setting logname to %s and mask to %x\n",filename,logmask);
        return 0;
}

//
// Warning, not re-enterant
//
int 
printlog(U32 levelmask, const char *fmt, ...)
{
#if defined(__ECOS)
#else
        FILE *fp;
        va_list args;
        char buffer[1024];      
        
		// do nothing if not initalized.
        if(init==0)
			return -1;

		// only print log if level is set
		if(!(levelmask & lmask))
			return 0;

		// toss out bad formats.
        if(strlen(fmt)>1024)
                return -1; 

        if(NULL == (fp = fopen(logname, "a")) )
                return -1;
               
        va_start(args, fmt);
        vsprintf(buffer, fmt, args);
        va_end(args);
        
        fprintf(fp, "%s", buffer);
        
		fclose(fp);
		fp=0;
#endif
        return 0;
}


//
// Poll the UDP log port for commands, status and quit?
//
int
YOICS_input_log_poll()
{
	if(0==logport)
	{
		// Poll the socket for input commands
	}
	return(0);
}



void
printl_uid(U32 levelmask,U8* uid)
{
	printlog(levelmask,"%x:%x:%x:%x:%x:%x:%x:%x",uid[0],uid[1],uid[2],uid[3],uid[4],uid[5],uid[6],uid[7]);
}

//
// Yoics Printf, we do not protect against buffer overflows, do not
// print strings longer than ~400 bytes
//
void
yprintf(const char *fmt, ...)
{
	int		ret;
	//int		idlen=0;
	char	*buff;
    va_list args;
	struct sockaddr_in	client;		/* Information about the client */

	// calculate idlen
	if(0!=logport)
	{
		if( strncmp(log_id,printf_buffer,strlen(log_id)) )
			yprintf("corrupted log_id\n");

		buff=strchr(printf_buffer,' ');
		buff++;
	}
	else
		buff=printf_buffer;

	va_start(args, fmt);
	vsprintf(buff, fmt, args);
	va_end(args);
	
	// Check if logging is enabled
	if(0==logport)
	{
		if(LOG_PRINTF&lmask)
		{
			printlog(LOG_PRINTF, "%s", printf_buffer);
		}

#if !defined(SUPRESS_PRINTFS)
		printf("%s", printf_buffer);
#endif

	}
	else
	{
        // clear out socket structure
        memset((void *)&client, '\0', sizeof(struct sockaddr_in));
		//
		// Send message to localhost on log_port
		//

		client.sin_family		= AF_INET;
		client.sin_addr.s_addr	= 0x0100007f;
		client.sin_port			= htons(logport);		// Destination port
		//
		//printf("send pkt at %x len %d\n",(int)pkt,len);
		ret=sendto(logsocket	, (char *)printf_buffer, strlen(printf_buffer), 0, (struct sockaddr *)&client, sizeof(struct sockaddr));
		//printf("end\n");
		if(ret<0)
			log_lasterror((U8 *) "yprintf error");
	}

}

void
log_lasterror(U8 *msg)
{
	printlog(LOG_MISC,"error in %s errno=%d\n",msg,get_last_error());
}

#else


// No Logging
void
yprintf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

// printf with timestamp
void
ytprintf(const char *format, ...)
{
    U32     count;
	va_list args;

    //char *timestamp=timestamp_get();
    //trim(timestamp);
    //printf("%s - ",timestamp);
    //free(timestamp);
    count=ms_count();
    printf("[%u] ",count);

	va_start(args, format);
    vprintf(format, args);
    va_end(args);

    fflush(stdout);

}


#endif

//
// Functions that are always here
//
int timerightnow(char *s, size_t size)
{
#ifdef LINUX
        time_t ti;
        struct tm *stm;
                
        time(&ti);
        stm = localtime(&ti);
        strftime(s, size, "%c", stm);
#endif        
        return 0;
}

// Returns a buffer that must be freed
char  *timestamp_get(void)
{
#if defined(WIN32)
    struct tm * timeinfo;
    char        *tptr;
#else
    struct tm result;
#endif

    time_t ltime; /* calendar time */
    char *time_str;

    time_str=(char*)malloc(64);

    if(time_str)
    {
        ltime=time(NULL); /* get current cal time */
#if defined(WIN32)
        timeinfo=localtime(&ltime);
        tptr=asctime(timeinfo);
        strncpy(time_str,tptr,63);
        time_str[63]=0;
#else
        localtime_r(&ltime, &result);
        asctime_r(&result, time_str);
#endif    
    }
    return(time_str);
}


void
print_uid(U8* uid)
{
	yprintf("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",uid[0],uid[1],uid[2],uid[3],uid[4],uid[5],uid[6],uid[7]);
}

//
// Yoics Printf Setup
//
// returns -1 for socket error, 0 for ok
//
// Assume we will only use first 16 bytes of ID
//
int
YOICS_Printf_Setup(U16 dest_port,U8 *id)
{
	int					ret=0;
#if defined(LOGGING)	
	struct sockaddr_in	client;		/* Information about the client */
	
	if(0==dest_port)
	{
		printf_buffer[0]=0;
		return(0);
	}

	logport=dest_port;
	//
	// Setup socket
	/* Open a datagram socket */
	logsocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (logsocket == INVALID_SOCKET)
	{
		DEBUG1("Could not create socket.\n");
		logsocket=0;
		return(-1);
	}

	// Assume ID is good, copy over the first 16 bytes to the buffer
	strncpy(printf_buffer,id,MAX_LOG_ID);
	printf_buffer[MAX_LOG_ID]=0;
	strcat(printf_buffer," ");	// Add a space
	strncpy(log_id,id,MAX_LOG_ID);
	//
	// Save the port
	logport=dest_port;


	// clear out socket structure
	memset((void *)&client, '\0', sizeof(struct sockaddr_in));
    
	client.sin_family = AF_INET;					// host byte order
	client.sin_port =  INADDR_ANY;//htons(logport);	// Grab any port
	/* Set server address */
	client.sin_addr.s_addr= INADDR_ANY;				// Check all IP

	ret=bind(logsocket, (struct sockaddr *)&client, sizeof(struct sockaddr_in));
	
	if(-1==ret)
	{
		closesocket(logsocket);
		// clear the port
		logport=0;
	}
	else 
		ret=0;
#endif
	return(ret);
}

int
YOICS_Printf_Shutdown()
{
#if defined(LOGGING)	
	if(logsocket)
	{
		closesocket(logsocket);
		logsocket=0;
	}
#endif
	return(0);
}


/*! \fn int yoics_write_info(U8 *filename, YOICS_INFO *info)

    \brief Write info to a file in javascript format, or other formate depending on type					
		
		  type=0, javascript var=

	\return 
*/
int
yoics_write_info(U8 *filename, int type, YOICS_STATUS_INFO *info)
{
	FILE *fp;
	
	if(!filename)
		return(-1);
	if(0==strlen((char*)filename))
		return(-1);

	DEBUG2(" write info at %s\n",filename);

	if(NULL == (fp = fopen((char*)filename, "w")) )				// fopen_s for windows?
		return -1;

	if(0==type)
	{
#if defined(NUMERIC_STATUS_INFO)

	fprintf(fp,"%d%d%d\n",info->state,info->initialized,info->server_status_num);

#else
#if defined(PINETRON) || defined(EDGE_DVR) || defined(DYNA_NVR) || defined(ACCESS)
//
		if(5==info->state)
		{
			if(info->initialized)
				fprintf(fp,"Registered and Connected to Yoics. ");
			else
				fprintf(fp,"Not Registered and Connected to Yoics. ");
		}
		else
		{
			fprintf(fp,"Not Connected to Yoics (%d). ",info->state);
		}

		if(info->server_status)	
		{
			fprintf(fp," %s",info->server_status);
		}
		fprintf(fp,"\n");


#else
		if(info->server_status)	
			fprintf(fp,"yoics_server_status = \"%s\";\n",info->server_status);
		else
			fprintf(fp,"yoics_server_status =\"\";\n");

		if(info->peer_status)	
			fprintf(fp,"yoics_peer_status = \"%s\";\n",info->peer_status);
		else
			fprintf(fp,"yoics_peer_status =\"\";\n");

		fprintf(fp,"yoicsd_state = %d;\n",info->state);
		fprintf(fp,"yoicsd_initialized = %d;\n",info->initialized);

		if(5==info->state)
		{
			if(info->initialized)
				fprintf(fp,"yoics_current_state=\"Registered and Connected to Yoics.\"");
			else
				fprintf(fp,"yoics_current_state=\"Not Registered but Connected to Yoics.\"");
		}
		else
		{
			fprintf(fp,"yoics_current_state=\"Not Connected to Yoics (code %d).\"",info->state);
		}
#endif	
#endif
	}
	

	fclose(fp);

	return(0);
}


