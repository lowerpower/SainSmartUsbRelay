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
#include "webio.h"
#include "net.h"
#include "debug.h"
#include "log.h"




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

#if defined(LINUX) || defined(MACOSX)

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

#if defined(LINUX) || defined(MACOSX)

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
	struct timeval tv;

#if defined(WINDOWS)
	int timeout=secs*100;
	ret = setsockopt(lsock,SOL_SOCKET ,SO_SNDTIMEO,(char *)&timeout,sizeof(timeout));
#else
	tv.tv_sec = secs;
	tv.tv_usec = 0;
	if ( (ret=setsockopt(lsock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)) ) < 0)
	{
		
	}
#endif
	return(ret);
}


int
set_sock_recv_timeout(SOCKET lsock, int secs)
{
	int ret=-1;
	struct timeval tv;

	tv.tv_sec = secs;
	tv.tv_usec = 0;
	if ( (ret=setsockopt(lsock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) ) < 0)
	{
		
	}
	return(ret);
}


int get_last_error()
{
#if defined(WIN32) || defined(WINCE)
	return(WSAGetLastError());
#endif

#if defined(LINUX) || defined(MACOSX) || defined(__ECOS)
	return(errno);
#endif
}



// Socket Select Set statics
static fd_set		fd_master_rx; 
static fd_set		fd_list_rx;					// temp fdset for listen, FD master copied into it before select
static fd_set		fd_master_tx; 
static fd_set		fd_list_tx;					// temp fdset for listen, FD master copied into it before select

void Select_Init()
{
	// Initialize the master and working list
	FD_ZERO(&fd_master_rx);
	FD_ZERO(&fd_list_rx);
	FD_ZERO(&fd_master_tx);
	FD_ZERO(&fd_list_tx);
}


/*! \fn Select_RX_Add(SOCKET sock);
    \brief Adds a socket to the rx select list
	
	\param SOCKET sock	-	socket you wish to add to the select list

	\return 0 = no error
*/
int
Select_RX_Add(SOCKET sock)
{
		FD_SET(sock, &fd_master_rx);
		return(0);
}

/*! \fn Select_RX_Remove(SOCKET sock);
    \brief Removes a socket from the rx select list
	
	\param SOCKET sock	-	socket you wish to remove to the select list

	\return 0 = no error
*/
int
Select_RX_Remove(SOCKET sock)
{
		FD_CLR(sock, &fd_master_rx);
		return(0);
}


/*! \fn Select_Check(SOCKET sock);
    \brief Checks if a socket has been selected
	
	\param SOCKET sock	-	socket you wish to check

	\return 0 = no error
*/
S16
Select_RX_Check(SOCKET sock)
{
	if(FD_ISSET(sock,&fd_list_rx))
		return(1);						// Important to return either 1 or 0 directly because of lencth issues
	else
		return(0);
}

/*! \fn Select_TX_Check(SOCKET sock);
    \brief Checks if a socket has been selected
	
	\param SOCKET sock	-	socket you wish to check

	\return 0 = no error
*/
S16
Select_TX_Check(SOCKET sock)
{
	if(FD_ISSET(sock,&fd_list_tx))
		return(1);						// Important to return either 1 or 0 directly because of lencth issues
	else
		return(0);
}


/*! \fn Select_RX();
    \brief Use this in the select call, copies the lists, and returns a set pointer to the list to use in the call
	
	\param SOCKET sock	-	socket you wish to check

	\return 0 = no error
*/
fd_set *Select_RX()
{
    //
	// Set master list to temp list
	//
	FD_COPY(&fd_master_rx,&fd_list_rx);
    return(&fd_list_rx);
}

/*! \fn Select_TX();
    \brief Use this in the select call, copies the lists, and returns a set pointer to the list to use in the call
	
	\param <None>

	\return 0 = no error
*/
fd_set *Select_TX()
{
    //
	// Set master list to temp list
	//
	FD_COPY(&fd_master_tx,&fd_list_tx);
    return(&fd_list_tx);
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
struct hostent		*host_info;



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

		host_info=gethostbyname(name);

		if(host_info!=0)
		{
			DEBUG0("OK\n");
			fflush(stdout);	
			answer.ip32=*((unsigned long*)host_info->h_addr);
		}
		else
		{
			DEBUG0("fail code %d\n",h_errno);
			fflush(stdout);	

#if defined(UCLINUX)
	//		res_init();
#endif

			answer.ip32=0;
		}
#endif
	}
	else
	{
		answer.ip32=inet_addr(name);
//		answer=gethostbyaddr((char*)&addr,sizeof(addr),AF_INET);
	}

	return(answer);
}


