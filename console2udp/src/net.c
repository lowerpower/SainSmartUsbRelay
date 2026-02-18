/*!																www.yoics.com			
 *---------------------------------------------------------------------------
 *! \file net.c
 *  \brief Yoics networks and web tools
 *																			
 *---------------------------------------------------------------------------
 * Version                                                                  -
 *		0.1 Original Version April 6, 2009									-        
 *
 *---------------------------------------------------------------------------    
 *                                                             				-
 * Copyright (C) 2006, Yoics Inc, www.yoics.com								-
 *                                                                         	-
 * $Date: mwj 2009/04/06 20:35:55 $
 *
 *---------------------------------------------------------------------------
 *
 * Notes:
 *
 *
 *
*/


#include "mytypes.h"
#include "config.h"
#include "arch.h"
#include "net.h"
#include "webio.h"
#include "debug.h"
//#include "log.h"

// Backup Resolver can be useful for embedded/misconfigured devices/and china
#if defined(RESOLVE)
#include "resolver.h"
#endif

#if defined(WIN32)
#define GETHOSTBYNAME 1
#endif


//
// network_init()-
// If anything needs to be initialized before using the network, put it here, mostly this
//	is for windows.
//
int network_init(void)
{
#if defined(WIN32) || defined(WINCE)

	WSADATA w;								/* Used to open Windows connection */
	/* Open windows connection */
	if (WSAStartup(0x0101, &w) != 0)
	{
		fprintf(stderr, "Could not open Windows connection.\n");
	    printf("**** Could not initialize Winsock.\n");
		exit(0);
	}	

#endif
return(0);
}

int
set_sock_nonblock(SOCKET lsock)
{
	int ret;
#if defined(WIN32) || defined(WINCE)
	u_long	flags;

	flags=0x1;
	ret=ioctlsocket ( lsock, FIONBIO, (u_long FAR *) &flags);
#endif

#if defined (__ECOS)
    int tr = 1;
    ret=ioctl(lsock, FIONBIO, &tr); 
#endif

#if defined(LINUX) || defined(MACOSX) || defined(IOS)

	int flags;

	flags = fcntl(lsock, F_GETFL, 0);
	ret=fcntl(lsock, F_SETFL, O_NONBLOCK | flags);

#endif

	return(ret);
}


int
set_sock_block(SOCKET lsock)
{
	int ret;
#if defined(WIN32) || defined(WINCE)
	u_long	flags;

	flags=0x0;
	ret=ioctlsocket ( lsock, FIONBIO, (u_long FAR *) &flags);
#endif

#if defined (__ECOS)
    int tr = 0;
    ret=ioctl(lsock, FIONBIO, &tr); 
#endif

#if defined(LINUX) || defined(MACOSX) || defined(IOS)

	int flags;

	flags = fcntl(lsock, F_GETFL, 0);
	ret=fcntl(lsock, F_SETFL, ~O_NONBLOCK & flags);

#endif

	return(ret);
}


int
set_sock_send_timeout(SOCKET lsock, int secs)
{
	int ret=-1;

#if defined(WINDOWS)
	int timeout=secs*1000;
	ret = setsockopt(lsock,SOL_SOCKET ,SO_SNDTIMEO,(char *)&timeout,sizeof(timeout));
#else
	struct timeval tv;
	tv.tv_sec = secs;
	tv.tv_usec = 0;
	if ( (ret=setsockopt(lsock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)) ) < 0)
	{
		
	}
#endif
	return(ret);
}


//setsockopt( sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv);
int
set_sock_recv_timeout(SOCKET lsock, int secs)
{
	int ret=-1;

#if defined(WINDOWS)
	int timeout=secs*1000;
	ret = setsockopt(lsock,SOL_SOCKET ,SO_RCVTIMEO,(char *)&timeout,sizeof(timeout));
#else
	struct timeval tv;
	tv.tv_sec = secs;
	tv.tv_usec = 0;
	if ( (ret=setsockopt(lsock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv)) ) < 0)
	{
		
	}
#endif
    DEBUG1("set recv timeout ret %d\n",ret);
	return(ret);
}


int get_last_error()
{
#if defined(WIN32) || defined(WINCE)
	return(WSAGetLastError());
#endif

#if defined(LINUX) || defined(MACOSX) || defined(__ECOS) || defined(IOS)
	return(errno);
#endif
}

/*! \fn IPADDR lookup_ipv4(char *name)
    \brief Returns the IPV4 address of a name or 0 for not found, uses only the machine resolver.

    \param name Hostname string.
	\return 0 if failed, or ip address
*/
IPADDR 
lookup_ipv4(char *name)
{
IPADDR answer;
#if defined(GETHOSTBYNAME)
struct hostent		*host_info;
#else
struct addrinfo hints;
struct addrinfo *res,*p;
#endif

    // Default answer is fail
    answer.ip32=0;

#if defined(GETHOSTBYNAME)
    // Use get host by name not
	host_info=gethostbyname(name);

	if(host_info!=0)
	{
		DEBUG2("OK\n");
		fflush(stdout);	
		answer.ip32=*((unsigned long*)host_info->h_addr);
	}
	else
	{
#if defined(DEBUG_LV3)
		DEBUG2("gethostbyname fail code %d\n",h_errno);
		fflush(stdout);
#endif
    }
#else
    // Use getaddrinfo  may be broken in 64 bit +++
    //
    memset(&hints, 0, sizeof hints);    // make sure the struct is empty
    hints.ai_family = AF_INET;          // IPV4 only
    hints.ai_socktype = 0;    
    hints.ai_flags = 0;       

    if (getaddrinfo(name,0, &hints, &res) == 0)
    {
        for(p = res;p != NULL; p = p->ai_next) {
            //void *addr;
            char *ipver;
            char ipstr[255];

            // get the pointer to the address itself,
            // different fields in IPv4 and IPv6:
            if (p->ai_family == AF_INET) { // IPv4
                struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
                answer.ip32 = ipv4->sin_addr.s_addr;
                ipver = "IPv4";
            } 
            // convert the IP to a string and print it:
            inet_ntop(p->ai_family, &answer, ipstr, sizeof ipstr);
            DEBUG2("  %s: %s\n", ipver, ipstr);
        }
        freeaddrinfo(res); // free the linked-list
    }

#endif

    return(answer);
}


