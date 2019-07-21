#ifndef __YSELECT_H__
#define __YSELECT_H__
//---------------------------------------------------------------------------
// yselect.h - Select Managment Code            							-
//---------------------------------------------------------------------------
// Version                                                                  -
//		0.1 Original Version August 31, 2006     							-
//																			-
// (c)2006 Mycal.net							-
//---------------------------------------------------------------------------


// Initialize the master and working list
void Y_Init_Select();

// set a socket to select
S16 Y_Set_Select_rx(SOCKET sock);
S16 Y_Set_Select_tx(SOCKET sock);


// remove a socket to select
S16 Y_Del_Select_rx(SOCKET sock);
S16 Y_Del_Select_tx(SOCKET sock);

// set the maxfd
int Y_set_fd_max(int max);

// Is a socket selected? 1=rx 2=tx 3=both
S16 Y_Is_Select(SOCKET sock);

S16 Y_Is_Select_rx(SOCKET sock);
S16 Y_Is_Select_tx(SOCKET sock);

// 0 for timeout -1 for error, or the number of bits set from select
int Y_Select(int timeout);

#endif
