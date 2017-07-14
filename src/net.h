#ifndef __NET_H__
#define __NET_H__
/*! \file net.h
*/

#include	"mytypes.h"
#include	"webio.h"

#if !defined(MACOSX) && !defined(IOS)
#define FD_COPY(f, t) memcpy(t, f, sizeof(*(f))) 
#endif

#if defined(LINUX) || defined(MACOSX) || defined(__ECOS)

#define SOCKET_ERROR		-1
#define INVALID_SOCKET		-1
#define closesocket			close

#define TRUE				1 
#define FALSE				0

#endif

//
// Http Header Processing Structs
//
typedef struct http_resp
{
	int					http_type;
	int					resp_code;
	struct http_header_	*header;
}HTTP_RESP;

typedef struct http_header_
{
	char				*type;
	char				*data;
	struct header_		*next;
}HTTP_HEADER;



/*! \fn int network_init(void)
	
	\brief Setup for network access if needed.

	\return <0 SOCKET_ERROR
*/
int network_init(void);



/*! \fn int set_sock_nonblock(SOCKET lsock)


    \brief Sets a socket to a be non-blocking

    \param SOCKET sd			- socket to act on							
	
	\return >0 number of bytes received
	\return <0 SOCKET_ERROR

*/
int set_sock_nonblock(SOCKET lsock);


/*! \fn int get_web_response(SOCKET soc, int timeout, HTTP_HEADER **header)

    \brief Gets a web response

*/
int get_web_response(SOCKET soc, int timeout, HTTP_HEADER **header);


/*! \fn int send_with_timeout(SOCKET sock,char *buffer,int len,int flags, int timeout_seconds);

    \brief Send on a socket with timeout

*/
int send_with_timeout(SOCKET sock,char *buffer,int len,int flags, int timeout_seconds);


/*! \fn int read_sock_line(SOCKET sd, U8 *buffer, U16 size);

    \brief Read a line from the a socket

*/
int read_sock_line(SOCKET sd, U8 *buffer, U16 size);


/*! \fn int get_last_error(void);

    \brief Cross Platform get the last error

*/
int get_last_error(void);


/*! \fn int set_sock_recv_timeout(SOCKET lsock, int secs);;

    \brief Sets the Receive Timeout on socket.

*/
int set_sock_recv_timeout(SOCKET lsock, int secs);


/*! \fn SOCKET udp_listener(U16 port);

    \brief Binds a UDP port for listen (don't use)

*/
SOCKET udp_listener(U16 port, IPADDR ip);

#endif
