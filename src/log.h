#ifndef __log_h_
#define __log_h_

// Set #define LOGGING 1   to use logging

#include "mytypes.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

//
// Log levels
//
#define		LOG_MEMORY_ALLOC		0x0001
#define		LOG_SERVER_EVENTS		0x0002
#define		LOG_SERVER_PINGS		0x0004
#define		LOG_SESSION_EVENTS		0x0008
#define		LOG_TUNNEL_CREATE		0x0010
#define		LOG_TUNNEL_DESTROY		0x0020
#define		LOG_TUNNEL_PING			0x0040
#define		LOG_TUNNEL_SENDS		0x0080
#define		LOG_TUNNEL_RX			0x0100
#define		LOG_TIME_OUTS			0x0200
#define		LOG_PROXY_EVENTS		0x1000
#define		LOG_PROXY_LOCAL_RX		0x2000
#define		LOG_PROXY_LOCAL_TX		0x4000
#define		LOG_MISC				0x8000
#define		LOG_PRINTF				0x10000

// Yoics status file info
typedef struct YOICS_STATUS_INFO_
{
	int 				initialized;				// 0 default, 1=secret present
	int					state;						// Server Connection State
	int					updated;
	int					server_status_num;			// server status number
	char				server_status[128];			// Current status
	char				peer_status[128];
}YOICS_STATUS_INFO;


char  *timestamp_get(void);
void	yprintf(const char *fmt, ...);
void	ytprintf(const char *fmt, ...);

#if defined(LOGGING)

int setlogname(U32 logmask, const char *filename);

int printlog(U32, const char *fmt, ...);

void	printl_uid(U32 levelmask, U8* uid);
int timerightnow(char *s, size_t size);


void	log_lasterror(U8 *msg);

// UDP logging

void	yprintf(const char *fmt, ...);


#else

// No logging enabled
#ifdef WIN32
    #define		printlog
#else
	#define printlog(...)		//
#endif

#ifdef WIN32
    #define		printl_uid
#else
	#define printl_uid(...)		//
#endif


#ifdef WIN32
    #define		setlogname
#else
	#define setlogname(...)		//
#endif

#ifdef WIN32
    #define		log_lasterror
#else
	#define log_lasterror(...)		//
#endif


#endif

//
// Functions that always live
//

int		YOICS_Printf_Setup(U16 dest_port,U8 *id);
int		YOICS_Printf_Shutdown();

void	print_uid(U8* uid);

// Yoics status file
//int yoics_write_status(U8 *filename, int type, YOICS_STATUS_INFO *info);


/*! \fn int yoics_write_info(U8 *filename, YOICS_INFO *info)

    \brief Write info to a file in javascript format, or other formate depending on type					
		
		  type=0, javascript var=

	\return 
*/
int yoics_write_info(U8 *filename, int type, YOICS_STATUS_INFO *info);



#endif