/*! \fn struct hostent*	resolve(char * name)
    \brief Returns the DNS address information of the passwd host name.

    \param name Hostname string.
	\return 0 if failed, or a pointer to the hostent structure with DNS information
*/
IPADDR 
resolve(char *name)
{
IPADDR answer;


    TRACEIN;
	if(inet_addr(name)==INADDR_NONE)
	{
#if defined(__ECOS)
		
		answer.ip32=dns_gethostbyname(name);
		if(answer.ip32)
		{
			answer.ip32=htonl(answer.ip32);
		}
		else
			answer.ip32=0;
#else

//#if !defined(SUPPRESS_PRINTFS)
DEBUG0("host to resolve = %s\n",name);
//#endif

		fflush(stdout);	
		//res_init();

        answer=lookup_ipv4(name);

#if defined(RESOLVE)
        if(answer.ip32==0)
        {
			//
			// We have failed, lets try to resolve with our built in resolver   +++ This needs to be well tested
			if(0<resolve_name(&answer, name))
			{
				// Log the fail
				DEBUG0("Cannot resolve name with built in resolver\n");
#if defined(DEBUG_LV3)
				fflush(stdout);
#endif
				answer.ip32=0;
			}
		}
#endif


#endif
	}
	else
	{
		answer.ip32=inet_addr(name);
//		answer=gethostbyaddr((char*)&addr,sizeof(addr),AF_INET);
	}
    TRACEOUT;
	return(answer);
}



/*
	NetConnect() Connect a TCP socket with timeout

	pcServer		:	String of name or IP address of host to connect to
	usPort			:	Port to connect to
	iMillSecTimeout	:	Timeout in MS to try connect
	iOut_fd			:	a socket handle if creation was good

	Not that the socket is left in non-blocking mode when returned

	communication with IPCam by HTTP
	Return: -1, network error
			0, success
*/
int NetConnect1(const char *pcServer, unsigned short usPort, int iMillSecTimeout, SOCKET *iOut_fd)
{
	int fd;
	//int iFl;
	struct timeval tv;
	fd_set fdsRead;
	fd_set fdsWrite;
    IPADDR  ip;
	//struct hostent* sHostent = NULL;
	struct sockaddr_in sin;
	//char **ppc;

    TRACEIN;
	if (iOut_fd == NULL || pcServer == NULL || iMillSecTimeout <= 0)
	{
		DEBUG3("YOICS_CONFIG:NetConnect1:Illegal parameters while call NetConnect!\n");
        TRACEOUT;
		return -1;
	}
	*iOut_fd = -1;

    ip=resolve((char*)pcServer);

    if(ip.ip32==0)
	{
		DEBUG3("YOICS_CONFIG:NetConnect1:Failed to resolve host %s!\n",pcServer);
        TRACEOUT;
		return -1;
	}

	tv.tv_sec = iMillSecTimeout / 1000;
	tv.tv_usec = (iMillSecTimeout - 1000 * tv.tv_sec) * 1000;

//	for (ppc=sHostent->h_addr_list; *ppc; ppc++)
//	{

		/* create socket */
		fd = socket(PF_INET,SOCK_STREAM,0);
		if (fd == INVALID_SOCKET)
		{
			DEBUG3("YOICS_CONFIG:NetConnect:Can not create socket!\n");
            TRACEOUT;
			return -1;
		}

		set_sock_nonblock(fd);
		FD_ZERO(&fdsRead);
		FD_SET(fd, &fdsRead);
		FD_ZERO(&fdsWrite);
		FD_SET(fd, &fdsWrite);

		/* Connect to server */
		memset((void *)&sin, '\0', sizeof(struct sockaddr_in));
		sin.sin_family = AF_INET;
		sin.sin_port = htons(usPort);	//smtp port number
		sin.sin_addr.s_addr = ip.ip32;

		DEBUG2("resolved %s to %s\n",pcServer,inet_ntoa(sin.sin_addr));

		connect(fd,(struct sockaddr *)&sin,sizeof(sin));

		if (select(fd+1, &fdsRead, &fdsWrite, NULL, &tv) > 0)
		{
			if (FD_ISSET(fd, &fdsWrite) || FD_ISSET(fd, &fdsRead))
			{
				int iErrorCode;
				int iErrorCodeLen = sizeof(iErrorCode);
				if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&iErrorCode, (socklen_t *)&iErrorCodeLen) == 0)
				{
					if (iErrorCode == 0)
					{
						*iOut_fd = fd;
						// Turn nonblock off
						//set_sock_nonblock(fd);
						//fcntl(fd, F_SETFL, iFl);
                        TRACEOUT;
						return 0;
					}
				}
			}
		}

		DEBUG3("NetConnect1:Can not connect %s:%d in %d millsecond.\n", pcServer,usPort, iMillSecTimeout);
		closesocket(fd);
		DEBUG3("closefd\n");
	//}
    TRACEOUT;
	return -1;
}

