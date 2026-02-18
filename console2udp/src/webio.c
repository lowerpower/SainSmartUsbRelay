/*!																www.yoics.com			
 *---------------------------------------------------------------------------
 *! \file webio.c
 *  \brief This module provides the I/O functionality for communicating
 *         with the web server.
 *																			
 *---------------------------------------------------------------------------
 * Version                                                                  -
 *		0.1 Original Version March 19, 2008									-        
 *
 *---------------------------------------------------------------------------    
 * Version 
 *
 * 0.1 Original Version March 18, 2008     									-
 *																			-
 * (c)2008 Yoics Inc. All Rights Reserved									-
 *---------------------------------------------------------------------------
 *
 */
 
 /***************************************************************************

	Header Files

****************************************************************************/
#include "config.h"
#include "mytypes.h"
#include "webio.h"
#include "arch.h"
#include "net.h"
//#include "log.h"
#include "debug.h"

/****************************************************************************

	Variables local to this module

*****************************************************************************/

static BOOLEAN	webIOInitialized = FALSE;		// determines if we ned to init

#if defined(WEB_SSL)


#define	WEBIO_SSL_TIMEOUT		10	


static	ssl_context		sslContext;					
static	ssl_session		sslSession;
static	havege_state	sslRNGState;
static	SOCKET			sslSocket;
static	BOOLEAN			sslEnabled = FALSE;

//
// We set SSL to RC4 only for best performance on limited devices
//
int ssl_min_ciphers[] =
{
    SSL_RSA_RC4_128_SHA,
    SSL_RSA_RC4_128_MD5,
	0
};



#endif

/****************************************************************************

	Private functions

****************************************************************************/


#if defined(WEB_SSL)

static void my_debug( void *ctx, int level, const char *str )
{
	/*if (level >= SSL_DEBUG_LEVEL) {
		return;
	}
*/
	printf( "SSL:%s", str);
}

/****************************************************************************

	static int SSL_Init(ssl_context *p_ssl, ssl_session *p_ssn, SOCKET *p_sd);


	This function sets up the ssl context for a connection.

****************************************************************************/
static int SSL_Init(ssl_context *p_ssl, ssl_session *p_ssn, havege_state *p_hs, SOCKET *p_fd)
{

	int ret;

	//
	// Clear SSL session structure
	//
	memset(p_ssn, 0, sizeof(ssl_session));

	//
	// Initialize the ssl context
	//
	if ((ret = ssl_init(p_ssl)) != 0) 
		return ret;

	//
	// Configure ssl debug level, endpoint type, and authmode
	// 
#if defined(DEBUG_SSL)
	ssl_set_dbg(p_ssl,my_debug, stderr);
#endif	
	ssl_set_endpoint(p_ssl, SSL_IS_CLIENT);
	ssl_set_authmode(p_ssl, SSL_VERIFY_NONE);	// NOTE: no CA certs available on embedded targets

	//
	// Seupt random number generator callback
	//
	ssl_set_rng(p_ssl, havege_rand, p_hs);

	//
	// Setup read and write callbacks
	//
	ssl_set_bio(p_ssl, net_recv, p_fd, net_send, p_fd);

	//
	// Setup supported ciphers
	//
	ssl_set_ciphers(p_ssl, ssl_min_ciphers);
	//ssl_set_ciphers(p_ssl, ssl_default_ciphers);

	//
	// Setup SSL session
	//
	ssl_set_session(p_ssl, 1, 600, p_ssn);


//	ssl_handshake(p_ssl);
//	while(XYSSL_ERR_NET_TRY_AGAIN==ssl_handshake(p_ssl))
	//	printf(".");;


	return (0);
}



/*
int 
WebIOProcessSSL_handshake(ssl_context *sslContext,int SecTimeout)
{
	int ret=0;
	

	start_time=second_count();
	while(XYSSL_ERR_NET_TRY_AGAIN==ssl_handshake(&sslContext))
	{
		if((second_count-start_time)>SecTimeout)
		{
			ret=-1;
			break;
		}
	}
	return(ret);
}
*/


#endif


/***************************************************************************

	Public Functions

***************************************************************************/
/*! \fn int WebIOInit(void);

    \brief Initializes the WEB IO module 

    \param  None		
	
	\return 0

	This function does any necessary initialization required by the WEB
	I/O module.

*/
int WebIOInit(void)
{

	if (!webIOInitialized) {

#if defined (WEB_SSL)

		//
		// Set the default SSL state
		//
		WebIOSetSSL(SSL_DEFAULT_STATE);

		//
		// Initialize the randon number generator
		//
		havege_init(&sslRNGState);

#endif

		webIOInitialized = TRUE;
	}

	return (0);

}







