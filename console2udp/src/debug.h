#ifndef __DEBUG_H__
#define __DEBUG_H__

//---------------------------------------------------------------------------
// debug.h - Debug equates for messaging at different debug levels			-
//---------------------------------------------------------------------------
// Version                                                                  -
//		0.1 Original Version Jan 14, 2001									-
//																			-
// (c)2001 Mycal Labs, All Rights Reserved									-
//---------------------------------------------------------------------------

#if defined(WEB_PROXY)
#define DEBUG_LV0	1
#else
//#define DEBUG_LV0	1
#endif

//#define DEBUG_LV1	1                               /* general */
//#define DEBUG_LV2	1                               /* files and yhash */
//#define DEBUG_LV3	1                               /* packets */
//#define DEBUG_LV4	1								/* memory debug */
//#define DEBUG_LV5	1								/*rtt*/
//#define DEBUG_LV9	1								/*Misc temporary debug*/
//#define PACKET_RX_DEBUG 1
	

//#if DEBUG_LV1 || DEBUG_LV2 123
//#include <stdio.h> 
//#endif


//
// Debug level 0 proxy application debug
//
#ifdef DEBUG_LV0
	#define	DEBUG0			yprintf
#else
#ifdef WIN32
    #define		DEBUG0
#else
	#define DEBUG0(...)		//
#endif
#endif
//
// Debug level 1 proxy application debug
//
#ifdef DEBUG_LV1
	#define	DEBUG1			yprintf
#else
#ifdef WIN32
#define		DEBUG1
#else
	#define DEBUG1(...)		//
#endif
#endif

#ifdef DEBUG_LV2
	#define	DEBUG2			yprintf
#else
#ifdef WIN32
#define		DEBUG2
#else
	#define DEBUG2(...)		//
#endif
#endif

#ifdef DEBUG_LV3
	#define	DEBUG3			yprintf
#else
#ifdef WIN32
#define		DEBUG3
#else
	#define DEBUG3(...)		//
#endif	
#endif

#ifdef DEBUG_LV4
	#define	DEBUG4			yprintf
#else
#ifdef WIN32
#define		DEBUG4
#else
	#define DEBUG4(...)		//
#endif	
#endif

// Debug level 5 is for RTT debugging
#ifdef DEBUG_LV5
	#define	DEBUG5			yprintf
#else
#ifdef WIN32
#define		DEBUG5
#else
	#define DEBUG5(...)		//
#endif		//				//
#endif

// Debug level 9 is for testing specific things and is not set to one area.
#ifdef DEBUG_LV9
	#define	DEBUG9			yprintf
#else
#ifdef WIN32
#define		DEBUG9
#else
	#define DEBUG9(...)		//
#endif		//				//
#endif


#endif