#if 0
int NetConnect1(const char *pcServer, unsigned short usPort, int iMillSecTimeout, SOCKET *iOut_fd)
{
	int fd;
	//int iFl;
	struct timeval tv;
	fd_set fdsRead;
	fd_set fdsWrite;
	struct hostent* sHostent = NULL;
	struct sockaddr_in sin;
	char **ppc;

    TRACEIN;
	if (iOut_fd == NULL || pcServer == NULL || iMillSecTimeout <= 0)
	{
		DEBUG3("YOICS_CONFIG:NetConnect1:Illegal parameters while call NetConnect!\n");
        TRACEOUT;
		return -1;
	}
	*iOut_fd = -1;

    resolve(pcServer);
	if ((sHostent = gethostbyname(pcServer)) == NULL)
	{
		DEBUG3("YOICS_CONFIG:NetConnect1:Failed to resolve host %s!\n",pcServer);
        TRACEOUT;
		return -1;
	}

	tv.tv_sec = iMillSecTimeout / 1000;
	tv.tv_usec = (iMillSecTimeout - 1000 * tv.tv_sec) * 1000;

	for (ppc=sHostent->h_addr_list; *ppc; ppc++)
	{

		/* create socket */
		fd = socket(PF_INET,SOCK_STREAM,0);
		if (fd == INVALID_SOCKET)
		{
			DEBUG3("YOICS_CONFIG:NetConnect:Can not create socket!\n");
            TRACEOUT;
			return -1;
		}

		set_sock_nonblock(fd);
		/* set socket to O_NDELAY */
/*		iFl = fcntl(fd, F_GETFL, 0);
		if (fcntl(fd, F_SETFL, iFl | O_NDELAY) != 0)
		{
			fprintf(stderr, "YOICS_CONFIG:NetConnect:Can not set socket fd to O_NDELAY mode.\n");
			closesocket(fd);
			return -1;
		}
*/
		FD_ZERO(&fdsRead);
		FD_SET(fd, &fdsRead);
		FD_ZERO(&fdsWrite);
		FD_SET(fd, &fdsWrite);

		/* Connect to server */
		memset((void *)&sin, '\0', sizeof(struct sockaddr_in));
		sin.sin_family = AF_INET;
		sin.sin_port = htons(usPort);	//smtp port number
		sin.sin_addr.s_addr = *(unsigned long*)*ppc;

		DEBUG3("resolved %s to %s\n",pcServer,inet_ntoa(sin.sin_addr));

		connect(fd,(struct sockaddr *)&sin,sizeof(sin));

		if (select(fd+1, &fdsRead, &fdsWrite, NULL, &tv) > 0)
		{
			if (FD_ISSET(fd, &fdsWrite) || FD_ISSET(fd, &fdsRead))
			{
				int iErrorCode;
				int iErrorCodeLen = sizeof(iErrorCode);
				if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&iErrorCode, (socklen_t *)&iErrorCodeLen) == 0)
				{
					if (iErrorCode == 0)
					{
						*iOut_fd = fd;
						// Turn nonblock off
						//set_sock_nonblock(fd);
						//fcntl(fd, F_SETFL, iFl);
                        TRACEOUT;
						return 0;
					}
				}
			}
		}

		DEBUG3("NetConnect1:Can not connect %s:%d in %d millsecond.\n", pcServer,usPort, iMillSecTimeout);
		closesocket(fd);
		DEBUG3("closefd\n");
	}
    TRACEOUT;
	return -1;
}
#endif

/*! \fn S16 test_udp_bind_sock(port)  - test if we can bind this socket
    \brief 

    \param 
	\return -1 no bind,  else port number of bound port
*/
int
test_udp_bind(U16 port)
{
	SOCKET				new_soc;
	int					ret,len;
	struct sockaddr_in	client;		/* Information about the client */
    //int                 s,ttl;
    
	// bind a UDP port
	/* Open a datagram socket */
	new_soc = socket(AF_INET, SOCK_DGRAM, 0);

	if (new_soc == INVALID_SOCKET)
	{
		DEBUG1("Could not create socket.\n");
		return(-1);
	}

	// clear out socket structure
	memset((void *)&client, '\0', sizeof(struct sockaddr_in));    
	client.sin_family = AF_INET;					// host byte order
	client.sin_port =  htons(port);					// Try to use this port  htons? +++
	/* Set server address */

	client.sin_addr.s_addr= INADDR_ANY;				// Check all IP

	ret=bind(new_soc, (struct sockaddr *)&client, sizeof(struct sockaddr_in));
	
	if(-1==ret)
	{
		// can't use this keep ret=-1
		//ret=1;
	}
	else
	{
		// Get port!
		len=sizeof(client);
		ret=getsockname(new_soc,(struct sockaddr *)&client,(socklen_t *) &len);
		if(-1!=ret)
		{
			ret=htons(client.sin_port);
		}
	}
	// always close the socket 
	closesocket(new_soc);
	return(ret);
}