//
//
// Return Codes
// -1 = bad webio params
// -2 = Failed to resolve host
// -3 = Cannot Create Socket

int WebIOConnect(const char *pcServer, unsigned short usPort, int iMillSecTimeout, int *iOut_fd)
{
	int rc=-1;
	unsigned int fd;
	struct timeval tv;
	fd_set fdsRead;
	fd_set fdsWrite;
	struct hostent* sHostent = NULL;
	struct sockaddr_in sin;
	char **ppc;
#if defined(WEB_SSL)
	U32 start_time;
	int s,tret,ret;
#endif


	DEBUG0("webIOInit()\n");
	//
	// Make sure we are initialized
	//
	WebIOInit();

	if (iOut_fd == NULL || pcServer == NULL || iMillSecTimeout <= 0)
	{
		DEBUG0("YOICS_CONFIG:NetConnectD:Illegal parameters while call NetConnect!\n");
		return YOICS_SO_NETCONNECTD_BAD_PARAMS;
	}
	*iOut_fd = -1;

	
	DEBUG4("get host by name for %s\n",pcServer);

	if ((sHostent = gethostbyname(pcServer)) == NULL)
	{	
		DEBUG0("YOICS_CONFIG:NetConnectD:Failed to resolve host!\n");
		return YOICS_SO_FAILED_TO_RESOLVE_HOST;
	}

	//
	tv.tv_sec = iMillSecTimeout / 1000;
	tv.tv_usec = (iMillSecTimeout - 1000 * tv.tv_sec) * 1000;

	rc=YOICS_SO_UNKNOWN_ERROR;





	for (ppc=sHostent->h_addr_list; *ppc; ppc++)
	{

		/* create socket */
		fd = socket(AF_INET,SOCK_STREAM,0);
		if (INVALID_SOCKET==fd)
		{
			DEBUG0("YOICS_CONFIG:NetConnectD:Can not create socket!\n");
			return YOICS_SO_CANNOT_CREATE_SOCKET;
		}
		else
		{
			DEBUG1("socket created\n");
		}
		set_sock_nonblock(fd);

		/* Connect to server */
		sin.sin_family = AF_INET;
		sin.sin_port = htons(usPort);	//smtp port number
		sin.sin_addr.s_addr = *(unsigned long*)*ppc;

		DEBUG4("connect\n");

		rc=connect(fd,(struct sockaddr *)&sin,sizeof(sin));

		if(-1==rc)
		{
			int err;

			err=get_last_error();
			if((EWOULDBLOCK!=err) && (EINPROGRESS!=err))
			{
				// Connect error, abort
				DEBUG1("bad connect err %d \n",err);
				closesocket(fd);
				return(YOICS_SO_UNKNOWN_ERROR);
			}
		}


		FD_ZERO(&fdsRead);
		FD_SET(fd, &fdsRead);
		FD_ZERO(&fdsWrite);
		FD_SET(fd, &fdsWrite);

		// Do we need this?
		//ysleep_seconds(2);

		DEBUG4("Select\n");

		if ((rc=select(fd+1, &fdsRead, &fdsWrite, NULL, &tv)) > 0)
		{
			DEBUG4("Out Sel ret=%d\n",rc);
			if (FD_ISSET(fd, &fdsWrite) || FD_ISSET(fd, &fdsRead))
			{
				int iErrorCode;
				int iErrorCodeLen = sizeof(iErrorCode);
				if ( (rc=getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&iErrorCode, (socklen_t *) &iErrorCodeLen)) == 0)
				{
					if (iErrorCode == 0)
					{
						*iOut_fd = fd;
						rc=YOICS_SO_SUCCESS;
#if defined(WEB_SSL)
//#if defined(WEB_SSL)
						//
						// If SSL is enabled set up the ssl context
						//
						if (sslEnabled) {

							int ret_sslinit;

							DEBUG4("SSL Init\n");
			
							sslSocket = fd;
							if ((ret_sslinit = SSL_Init(&sslContext, &sslSession, &sslRNGState, &sslSocket)) != 0)
								return ret_sslinit;


							//
							// connect the SSL transport, we need timeout here
							//
							DEBUG4("process ssl\n");

							// Set timeout, do we need to bail if something goes wrong?
							start_time=second_count();
							while(XYSSL_ERR_NET_TRY_AGAIN==(tret=ssl_handshake(&sslContext)) )
							{
								// Hardcoded to 5 second timeout
								if((second_count()-start_time)>WEBIO_SSL_TIMEOUT)
								{
									DEBUG4("timeout\n");
									rc=YOICS_SO_SSL_TIMEOUT;
									break;
								}
								ysleep_usec(1000);
							}

							DEBUG4("process ssl out, tret=%x\n",tret);
							if(tret!=0)
								rc=YOICS_SO_SOCKET_NO_SSL;

						}
						break;
#else
						break;
#endif
					}
				}
			}
		}
		else
		{
			DEBUG0("MCTEST:NetConnect:Can not connect %s in %d millsecond.\n", pcServer, iMillSecTimeout);
			rc=YOICS_SO_TIMEOUT;
		}
		if(YOICS_SO_SUCCESS!=rc)
		{
			DEBUG4("close!!\n");
			WebIOClose(fd);
		}
		//closesocket(fd);	// was close(fd);
	}

	return rc;
}


