#ifndef __USB_RELAY_H__
#define __USB_RELAY_H__
/*! \file usb-relay.h
    \brief Configuation for usb-relay
*/

#if 0
/*moved to config.h*/
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



#define	VERSION		"0.1"

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


#define MAX_RELAY_BOARDS    4

// description of the relay
typedef struct relay_
{
    char    *device;
    int     fd;
    int     active;     
}RELAY;

// program config structure
typedef struct relay_config_
{
	IPADDR		control_bind_ip;
	U16			control_port;
    SOCKET		control_soc;    
    char        dev_dir[255];
    char        pidfile[255];

    int         daemonize;
    int         verbose;
    int         board_count;
    int         max_on_time;                            // in 100's of ms (tenths of a second)

    struct relay_ relays[MAX_RELAY_BOARDS];
}RELAY_CONFIG;


int is_device_relay(char *device_directory, char *device_name);
int read_bitmask(RELAY_CONFIG *config);
int read_current_state(int fd);
const char *bus_str(int bus);




#endif