//
//	IPADDR GetPrimaryIp(void) 
//
//  This seems like the best way to find local IP address!  It doesn't actually send any packets, just finds out which adapter and IP address on the
//	device would be used to send a packet to google DNS.
//
//
//#if defined(LINUX)
//void GetPrimaryIp(char* buffer, size_t buflen) 
IPADDR GetPrimaryIp(void) 
{
int		err=0;
IPADDR	ip;
SOCKET	sock;
struct sockaddr_in serv;

    //assert(sock != -1);
    const char* kGoogleDnsIp = "8.8.8.8";
    U16 kDnsPort = 53;
	ip.ip32=0;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(kGoogleDnsIp);
    serv.sin_port = htons(kDnsPort);

    err = connect(sock, (struct sockaddr*) &serv, sizeof(serv));
	if(err>=0)
	{
		struct sockaddr_in name;
		socklen_t namelen = sizeof(name);
		err = getsockname(sock, (struct sockaddr*) &name, &namelen);
		if(err>=0)
		{
		//assert(err != -1);
			ip.ip32=name.sin_addr.s_addr;
		}
	}
    //const char* p = inet_ntop(AF_INET, &name.sin_addr, buffer, buflen);
    closesocket(sock);

	return(ip);
}


int
read_all(SOCKET sd, U8 *buffer, U16 size)
{
	int totread=0
		,nread,fail_count=0;


            TRACEIN;

	DEBUG0("read all to %d\n",size);

	do
	{
		nread= WebIORecv(sd,(char*) &(buffer[totread]), size - totread, 0);
		DEBUG1("nread %d\n",nread);
		if(0>nread)
		{
			int	j=get_last_error();

			// Should be would block, if weould block, lets sleep and retry
			if(EWOULDBLOCK==j)			//(10060==j)
			{
				DEBUG0("not all read yet, sleep, size is %d of %d\n",totread,size);
				fail_count++;
				if(fail_count>6)
				{
					DEBUG0("Exit on fail\n");
                    
                    TRACEOUT;
					if(totread)
						return(totread);
					else
						return nread;
				}
				ysleep_usec(200000);
				continue;			// Retry Send
			}
			perror("readall\n");
            
            TRACEOUT;
			if(totread)
				return(totread);
			else
				return nread;
		}
		else if(0==nread)
		{
            TRACEOUT;
			if(totread)
				return(totread);
			else
				return nread;
		}
		totread+= nread;
	}	
	while(totread != size);

	if(totread==size)
		DEBUG1("Exit on readall done\n");

    TRACEOUT;
	return(totread);
}

//
// Will read until buffer size has been depleated or until end of matched string
//	should timeout +++
//
int
read_to_string(SOCKET sd, U8* string, U8 *buffer, U16 size)
{
	int		totread=0,nread,strl,i=0;
	char	c;

    TRACEIN;
	
    strl=strlen((char *) string);

	do
	{
		//
		// read one byte at a time,
		//
		nread= WebIORecv(sd, &c,1,0);
		if(nread<=0)
        {
            TRACEOUT;
			return-1;
        }

		if(0==nread)
		{
			continue;
		}

		buffer[totread++]=c;
		//
		// String check
		//
		if(string[i]==c)
		{
			i++;
			if(i==strl)
			{
				break;
			}
		}
		else
			i=0;
	}	
	while(totread != size);

    TRACEOUT;
	return(totread);
}




//  
// Returns total size read,  0 for socket closed, -1 for error, -2 for error
//
// If there is a null line then retunrs 1, but string is null. 
//
int
read_sock_line(SOCKET sd, U8 *buffer, U16 size)
{
	int		nread,tot=0,/*wait=0,*/err=0;
	char	tbuff[2];

	while( (nread=WebIORecv(sd, &tbuff[0], 1, 0))>0 )
	{
		if(0x0d==tbuff[0])
			continue;
		if(0x0a==tbuff[0])
		{
			buffer[tot]=0;
			break;
		}
		else
		{
			buffer[tot++]=tbuff[0];
			if(tot==size)
				break;
		}
	}
	if(nread<0)
	{
		// Check error
		err=get_last_error();
		if(err!=EWOULDBLOCK)
		{
			ysleep_seconds(1);
			return(-2);
		}
	}

	return(tot);

	/*

	if(nread<0)
	{
		// Check error
		err=get_last_error();
		
		if(ret!=EWOULDBLOCK)
			return(-1);

		if(0==wait)
		{
			wait=1;
			ysleep_seconds(1);
		}
		else
			return (nread);

	}

	if(0==nread)
		return(tot);
	return(tot);
*/
}

