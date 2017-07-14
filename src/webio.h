/*!																www.yoics.com			
 *---------------------------------------------------------------------------
 *! \file webio.h
 *  \brief Header file for web server IO module
 *																			
 *---------------------------------------------------------------------------
 * Version                                                                  -
 *		0.1 Original Version March 19, 2008									-        
 *
 *---------------------------------------------------------------------------    
 * Version 
 *
 * 0.1 Original Version March 18, 2008     								-
 *																			-
 * (c)2008 Yoics Inc. All Rights Reserved									-
 *---------------------------------------------------------------------------
 *
 */

#ifndef __WEBIO_H__
#define __WEBIO_H__

#if defined(WEB_SSL)

#include <xyssl/net.h>
#include <xyssl/ssl.h>
#include <xyssl/havege.h>

//
// SSL defines
//
typedef	enum {
	SSL_DISABLE	=	0,
	SSL_ENABLE	=	1
} SSL_STATE, *P_SSL_STATE;

//
// SSL State Definitions
//
#define SSL_DEFAULT_STATE	SSL_DISABLE


#endif

//
// Return codes
//
//
// Socket failure codes
//
#define		YOICS_SO_SUCCESS				0	// 0  = default success for lotsuv things.
#define		YOICS_SO_UNKNOWN_ERROR			-1	// -1 = default error for lotsuv things.
#define		YOICS_SO_FAILED_TO_RESOLVE_HOST	-2	// unique error msgs start here.
#define		YOICS_SO_CANNOT_CREATE_SOCKET	-3
#define		YOICS_SO_O_NDELAY_MODE_FAILED	-4
#define		YOICS_SO_NETCONNECTD_BAD_PARAMS	-5
#define		YOICS_SO_TIMEOUT				-6
#define		YOICS_SO_BAD_CREDENTIALS		-7
#define		YOICS_SO_BAD_PARAMS				-8
#define		YOICS_SO_FAIL_READ				-9
#define		YOICS_SO_SOCKET_CLOSED			-10
#define		YOICS_SO_SOCKET_NO_SSL			-11
#define		YOICS_SO_FILE_ERROR				-12
#define		YOICS_SO_SSL_TIMEOUT			-21
//
// Miscellaneous failure codes
//	

#define		YOICS_SUCCESS					0x00000000
#define		YOICS_UNKNOWN_ERROR				0xffffffff
#define		YOICS_USERLIST_MAX_EXCEEDED		0xffff0000
#define		YOICS_OPERATION_FAILED			0xffff1001
#define		YOICS_COULD_NOT_ALLOC_MEMORY	0xffff1002
#define		YOICS_GETUID_FAILED			-3
#define		YOICS_CREATE_DEVICE_FAILED		0xffff2000
#define		YOICS_DELETE_DEVICE_FAILED		0xffff2001
#define		YOICS_CREATE_USER_FAILED		0xffff3000
#define		YOICS_DUPLICATE_USER_NAME		0xffff3001
#define		YOICS_DUPLICATE_EMAIL			0xffff3002
#define		YOICS_INPUT_NOT_VALIDATED		0xffff3003
#define		YOICS_BLANK_USERNAME			0xffff3004
#define		YOICS_BLANK_PASSWORD			0xffff3005
#define		YOICS_BLANK_EMAIL				0xffff3005
#define		YOICS_BLANK_SECURITY_QUESTION	0xffff3006
#define		YOICS_BLANK_SECURITY_ANSWER		0xffff3007





/***************************************************************************

  Global Variables

***************************************************************************/








#ifdef __cplusplus
extern "C" {
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
int WebIOInit(void);

//#if defined(WEB_SSL)

/*! \fn void WebIOSetSSL(BOOL state);

    \brief Turn SSL on/off.

    \param  SSL_STATE	state		- SSL_ENABLE = on, SSL_DISABLE = off
	
	\return void

	This function enables or disable SSL functionality.
	

*/
#if defined(WEB_SSL)
void WebIOSetSSL(SSL_STATE state);
#endif

//#endif






int WebIOConnect(const char *pcServer, unsigned short usPort, int iMillSecTimeout, int *iOut_fd);


/*! \fn int webio_send(SOCKET sd, char *pBuf, int len, int flags);

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

int WebIOSend(SOCKET sd, char *pBuf, int len, int flags);



/*! \fn int webio_recv(SOCKET sd, char *pBuf, int len, int flags);

    \brief Reads data from the indicated socket. 

    \param SOCKET sd			- source socket
	\param char *pBuf			- pointer to buffer
	\param int  len				- number of bytes to send
	\param int  flags			- 								
	
	\return >0 number of bytes received
	\return <0 SOCKET_ERROR

	This function reads data from the indicated socket. If SSL is enabled 
	the data is received from the SSL module. 
*/

int WebIORecv(SOCKET sd, char *pBuf, int len, int flags);


/*! \fn int WebIOClose(SOCKET s)

    \brief Close an SSL socket 

    \param SOCKET sd			- source socket							
	
	\return >0 OK
	\return <0 SOCKET_ERROR

*/ 

int WebIOClose(SOCKET s);



#ifdef __cplusplus
}
#endif

#endif	// __WEBIO_H__
