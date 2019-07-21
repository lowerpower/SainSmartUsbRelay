/*!																www.mycal.net			
 *---------------------------------------------------------------------------
 *! \yselect.c
 *  \brief Select Managment Code
 *																			
 *---------------------------------------------------------------------------
 * Version                                                                  -
 *		0.1 Original Version April 6, 2006									-        
 *
 *---------------------------------------------------------------------------    
 *                                                             				-
 * Copyright (C) 2006, Mycal.net							-
 *                                                                         	-
 * $Date: mwj 2006/04/06 20:35:55 $
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
#include "yselect.h"
#include "debug.h"

#if !defined(MACOSX) && !defined(IOS)
#define FD_COPY(f, t) memcpy(t, f, sizeof(*(f))) 
#endif

// Socket Select Set
static fd_set		fd_rx_master; 
static fd_set		fd_rx_list;					// temp fdset for listen, FD master copied into it before select
static fd_set		fd_tx_master; 
static fd_set		fd_tx_list;					// temp fdset for listen, FD master copied into it before select
static unsigned int	fd_max;


void Y_Init_Select()
{
	// Initialize the master and working list
	FD_ZERO(&fd_rx_master);
	FD_ZERO(&fd_rx_list);
	FD_ZERO(&fd_tx_master);
	FD_ZERO(&fd_tx_list);
	fd_max=0;
}

S16
Y_Set_Select_rx(SOCKET sock)
{
		if(sock>fd_max)
			fd_max=sock;
		FD_SET(sock, &fd_rx_master);
		return(0);
}

S16
Y_Set_Select_tx(SOCKET sock)
{
		if(sock>fd_max)
			fd_max=sock;
		FD_SET(sock, &fd_tx_master);
		return(0);
}



S16
Y_Del_Select_rx(SOCKET sock)
{
		FD_CLR(sock, &fd_rx_master);
		return(0);
}

S16
Y_Del_Select_tx(SOCKET sock)
{
		FD_CLR(sock, &fd_tx_master);
		return(0);
}

S16
Y_Is_Select_rx(SOCKET sock)
{
    S16 ret = 0;

    if (FD_ISSET(sock, &fd_rx_list))
        ret = 1;
    return(ret);
}

S16
Y_Is_Select_tx(SOCKET sock)
{
    S16 ret = 0;

    if (FD_ISSET(sock, &fd_tx_list))
        ret = 1;
    return(ret);
}

S16
Y_Is_Select(SOCKET sock)
{
    S16 ret=0;

    if(FD_ISSET(sock,&fd_rx_list))
        ret=1;
    if(FD_ISSET(sock,&fd_tx_list))
        ret=ret|2;
    return(ret);
}


int
Y_get_fd_max(void)
{
	return(fd_max);
}

int
Y_set_fd_max(int max)
{
	fd_max=max;
	return(fd_max);
}


//
// Timeout is in MS, 0 = no timeout
//
int
Y_Select(int timeout)
{
    int ret=0;
    int seconds=0;
    int useconds=0;
    struct timeval		tv;

    //
	// Set master list to temp list
	//
	FD_COPY(&fd_tx_master,&fd_tx_list);
	FD_COPY(&fd_rx_master,&fd_rx_list);

	//
	// For everyone else (IE not web proxy) 200ms max
	//
    seconds=(timeout/1000);                           // convert to seconds
    useconds=(timeout%1000)*1000;                    // Convert to useconds (IE 1unit = 10ms = 10000us)

    DEBUG2("Timeout %d seconds, %d useconds\n",seconds, useconds);

	memset(&tv,'\0',sizeof(struct timeval));
	tv.tv_sec = seconds;
    tv.tv_usec = useconds;

    //
    // Wait on select
    //
    //
    ret = select(fd_max+1, &fd_rx_list, &fd_tx_list, NULL, &tv);

	DEBUG2("select returned %d, fd_max\n",ret,fd_max);

    return(ret);
}