// int read_sock_web_header(SOCKET sd, U8 *buffer, U16 size, int timeout);
//  Returns a zero terminated buffer, reads until a 0xd 0xa without data (blank line)
//  timeout in seconds
//
// returns len, or 0 for closed socket, -1 for err
//
int
read_sock_web_header(SOCKET sd, char * buffer, int len, int timeout)
{
	int		nread,tot=0,err=0,line=0;//,wait=0;
	char	tbuff[2],*tptr;
	U32		timestamp=second_count();


	if(0==buffer)
		return(-1);

	buffer[0]=0;
	tptr=&buffer[0];

	memset(buffer,(int) '\0',len);

	DEBUG2("read sock web header up to %d len with timeout %d\n",len,timeout);

	// Get start time
	while(1)
	{
		// Read until blank line, len or timeout
		while( (nread=WebIORecv(sd, &tbuff[0], 1, 0))>0 )
		{
			if(0x0d==tbuff[0])
			{
				buffer[tot++]=tbuff[0];
				continue;
			}
			if(0x0a==tbuff[0])
			{
				buffer[tot++]=tbuff[0];
				buffer[tot]=0;
				DEBUG2("line = %s",tptr);
				tptr=&buffer[tot];
				if(0==line)
				{
					break;
				}
				line=0;
			}
			else
			{
				// we are in a line, inc
				line++;
				// copy to buffer
				buffer[tot++]=tbuff[0];
				// check if buffer full
				if(tot==len)
					break;
			}
		}
		DEBUG2("End Copy line len=%d, tot =%d\n",line,tot);
		// Zero Terminate at this point
		buffer[tot]=0;
		if(nread<0)
		{
			DEBUG2("nread<0\n");
			// Check error
			err=get_last_error();
			if(err!=EWOULDBLOCK)
			{
				if( (second_count()-timestamp)> (U32)timeout)
					break;
				ysleep_usec(100000);
			}
		}
		if(0==nread)
			break;
		if(tot==len)
			break;
		if(0==line)
			break;
	}
    DEBUG2("buffer=%s\n",buffer);
	return(tot);
}



#if 0
/*
 * Like connect(), but with an explicit timeout
 */
int connectWithTimeout (struct sockaddr *addr,
                        int addrlen,
                        struct timeval *timeout)
{
    struct timeval sv;
    int svlen = sizeof sv;
    int ret;
	unsigned int fd;

	/* create socket */
	fd = socket(AF_INET,SOCK_STREAM,0);
	if (fd < 0)
	{
		DEBUG0("YOICS_CONFIG:NetConnectD:Can not create socket!\n");
		return YOICS_SO_CANNOT_CREATE_SOCKET;
	}

	/* Connect to server */
	sin.sin_family = AF_INET;
	sin.sin_port = htons(usPort);	//smtp port number
	sin.sin_addr.s_addr = *(unsigned long*)*ppc;

    if (!timeout)
        return connect (sfd, addr, addrlen);
    if (getsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&sv, &svlen) <0)
        return -1;
    if (setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, (char *)timeout, sizeof *timeout) < 0)
        return -1;
    ret = connect (fd, addr, addrlen);
    setsockopt (sfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&sv, sizeof sv);
    return ret;
}

int connectWithTimeout(const char *pcServer, unsigned short usPort, int iMillSecTimeout, int *iOut_fd)
{
	int rc;
	unsigned int fd;
	struct timeval tv,ttv;
	int tvlen = sizeof ttv;
	struct hostent* sHostent = NULL;
	struct sockaddr_in sin;
	char **ppc;


	if (iOut_fd == NULL || pcServer == NULL || iMillSecTimeout <= 0)
	{
		DEBUG0("YOICS_CONFIG:NetConnectD:Illegal parameters while call NetConnect!\n");
		return YOICS_SO_NETCONNECTD_BAD_PARAMS;
	}
	*iOut_fd = -1;

	if ((sHostent = gethostbyname(pcServer)) == NULL)
	{	
		DEBUG0("YOICS_CONFIG:NetConnectD:Failed to resolve host!\n");
		return YOICS_SO_FAILED_TO_RESOLVE_HOST;
	}

	tv.tv_sec = iMillSecTimeout / 1000;
	tv.tv_usec = (iMillSecTimeout - 1000 * tv.tv_sec) * 1000;

	for (ppc=sHostent->h_addr_list; *ppc; ppc++)
	{
		/* create socket */
		fd = socket(AF_INET,SOCK_STREAM,0);
		if (fd < 0)
		{
			DEBUG0("YOICS_CONFIG:NetConnectD:Can not create socket!\n");
			return YOICS_SO_CANNOT_CREATE_SOCKET;
		}

		/* Connect to server */
		sin.sin_family = AF_INET;
		sin.sin_port = htons(usPort);	//smtp port number
		sin.sin_addr.s_addr = *(unsigned long*)*ppc;

		if (!iMillSecTimeout)
		{
			rc=connect(fd,(struct sockaddr *)&sin,sizeof(sin));
		}
		else 
		{
			int slen=50;

			if (getsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, &ttv, &tvlen) <0)
			{
				rc=YOICS_SO_O_NDELAY_MODE_FAILED;
				closesocket(fd);	// was close(fd);
				break;
			}

			if ( setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&slen, sizeof(slen)) )
			//if (setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv) < 0)
			//if (setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, (char *)timeout, sizeof *timeout) < 0)
			{
				rc=YOICS_SO_O_NDELAY_MODE_FAILED;
				closesocket(fd);	// was close(fd);
				break;
			}

			rc = connect(fd,(struct sockaddr *)&sin,sizeof(sin));
			setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&ttv, sizeof tv);
		}

		if(rc>=0)
			return fd;
		else
			rc=-1;


		DEBUG0("MCTEST:NetConnect:Can not connect %s in %d millsecond.\n", pcServer, iMillSecTimeout);
		closesocket(fd);	// was close(fd);
	}

	return rc;
}

#endif