#if defined (WEB_SSL)
/*! \fn void WebIOSetSSL(BOOL state);

    \brief Turn SSL on/off.

    \param  BOOL	state		- SSL_ENABLE = on, SSL_DISABLE = off
	
	\return void

	This function enables or disable SSL functionality.
	

*/
void WebIOSetSSL(SSL_STATE state)
{

	//
	// Set the stte varible
	//
	if (state == SSL_ENABLE)
		sslEnabled = TRUE;
	else
		sslEnabled = FALSE;
}
#endif

/*! \fn int WebIOSend(SOCKET sd, char *pBuf, int len, int flags);

    \brief Sends data to the indicated socket. 

    \param SOCKET sd			- destination socket
	\param char *pBuf			- pointer to buffer
	\param int  len				- number of bytes to send
	\param int  flags			- 								
	
	\return >0 number of bytes sent
	\return <0 SOCKET_ERROR

	This function sends the data out the socket indicated. If SSL is enabled 
	the data is sent to the SSL module for output. 
*/
int WebIOSend(SOCKET sd, char *pBuf, int iLen, int flags)
{
	int ret;
#if defined(WEB_SSL)
	U32 start_time;
	if (sslEnabled) 
	{
		// Need timeout?
		start_time=second_count();
		//DEBUG4("inloop wr st=%d\n",start_time);
		while( XYSSL_ERR_NET_TRY_AGAIN==( ret =ssl_write(&sslContext, pBuf, iLen)) )
		{
			DEBUG4("WSEND: again\n");
			// Hardcoded to 5 second timeout
			if((second_count()-start_time)>WEBIO_SSL_TIMEOUT)
			{
				DEBUG4("WSEND: break\n");
				ret=YOICS_SO_TIMEOUT;
				break;
			}
			DEBUG4("WSEND: sleep ret = %d\n",ret);
			ysleep_usec(1000);
		}
	}
	else
		ret = send(sd, pBuf, iLen, flags); 

	DEBUG4("outloop\n");
#else
	ret = send(sd, pBuf, iLen, flags);
#endif
	return ret;

}


/*! \fn int WebIORecv(SOCKET sd, char *pBuf, int len, int flags);

    \brief Reads data from the indicated socket. 

    \param SOCKET sd			- source socket
	\param char *pBuf			- pointer to buffer
	\param int  len				- number of bytes to receive
	\param int  flags			- 								
	
	\return >0 number of bytes received
	\return <0 SOCKET_ERROR

	This function reads data from the indicated socket. If SSL is enabled 
	the data is received from the SSL module. 
*/

int WebIORecv(SOCKET sd, char *pBuf, int iLen, int flags)
{
	int ret;
#if defined(WEB_SSL)
	int start_time;
	if (sslEnabled)
	{
		// While the SSL engine says "try again"; try again. Else return 
		start_time=second_count();
		DEBUG5("inloop rd st=%d\n",start_time);
		while( XYSSL_ERR_NET_TRY_AGAIN==(ret= ssl_read(&sslContext, pBuf, iLen)) )
		{
			// Hardcoded to 5 second timeout
			if((second_count()-start_time)>WEBIO_SSL_TIMEOUT)
			{
				DEBUG4("break\n");
				ret=YOICS_SO_TIMEOUT;
				break;
			}
			DEBUG4("sleep %d\n",second_count()-start_time);
			ysleep_usec(100);
		}
	}
	else
		ret = recv(sd, pBuf, iLen, flags);
#else
		ret = recv(sd, pBuf, iLen, flags);
#endif

	return ret;

}

 

int WebIOClose(SOCKET s)
{
	int ret;

#if defined (WEB_SSL)
		
	if (sslEnabled) {



		ssl_close_notify(&sslContext);
		shutdown(s, 2);
		ret = closesocket(s);
		ssl_free(&sslContext);



	}
	else
		ret = closesocket(s);

#else
	ret = closesocket(s);
#endif

	return ret;


}



