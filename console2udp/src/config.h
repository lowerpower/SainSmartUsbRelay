#ifndef __CONFIG_H__
#define __CONFIG_H__
/*! \file config.h
    \brief Configuation file 
*/

#include "mytypes.h"

#if !defined(WINCE)
#include <sys/types.h>
#endif

#include <stdio.h>

#define	VERSION		"0.1"


#define _CRT_SECURE_NO_WARNINGS 1


// Don't include malloc for the MAC
#if defined(LINUX) || defined(WIN32) 
//#include <malloc.h>
#endif


#if defined(ARM) && !defined(WINCE)
// Align on 4 byte boundry for ARM
#define ALIGN4			__attribute__ ((aligned (4)));	
#else
#define ALIGN4			;
#endif


//// pack all structures to byte level, this should work for all compilers
//#pragma pack(1)

#ifndef TRACE_LOG
#define TRACEIN   
#define	TRACEOUT
#endif


// For windows include the following
#if defined(WIN32)
#ifndef _MT 
#define _MT 
#endif 
//#ifndef  WIN32_LEAN_AND_MEAN 
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <process.h>    /* _beginthread, _endthread */

//#include <winsock2.h>
#include <stdlib.h>
#include <conio.h>
#include <io.h>
#include <time.h>
#include <signal.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <Rpc.h>
#include <string.h>
#include <assert.h>
#include <process.h>


#if defined(WIN_UPNP_SUPPORT)
#include <Natupnp.h>
#endif

#define snprintf	_snprintf
#define strtok_r	strtok_s

#define _CRTDBG_MAP_ALLOC

#include <crtdbg.h>

// Winsock fix
#define MSG_WAITALL 0x8 

//#define FD_SETSIZE 512

// Unix some winsock errors
#define EWOULDBLOCK		WSAEWOULDBLOCK
#define EINPROGRESS		WSAEINPROGRESS
#define EALREADY		WSAEALREADY
#define EINVAL			WSAEINVAL
#define EISCONN			WSAEISCONN
#define ENOTCONN		WSAENOTCONN
#define EMSGSIZE        WSAEMSGSIZE

//WSAEFAULT

#define usleep			Sleep

#if defined(ZAVIO)
#define openlog(...) //	
#define syslog(...) //
#endif

//#endif
#endif

#if defined(LINUX) || defined(MACOSX)

// For Linux include the following
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <net/if.h>
#include <signal.h>
#include <errno.h>
#include <sys/timeb.h>
#include <sched.h>
#include <syslog.h>
#include <ctype.h>
//#include <linux/limits.h>
#include <dirent.h>

#define MAX_PATH PATH_MAX


#endif

#if defined(WINCE)
#define _tfunc AnsiFunc

#include <windows.h>
#include <winbase.h>
//#include <winsock.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Rpc.h>
#include <string.h>
#include <assert.h>

#define socklen_t int
#define strerror(n) _T("file error")

// Winsock fix
#define MSG_WAITALL 0x8 

// Unix some winsock errors
#define EWOULDBLOCK		WSAEWOULDBLOCK
#define EINPROGRESS		WSAEINPROGRESS
#define EALREADY		WSAEALREADY
#define EINVAL			WSAEINVAL
#define EISCONN			WSAEISCONN
#define ENOTCONN		WSAENOTCONN

#define usleep			Sleep
#define snprintf _snprintf

#define	close			closesocket

#undef WIN32

#endif

#if defined(__ECOS)

#include <network.h>
#include <pkgconf/net.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <prod_config.h>
#include <cyg/libc/time/time.h>
#include <time.h>

#endif



#if defined(LINUX) || defined(MACOSX) || defined(__ECOS)

#define SOCKET_ERROR		-1
#define INVALID_SOCKET		-1
#define closesocket			close

#define TRUE				1 
#define FALSE				0

#endif




#endif