//free_headers()  +++ this needs to actually do the headers
//{
//}

#define MAX_WEB_BUFF	512



// Read the socket until a a newline without any data is hit, read up to 2K and allocate a buffer
char
*read_web_header(SOCKET soc, int timeout)
{
	char	*buffer;

	buffer=malloc(WEB_RESP_HEADER_LEN);
	if(buffer)
	{
	}
	return(buffer);
}



int
get_web_response(SOCKET soc, int timeout, HTTP_HEADER **header)
{
int					ret,rc=0,breaker=0;
char				buffer[MAX_WEB_BUFF];
struct timeval		tv;
fd_set				socks_list; 
//HTTP_HEADER			*l_header,*t_header;
//char				*tptr;


DEBUG4("get web resp soc=%d .\n",soc);

	ret=set_sock_nonblock(soc);


DEBUG4("get web resp1 = %d\n",ret);
	tv.tv_sec = timeout;
	tv.tv_usec = 0;				
DEBUG4("get web resp1.1\n");
	// Select for read on this socket
	FD_ZERO(&socks_list);
DEBUG4("get web resp1.2\n");
	FD_SET(soc, &socks_list);
DEBUG4("get web resp2\n");
	// Use select to wait for read or timeout.
	ret = select(FD_SETSIZE, &socks_list, NULL, NULL, &tv);

	ysleep_seconds(1);
DEBUG4("get web resp3\n");
	if(ret>0)
	{
		DEBUG4("get web resp ret = %d\n",ret);
		// Read HTTP response (-2 is would block)
		while(-2==(ret=read_sock_line(soc,(U8*)buffer,MAX_WEB_BUFF)))
		{
			if(breaker>=timeout)
				break;
			if(-2==ret)
				breaker++;
		}
		if(ret<0)
		{
			return(YOICS_SO_FAIL_READ);
		}
		if(0==ret)
		{
			return(YOICS_SO_SOCKET_CLOSED);
		}
		DEBUG4("toupper\n");
		// Parse Response in the format HTTP/1.1 200 OK
		if(toupper(buffer[0])=='H')
		{
			char	*tbuff=buffer;


			while(*tbuff)
			{
				if(' '==*tbuff++)
					break;
			}
			if(*tbuff)
				rc=atoi(tbuff);				
		}
		else
		{
			return(YOICS_SO_UNKNOWN_ERROR);
		}


		if(0==header)
		{
			// No header requested, don't parse headers, just get to end of
			// Get HTTP headers
			while((ret=read_sock_line(soc,(U8*)buffer,MAX_WEB_BUFF))>0)   //read_sock_line(SOCKET sd, U8 *buffer, U16 size)
				DEBUG0("+> %s\n",buffer);

		}
		else
		{

			// ++ this should be done at some time, lets just write the header to a buffer, parse as needed
			*header=0;
			//l_header=0;
			// Get HTTP headers
			while((ret=read_sock_line(soc,(U8*)buffer,MAX_WEB_BUFF))>0)   //read_sock_line(SOCKET sd, U8 *buffer, U16 size)
			{
				//char *saveptr1, *tptr;

				DEBUG0("+> %s\n",buffer);
#if 0
					//
					// We hae a header allocate a struct
					//
					t_header=malloc(sizeof(HTTP_HEADER));
					t_header->next=0;
					t_header->data=0;
					t_header->type=0;
					//
					tptr=strchr(buffer,':');
					*tptr=0;
					tptr++;
					while(' '==*tptr)
						tptr++;
	
					t_header->type=malloc(strlen(buffer));
					strcpy(t_header->type,buffer);
					t_header->data=malloc(strlen(tptr));
					strcpy(t_header->data,tptr);
				
					if(0==header)
					{
						*header=t_header;
						l_header=t_header;
					}
					else
					{
						l_header->next=t_header;
						l_header=t_header;
					}
#endif
				}
			}//if(0==*header)
//		}
	}
	else if(ret<0)
	{
		DEBUG0("select returned %d with error %d\n",ret,get_last_error());
	}
	else 
	{
		DEBUG0("select returned 0\n");
		rc=YOICS_SO_TIMEOUT;
	}

	return(rc);
}


//
//			Search for needle, malloc and return value
// 			tptr=web_header_parse(resp->header,"CONTENT-LENGTH:");
//
//	Returns a malloc string of the value of the header key.  Must free it if not zero.
//

char
*web_header_parse(char *haystack, char *needle) 
{
	char *tptr,*cptr,*rptr=0,*aptr;
	int nlen,size=0;

    TRACEIN;

	if((0==haystack) || (0==needle))
    {
        if(0==haystack)
            DEBUG2("Haystack is null\n");
        else
            DEBUG2("Needle is null\n");
        TRACEOUT;
		return(NULL);
    }

	nlen=strlen(needle)+1;

	DEBUG2("web header parse find: %s\n in header %s\n",needle,haystack);

	// Search for needle, malloc and return value
	tptr=(char*)strcasestr((const char*)haystack,(const char*)needle);

	if(tptr)
	{
		DEBUG2("found\n");
		// Copy over after needel, until /a or /d, find len first
		cptr=tptr+nlen;
		while((*cptr!=0xa) && (*cptr!=0xd) && (*cptr!=0))
		{
			size++;
			cptr++;
		}
		DEBUG0("size = %d\n",size);
		// Malloc size
		rptr=aptr=(char*)malloc((cptr-tptr)+1);
		if(aptr)
		{
			DEBUG1("malloc\n");
			// Copy
			cptr=tptr+nlen;
			size=0;
			while((*cptr!=0xa) && (*cptr!=0xd) && (*cptr!=0))
			{
				size++;
				*aptr++=*cptr++;
			}
			*aptr=0;
		}
	}
    if(rptr)
    {
	    trim(rptr);
	    DEBUG2("found %s<-------------- size %d\n",rptr,strlen(rptr));
    }
    TRACEOUT;
	return(rptr);
}

