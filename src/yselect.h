#ifndef __YSELECT_H__
#define __YSELECT_H__
//---------------------------------------------------------------------------
// yselect.h - Yoics select code library							-
//---------------------------------------------------------------------------
// Version                                                                  -
//		0.1 Original Version August 31, 2006     							-
//																			-
// (c)2006 Yoics Inc. All Rights Reserved									-
//---------------------------------------------------------------------------


// Initialize the master and working list
void Yoics_Init_Select();

// set a socket to select
S16 Yoics_Set_Select_rx(SOCKET sock);
S16 Yoics_Set_Select_tx(SOCKET sock);


// remove a socket to select
S16 Yoics_Del_Select_rx(SOCKET sock);
S16 Yoics_Del_Select_tx(SOCKET sock);

// set the maxfd
int Yoics_set_fd_max(int max);

// Is a socket selected? 1=rx 2=tx 3=both
S16 Yoics_Is_Select(SOCKET sock);

// 0 for timeout -1 for error, or the number of bits set from select
int Yoics_Select(int timeout);

#endif
