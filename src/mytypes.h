#ifndef MTYPES_H
#define MTYPES_H

//---------------------------------------------------------------------------
// mTypes.h - Mycal standard types											-
//---------------------------------------------------------------------------
// Version                                                                  -
//		0.1 Original Version May 21, 2000									-
//---------------------------------------------------------------------------

#include <limits.h>

#define		swap(x)		(U16)((x>>8) | (x<<8))      
#define		swapl(x) ( (((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) | (((x) & 0x0000ff00) << 8) | (((x) & 0x000000ff) << 24)) 

//
// Lets work for both 32 and 64bit
//
#if UCHAR_MAX == 255u
typedef unsigned char U8;
typedef signed char S8;
typedef unsigned char  u08;
typedef   signed char  s08;
#else
    error Please define uint_8t as an 8-bit unsigned integer type in mytypes.h
#endif


# if USHRT_MAX == 65535u
	typedef unsigned short U16;
	typedef signed short S16;
	typedef unsigned short u16;
	typedef   signed short s16;
#  else
#    error Please define uint_16t as a 16-bit unsigned short type in mytypes.h
#  endif


#if UINT_MAX == 4294967295u
	typedef unsigned int  U32;
	typedef signed int S32;
	typedef unsigned int  u32;
	typedef signed int s32;
#  elif ULONG_MAX == 4294967295u
	typedef unsigned long U32;
	typedef signed long S32;
	typedef unsigned long  u32;
	typedef signed long  s32;
#  elif defined( _CRAY )
#    error This code needs 32-bit data types, which Cray machines do not provide
#  else
#    error Please define uint_32t as a 32-bit unsigned integer type in mytypes.h
#  endif



#  if defined( __BORLANDC__ ) && !defined( __MSDOS__ )
     typedef unsigned __int64 U64;
     typedef unsigned __int64 u64;
     typedef __int64 S64;
     typedef __int64 s64;
#  elif defined( _MSC_VER ) && ( _MSC_VER < 1300 )    /* 1300 == VC++ 7.0 */
     typedef unsigned __int64 U64;
     typedef unsigned __int64 u64;
     typedef __int64 S64;
     typedef __int64 s64;
#  elif defined( __sun ) && defined(ULONG_MAX) && ULONG_MAX == 0xfffffffful
     typedef unsigned long long U64;
     typedef unsigned long long u64;
     typedef long long S64;
     typedef long long s64;
#  elif defined( UINT_MAX ) && UINT_MAX > 4294967295u
#    if UINT_MAX == 18446744073709551615u
       typedef unsigned int U64;
       typedef unsigned int u64;
       typedef int S64;
       typedef int s64;
#    endif
#  elif defined( ULONG_MAX ) && ULONG_MAX > 4294967295u
#    if ULONG_MAX == 18446744073709551615ul
       typedef unsigned long U64;
       typedef unsigned long u64;
       typedef long S64;
       typedef long s64;
#    endif
#  elif defined( ULLONG_MAX ) && ULLONG_MAX > 4294967295u
#    if ULLONG_MAX == 18446744073709551615ull
     typedef unsigned long long U64;
     typedef unsigned long long u64;
     typedef long long S64;
     typedef long long s64;
#    endif
#  elif defined( ULONG_LONG_MAX ) && ULONG_LONG_MAX > 4294967295u
#    if ULONG_LONG_MAX == 18446744073709551615ull
     typedef unsigned long long uint_64t;
     typedef unsigned long long U64;
     typedef unsigned long long u64;
     typedef long long S64;
     typedef long long s64;
#    endif
#  endif



typedef unsigned char BOOLEAN;

#if defined(WIN32) || defined(WINCE)
typedef int		socklen_t;
#else
typedef int		SOCKET;
#endif


typedef struct  _IPADDR
{
    union
    {
        U32             S_addr;     /* IP address as a 32 bit value */
        
        struct                      /* IP address in bytes i.e. 192.168.2.90 */
        {   
            U8          s_b1,       /* byte 1 i.e. 192 (the high order byte) */
                        s_b2,       /* byte 2 i.e. 168 */
                        s_b3,       /* byte 3 i.e. 2   */
                        s_b4;       /* byte 4 i.e. 90  */
        } S_un_b;
        
        struct
        {
            U16         s_w1,       /* 1st 16 bits (low order) */
                        s_w2;       /* 2nd 16 bits */
        } S_un_w;
    } S_un;
} IPADDR;

//typedef IPADDR IPAddr;


#define ip32    S_un.S_addr
#define ipb1    S_un.S_un_b.s_b1
#define ipb2    S_un.S_un_b.s_b2
#define ipb3    S_un.S_un_b.s_b3
#define ipb4    S_un.S_un_b.s_b4
#define ipw1    S_un.S_un_w.s_w1
#define ipw2    S_un.S_un_w.s_w2


typedef U16	Port;



typedef	S16	RET_CODE;
#define	RET_CODE_OK			 	         0
#define	RET_CODE_ABORT			        -1
#define RET_CODE_DONE			        -5
#define RET_CODE_TIMEOUT		        -7
#define RET_CODE_WOULD_BLOCK	        -8
#define	RET_CODE_FAIL			        -9
#define	RET_CODE_INSUFFICIENT_SPACE		-10
#define	RET_CODE_BAD_PARAM				-11
#define RET_CODE_DEV_ABSENT				-12

#endif /* mTypes_H */
