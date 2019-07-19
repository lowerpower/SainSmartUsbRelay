#ifndef MTYPES_H
#define MTYPES_H

//---------------------------------------------------------------------------
// mTypes.h - Mycal standard types											-
//---------------------------------------------------------------------------
// Version                                                                  -
//		0.1 Original Version May 21, 2000									-
//---------------------------------------------------------------------------


#define		swap(x)		(U16)((x>>8) | (x<<8))      
#define		swapl(x) ( (((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) | (((x) & 0x0000ff00) << 8) | (((x) & 0x000000ff) << 24)) 

typedef unsigned char U8;
typedef signed char S8;
typedef unsigned short U16;
typedef signed short S16;
typedef unsigned long U32;
typedef signed long S32;

#if defined(WIN32) || defined(WINCE)
typedef	__int64				S64;
typedef	unsigned __int64	U64;
#else
typedef long long			S64;
typedef unsigned long long	U64;
#endif
// datatype definitions macros
typedef unsigned char  u08;
typedef   signed char  s08;
typedef unsigned short u16;
typedef   signed short s16;
typedef unsigned long  u32;
typedef   signed long  s32;
typedef unsigned long u64;
typedef   signed long s64;

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