// *** depricated ***
// get_local_ip(adapter) - get the local IP address from the adapter.  Useful mostly on
//		embedded platforms.  Better to use new method (get_our_ip)... 
//
IPADDR
get_local_ip(U8 *adapter)
{
	IPADDR	answer;
	 char hostname[80];

#if defined(UCLINUX)
	struct  sockaddr_in *our_ip;
	int	fd;
	struct ifreq r;

	sleep(1);
	fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	strcpy(r.ifr_name,adapter);
	ioctl(fd, SIOCGIFADDR, &r);
    our_ip = (struct sockaddr_in *) &r.ifr_addr;
	answer.ip32=our_ip->sin_addr.s_addr;
	close(fd);
	DEBUG1("our IP = %s\n",inet_ntoa(our_ip->sin_addr));

#else

#if defined(MACOSX) || defined(WIN32) || defined(LINUX)|| defined(WINCE)
  if (gethostname(hostname, sizeof(hostname)) != SOCKET_ERROR) 
  {
		answer=resolve(hostname);
  }
#else
	answer.ip32=0;
#endif
#endif

	return(answer);
}



int
read_all(SOCKET sd, U8 *buffer, U16 size)
{
	int totread=0
		,nread;

	do
	{
		nread= WebIORecv(sd, (char*)&(buffer[totread]), size - totread, 0);
		if(nread<=0)
			return -1;
		totread+= nread;
	}	
	while(totread != size);

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

	strl=strlen((char *) string);

	do
	{
		//
		// read one byte at a time,
		//
		nread= WebIORecv(sd, &c,1,0);
		if(nread<=0)
			return-1;

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

	return(totread);
}



int
read_sock_line(SOCKET sd, U8 *buffer, U16 size)
{
	int		nread,tot=0,err=0;
    //int     wait=0;
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
	//if(0==nread)
	//	return(tot);
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


//free_headers()
//{
//}

#define MAX_WEB_BUFF	512
int
get_web_response(SOCKET soc, int timeout, HTTP_HEADER **header)
{
int					ret,rc=0,breaker=0;
char				buffer[MAX_WEB_BUFF];
struct timeval		tv;
fd_set				socks_list; 
//HTTP_HEADER			*l_header;



	set_sock_nonblock(soc);

	tv.tv_sec = timeout;
	tv.tv_usec = 0;				

	// Select for read on this socket
	FD_ZERO(&socks_list);
	FD_SET(soc, &socks_list);

	// Use select to wait for read or timeout.
	ret = select(FD_SETSIZE, &socks_list, NULL, NULL, &tv);

	ysleep_seconds(1);

	if(ret>0)
	{
		// Read HTTP response
		while(-2==(ret=read_sock_line(soc,(U8*)buffer,MAX_WEB_BUFF)))
		{
			if(timeout>=breaker)
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


		// Right now only parse headers of web response is OK!
//		if(200==rc)
//		{
			if(0==header)
			{
				// No header requested, don't parse headers, just get to end of
				// Get HTTP headers
				while((ret=read_sock_line(soc,(U8*)buffer,MAX_WEB_BUFF)>0))   //read_sock_line(SOCKET sd, U8 *buffer, U16 size)
					DEBUG0("+> %s\n",buffer);

			}
			else
			{

				// ++ this should be done at some time
				*header=0;
				//l_header=0;
				// Get HTTP headers
				while((ret=read_sock_line(soc,(U8*)buffer,MAX_WEB_BUFF)>0))   //read_sock_line(SOCKET sd, U8 *buffer, U16 size)
				{
					//printf("+> %s\n",buffer);
#if 0
                    char *saveptr1, *tptr;
                    HTTP_HEADER			*t_header;
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
						header=t_header;
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


int
dump_socket(SOCKET soc, int timeout)
{
char	buffer[MAX_WEB_BUFF];
int		tcount=0;		

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
		printlog(LOG_MISC, "send not full %d of %d bytes\n",ret,len);
	
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
		printf("error on bind for port %d error %d\n",port,get_last_error());
		closesocket(new_soc);
        return(-1);
	}
    //
    // Set socket tx/rx buffer
    //
	if(1){
				// open up the send buffer in windows to 256K
		int s;		
		int i;
		int iLen = sizeof(i);

        getsockopt(new_soc, SOL_SOCKET, SO_RCVBUF, (char*) &i, (socklen_t *)&iLen);
        DEBUG5("rxbuffer=%d\n",i);

        // Bump buffers to 512K
		s=1024*512;
		setsockopt(new_soc, SOL_SOCKET, SO_SNDBUF, (char*) &s, sizeof(s));
		setsockopt(new_soc, SOL_SOCKET, SO_RCVBUF, (char*) &s, sizeof(s));

        getsockopt(new_soc, SOL_SOCKET, SO_RCVBUF, (char*) &i, (socklen_t *)&iLen);
        DEBUG5("rxbuffer=%d\n",s);
	}


	//
	// We are ready to go!
	//
	return(new_soc);
}