//
// Get a response from a web requests, returns a structure HTTP_RESP with return codes or NULL if they structure could not be malloc'ed
// Must call web_response_free(HTTP_RESP *)
//
//

HTTP_RESP
*get_web_response2(SOCKET soc, int timeout)
{
HTTP_RESP			*resp;
int					ret;
//int                 rc=0,breaker=0,header_len=0;
char				buffer[MAX_WEB_BUFF];
struct timeval		tv;
fd_set				socks_list; 
char				*tptr;
U32					timestamp=second_count();

    TRACEIN;
	// Malloc the response
	resp=(HTTP_RESP*)malloc(sizeof(HTTP_RESP));
	if(resp)
	{
		memset(resp,'\0',sizeof(HTTP_RESP));
		resp->header=(char*)malloc(WEB_RESP_HEADER_LEN);
		if(resp->header)
		{
			memset(resp->header,'\0',WEB_RESP_HEADER_LEN);
		}
		else
		{
			free(resp);
			resp=0;
		}
	}
	while(resp)
	{
		// Set the socket nonblock
		ret=set_sock_nonblock(soc);

		tv.tv_sec = timeout;
		tv.tv_usec = 0;				

		// Select for read on this socket
		FD_ZERO(&socks_list);
		FD_SET(soc, &socks_list);

		// Use select to wait for read or timeout.
		ret = select(FD_SETSIZE, &socks_list, NULL, NULL, &tv);

		// Do we want to really sleep?
		//ysleep_seconds(1);

		if(ret>0)
		{
            DEBUG2("Selected\n");
			// First read the response line
			// Read a line
			while(-2==(ret=read_sock_line(soc,(U8*)buffer,MAX_WEB_BUFF)))
			{
				if((second_count()-timestamp)> (U32)timeout)
				{
					resp->resp_code=YOICS_SO_TIMEOUT;
					break;
				}
				ysleep_usec(20000);
			}
			if(ret<0)
			{
				resp->resp_code=YOICS_SO_FAIL_READ;
				break;
			}
			if(0==ret)
			{
				resp->resp_code=YOICS_SO_SOCKET_CLOSED;
				break;
			}
			//
			// Check for a Good Response
			//
			// Parse Response in the format HTTP/1.1 200 OK
			if(0==strncmp(buffer,"HTTP/1.",7))
			{
				int	resp_text_len;
				char parse_buffer[8];
				// We have a good HTTP response, lets get the version, numeric return code, and msg
				parse_buffer[0]=buffer[7];
				parse_buffer[1]=0;
				// Get HTTP version number
				resp->http_version=atoi(parse_buffer);
				DEBUG2("Got HTTP version of 1.%d\n",resp->http_version);
				//
				// Get 3 digit response code
				//
				parse_buffer[0]=buffer[9];
				parse_buffer[1]=buffer[10];
				parse_buffer[2]=buffer[11];
				parse_buffer[3]=0;
				resp->resp_code=atoi(parse_buffer);
				DEBUG2("Got HTTP response code of %d\n",resp->resp_code);
				//
				// Get reponse text
				//
				resp_text_len=strlen(&buffer[13])+1;
				resp->response=malloc(resp_text_len);
				if(resp->response)
				{
					strcpy(resp->response,&buffer[13]);
					trim(resp->response);
					DEBUG2("Got HTTP response of %s\n",resp->response);
				}
			}
			else
			{
				resp->resp_code=YOICS_SO_UNKNOWN_ERROR;
				break;
			}

			//
			// Now Read the header  int read_sock_web_header(SOCKET sd, char * buffer, int len, int timeout)
			//
			ret=read_sock_web_header(soc, resp->header, WEB_RESP_HEADER_LEN, timeout-(second_count()-timestamp));

            DEBUG2("resp header from read_sock_web_header :\n%s\n",resp->header);
			// Good read get length
			tptr=web_header_parse(resp->header,"CONTENT-LENGTH:");
			if(tptr)
			{				
				//DEBUG0("parse %s\n",tptr);
				// Got length, store it
				resp->data_length=atoi(tptr);
				free(tptr);
				DEBUG2("Got Content Length of %d\n",resp->data_length);
			}
            else
            {
                DEBUG2("Did not find Content Length header\n");
            }
		}
		else if(ret<0)
		{
			DEBUG2("select returned %d with error %d\n",ret,get_last_error());
			resp->resp_code=ret;
		}
		else 
		{
			DEBUG2("select returned 0\n");
			resp->resp_code=YOICS_SO_TIMEOUT;
		}
		break;
	}
    TRACEOUT;
	return(resp);
}


int
web_response_free(HTTP_RESP *resp)
{
    TRACEIN;
	if(resp)
	{
		if(resp->data)
			free(resp->data);
		if(resp->header)
			free(resp->header);
		if(resp->response)
			free(resp->response);

		free(resp);
        TRACEOUT;
		return(1);
	}
    TRACEOUT;
	return(0);
}


