#ifndef __USB_RELAY_H__
#define __USB_RELAY_H__
/*! \file usb-relay.h
    \brief Configuation for usb-relay
*/

#if 0
/*moved to config->h*/
/* Linux */
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>
/* Unix */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
/*project*/
#endif

#include "config.h"
#include "log.h"
#include "arch.h"
#include "net.h"
#include "yselect.h"
#include "daemonize.h"
#include "debug.h"



//#define	VERSION		"0.2"

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int  U32;

//
// Docs to operate  Nuvoton HID controller
// from docs HID_CMD_SIGNATURE 0x43444948
//
// Commands are  xx size a1 a1 a1 a1 a2 a2 a2 a2 + signature + checksum
// Where A1 = argument 1 and A2 is argument 2
// checksum is simple sum.
//
// From https://github.com/Ban3/python-evic/blob/master/evic/device.py
//
// cmd = cmdcode + length + arg1 + arg2 + cls.hid_signature
//

#define USB_VID         0x0416  /* Vendor ID */
#define USB_PID         0x5020  /* Product ID */
#define HID_CMD_SIGNATURE 0x43444948

/* HID Transfer Commands for  Nuvoton hid controller */
#define HID_CMD_NONE     0x00
#define HID_CMD_ERASE    0x71
#define HID_CMD_READ     0xD2
#define HID_CMD_WRITE    0xC3
#define HID_CMD_TEST     0xB4
#define PAGE_SIZE       256
#define SECTOR_SIZE     4096
#define HID_PACKET_SIZE 64

// Maximum relay boards to support
#define MAX_RELAY_BOARDS    16
// 4 hex bytes per 16 channel relay board
#define STATE_SIZE          MAX_RELAY_BOARDS*4 

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)
typedef struct hid_command_
{
	U8	cmd;
	U8	len;
	U32	arg1;
	U32	arg2;
	U32	signature;
	U32	checksum;
}HID_COMMAND;

#pragma pack(pop)   /* restore original alignment from stack */



// description of the relay
typedef struct relay_
{
    char    *device;
    int     fd;
    int     active;     
    int     estate;
}RELAY;

typedef struct connections_
{
    IPADDR  ip;
    U16     port;
    U32     last_used;
    struct connections_ *next;
}CONNECTIONS;


// program config structure
typedef struct relay_config_
{
	IPADDR		control_bind_ip;
	U16			control_port;
    SOCKET		control_soc;    
    char        dev_dir[255];
    char        pidfile[255];

    // number of bards to emulate
    int         emulate;
    //long long   emulation_state;
    char        emulation_state_string[STATE_SIZE];

    CONNECTIONS *connections;

    int         daemonize;
    int         verbose;
    int         board_count;
    //
    U32         hold_start;
    U32         hold_time;
    //
    U32         on_time_start;
    U32         max_on_time;                            // in 100's of ms (tenths of a second)

    struct relay_ relays[MAX_RELAY_BOARDS];
}RELAY_CONFIG;


int send_status_single(RELAY_CONFIG *config, char *message, IPADDR ip, U16 port);
int send_status(RELAY_CONFIG *config, char *message);
int is_device_relay(char *device_directory, char *device_name);
//long long read_bitmask(RELAY_CONFIG *config);
char *read_bitmask(RELAY_CONFIG *config);
int read_current_state(int fd);
const char *bus_str(int bus);
int expire_clients(RELAY_CONFIG *config, int timeout_in_seconds);



#endif


