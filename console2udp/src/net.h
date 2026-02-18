#ifndef __NET_H__
#define __NET_H__
/*! \file net.h
*/

#include	"mytypes.h"


// Header maxes out at 2K
#define	WEB_RESP_HEADER_LEN		(1024*2)
//
// Http Header response, return the ret code, the header in a buffer and the data in a buffer, should free it after use
//
typedef struct http_resp
{
	int					resp_code;				// IE like 200, 400
	int					http_version;			// either 0 or 1 for http 1.0 or 1.1
	int					data_length;
	char				*response;				// HTTP resonse text response: example OK in HTTP/1.0 200 OK
	char				*header;				// Just a malloced buffer that contains the header elements
	char				*data;					// Just a malloc'd buffer that contains the data of the response
}HTTP_RESP;


// Not used
typedef struct http_header_
{
	char					*type;
	char					*data;
	struct http_header_		*next;
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

int set_sock_recv_timeout(SOCKET lsock, int secs);

/*! \fn int get_web_response(SOCKET soc, int timeout, HTTP_HEADER **header)

    \brief Gets a web response

*/
int get_web_response(SOCKET soc, int timeout, HTTP_HEADER **header);

HTTP_RESP *get_web_response2(SOCKET soc, int timeout);
int web_response_free(HTTP_RESP *resp);

/*! \fn int send_with_timeout(SOCKET sock,char *buffer,int len,int flags, int timeout_seconds);

    \brief Send on a socket with timeout

*/
int send_with_timeout(SOCKET sock,char *buffer,int len,int flags, int timeout_seconds);


/*! \fn int read_sock_line(SOCKET sd, U8 *buffer, U16 size);

    \brief Read a line from the a socket

*/
int read_sock_line(SOCKET sd, U8 *buffer, U16 size);

/*! \fn int read_sock_line(SOCKET sd, U8 *buffer, U16 size, int timeout);

    \brief Read a line from the a socket

*/
int
read_sock_web_header(SOCKET sd, char * buffer, int len, int timeout);


/*! \fn int get_last_error(void);

    \brief Cross Platform get the last error

*/
int get_last_error(void);


/*! \fn int read_all(SOCKET sd, U8 *buffer, U16 size);

    \brief Read an amount of data from the a socket

*/
int read_all(SOCKET sd, U8 *buffer, U16 size);
/*! \fn struct hostent*	resolve(char * name)
    \brief Returns the DNS address information of the passwd host name.

    \param name Hostname string.
	\return 0 if failed, or a pointer to the hostent structure with DNS information
*/
IPADDR  resolve(char *name);

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_encode(char *str);
/* Returns a url-decoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_decode(char *str);


HTTP_RESP *curl_get(char *host, int port, char *uri, char *extheader, int timeout);
int NetConnect1(const char *pcServer, unsigned short usPort, int iMillSecTimeout, SOCKET *iOut_fd);

int test_udp_bind(U16 port);
SOCKET udp_listener(U16 port, IPADDR ip);

IPADDR GetPrimaryIp(void);


#endif