HTTP_RESP *curl_get(char *host, int port, char *uri, char *extheader, int timeout)
{
int         ret=0,time=0;
SOCKET      soc=0;
char        header[2048];
HTTP_RESP   *resp=0;

    TRACEIN;

    while(1)
    {
        if(0==NetConnect1(host, port, timeout, &soc))
        {
            // Were connected, send request
            snprintf(header,sizeof(header),"GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: Yoics Embedded Web/0.3\r\nConnection: close\r\n%s\r\n",
                uri, host, extheader ? extheader : "");

            DEBUG2("Request :\n%s\n",header);

	        ret=WebIOSend(soc, header, strlen(header), 0);

            // Timeout here is in seconds
            time=timeout/1000;
            if(time<1) time=1;
            resp=get_web_response2(soc, time);

		    if((!resp) || (resp->resp_code<0))
		    {
			    web_response_free(resp);
                resp=0;
			    // Close Socket
			    WebIOClose(soc);
			    //sprintf(authstring,"fail get_web error %d",ret);
			    DEBUG0("Closed no resp\n");
                break;
		    }

            DEBUG2("Response :\n%s\n",resp->data);
            //
            // Lets get the data
            //
            //resp->data=mmalloc(resp->data_len,MALLOC_NET);
            if(resp->data_length)
            {               
                if(resp->data_length>8196)
                    resp->data_length=8196;

                resp->data=(char*)malloc(resp->data_length+1);
                if(0==resp->data)
                {
                    DEBUG1("Failed Malloc curl_get\n");
			        web_response_free(resp);
                    resp=0;
			        // Close Socket
			        WebIOClose(soc);
                    break;
                }
                //resp->data[resp->data_length]=0;
                //
                // read
                //
                ret=read_all(soc, (U8*)resp->data, resp->data_length);
                resp->data[ret]=0;
            }
			// Close Socket, we have good resp
			WebIOClose(soc);
        }
        break;
    }
    TRACEOUT;
    return(resp);
}




int
dump_socket(SOCKET soc, int timeout)
{
char	buffer[MAX_WEB_BUFF];
int		tcount=0;		

    TRACEIN;
	while(0<=read_sock_line(soc,(U8*)buffer,MAX_WEB_BUFF))
	{
		if(strlen(buffer))
		{
			tcount=0;
			printf(">>%s\n",buffer);
		}
		else
		{
			tcount++;
			if(tcount>10)
			{
				printf("tcount break\n");
				break;
		
			}
		}
	}
    TRACEOUT;
	return 0;
}









// Converts a hex character to its integer value 
char from_hex(char ch) 
{
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

// Converts an integer value to its hex character
//
char to_hex(char code) 
{
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_encode(char *str) 
{
	char *pstr = str, *buf = malloc(strlen(str) * 3 + 1), *pbuf = buf;
	while (*pstr) 
	{
		if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') 
			*pbuf++ = *pstr;
		else if (*pstr == ' ') 
			*pbuf++ = '+';
		else 
			*pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
		pstr++;
	}
	*pbuf = '\0';
	return buf;
}

/* Returns a url-decoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_decode(char *str) 
{
	char *pstr = str, *buf = malloc(strlen(str) + 1), *pbuf = buf;
	
	while (*pstr) 
	{
	    if (*pstr == '%') 
		{
			if (pstr[1] && pstr[2]) 
			{
				*pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
				pstr += 2;
			}	
		} 
		else if (*pstr == '+') 
		{ 
			*pbuf++ = ' ';
		} 
		else 
		{
			*pbuf++ = *pstr;
	    }
		pstr++;
	}
	*pbuf = '\0';
	return buf;
}



int
send_with_timeout(SOCKET sock,char *buffer,int len,int flags, int timeout_seconds)
{
int	ret;
		
	set_sock_block(sock);
	set_sock_send_timeout(sock,timeout_seconds);
	ret=WebIOSend(sock, buffer, len, 0);
			
	if(ret<0)
	{
		//DEBUG0("sendfail ret=%d err=\n",ret,get_last_error());
	}
	else if(ret!=len) 
    {
		//printlog(LOG_MISC, "send not full %d of %d bytes\n",ret,len);
	}
	return(ret);
}


//
// return -1 on error or socket
//
SOCKET
udp_listener(U16 port, IPADDR ip)
{
	int					ret;
	SOCKET				new_soc;
	struct sockaddr_in	client;	

	new_soc = socket(AF_INET, SOCK_DGRAM, 0);

	if(new_soc==INVALID_SOCKET)
	{
		return(new_soc);
	}
	// clear out socket structure
	memset((void *)&client, '\0', sizeof(struct sockaddr_in));    
	client.sin_family	= AF_INET;			
	client.sin_port		=  htons(port);				// Use the next port
	/* Set server address */
	client.sin_addr.s_addr=ip.ip32;	// any

	ret=bind(new_soc, (struct sockaddr *)&client, sizeof(struct sockaddr_in));

	if(SOCKET_ERROR==ret)
	{
		perror("error on bind\n");
		printf("error on bind %d\n",get_last_error());
		closesocket(new_soc);
		return(INVALID_SOCKET);
	}
	//
	// We are ready to go!
	//
	return(new_soc);
}

