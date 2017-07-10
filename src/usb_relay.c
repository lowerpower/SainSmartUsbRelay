/*
 * SainSmart USB Relay control using hidraw
 *
 * Copyright (c) 2017 mycal 
 *
 */

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
/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

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


#if 0
        cmd.cmd = HID_CMD_READ;
        cmd.len = sizeof(cmd) - 4; /* Not include checksum */
        cmd.arg1 = startPage;
        cmd.arg2 = pages;
        cmd.signature = HID_CMD_SIGNATURE;
        cmd.checksum = CalCheckSum((unsigned char *)&cmd, cmd.len   );

	bRet = io.WriteFile((unsigned char *)&cmd, sizeof(cmd), &length, USB_TIME_OUT);

#endif

//
// calculate_checksum(char * buffer, int len) - additve byte stream checksum with 32 bit accumulator
//
U32
calculate_checksum(char * buffer, int len)
{
    int i;
    U32 checksum=0;

    for(i=0;i<len;i++)
    {
        checksum=buffer[i];
    }
    return(checksum);
}

int
send_command(int fd, HID_COMMAND *hid_cmd)
{
    U8 *buffer;
    U8  *buf;
    int i;
    int ret;

    buf=(U8*)hid_cmd;
    buffer=(U8*)hid_cmd;

    hid_cmd->checksum=0;
    while(buffer<(U8*)&hid_cmd->checksum)
    {
        //printf("adding %x with %x\n",hid_cmd->checksum,*buffer);
        hid_cmd->checksum+=*buffer++;
        //printf("checksum is %x \n",hid_cmd->checksum);
    }
    /*for(i=0;i<sizeof(HID_COMMAND);i++)
        printf(" %x ",buf[i]);
    printf("\n");
    */


    ret = write(fd, hid_cmd, sizeof(HID_COMMAND));
    if (ret < 0) {
        printf("Error: %d\n", errno);
        perror("write");
    } else {
        printf("write() wrote %d bytes\n", ret);
    }
}

//
// Timeout in ms, 0 for nonblock
//
// Returns length of read and fills buffer
//
int
read_data(int fd,char *buf, int buflen, int timeout)
{
    int             res,i,ret;
    int             seconds=0;
    int             useconds=0;
    struct timeval	tv;
    fd_set          fds;

    // If timeout is >0 then wait on select, else fall through to non blocking read
    if(timeout>0)
    {
        // Setup timeout for select, incoming timeout is MS
        seconds=(timeout/1000);                           // convert to seconds
        useconds=(timeout%1000)*1000;                    // Convert to useconds (IE 1unit = 10ms = 10000us)

        //printf("Timeout %d seconds, %d useconds\n",seconds, useconds);
	    memset(&tv,'\0',sizeof(struct timeval));
	    tv.tv_sec = seconds;
        tv.tv_usec = useconds;
        //
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        // Wait on select        
        ret = select(fd+1, &fds, 0, NULL, &tv);
        //printf("select returned %d, fd_max %d\n",ret,fd+1);
    }
    // read
    res = read(fd, buf, buflen);
/*
    if (res < 0) {
        perror("read");
    } else {
        printf("read() read %d bytes:\n\t", res);
        for (i = 0; i < res; i++)
            printf("%hhx ", buf[i]);
        puts("\n");
    } 
*/
    return(res);
}

//
// read the relays current state
//
int 
read_current_state(int fd)
{
    HID_COMMAND hid_cmd;
    char    buffer[128];
    int     ret, value=0;

    hid_cmd.cmd=HID_CMD_READ;
    hid_cmd.len=sizeof(HID_COMMAND) - 4; /* Not include checksum */
    hid_cmd.arg1=0x11111111;
    hid_cmd.arg2=0x11111111;
    hid_cmd.signature =HID_CMD_SIGNATURE;
    hid_cmd.checksum=0;

    send_command(fd,&hid_cmd);
    //sleep(1);
    ret=read_data(fd,buffer,128,2000);
    //
    // Process buffer
    printf("buffer = %x:%x \n",buffer[2],buffer[3]);
    if(ret)
    {
        if(buffer[2]&0x80) value+=1;
        if(buffer[2]&0x40) value+=4;
        if(buffer[2]&0x20) value+=16;
        if(buffer[2]&0x10) value+=64;
        if(buffer[2]&0x08) value+=256;
        if(buffer[2]&0x04) value+=1024;
        if(buffer[2]&0x02) value+=4096;
        if(buffer[2]&0x01) value+=16384;

        if(buffer[3]&0x01) value+=2;
        if(buffer[3]&0x02) value+=8;
        if(buffer[3]&0x04) value+=32;
        if(buffer[3]&0x08) value+=128;
        if(buffer[3]&0x10) value+=512;
        if(buffer[3]&0x20) value+=2048;
        if(buffer[3]&0x40) value+=8192;
        if(buffer[3]&0x80) value+=32768;
    }
    return(value);
}


const char *bus_str(int bus);

int main(int argc, char **argv)
{
    HID_COMMAND hid_cmd;
	int fd;
	int i, res, desc_size = 0;
	char buf[256];
	struct hidraw_report_descriptor rpt_desc;
	struct hidraw_devinfo info;
	char *device = "/dev/hidraw0";

	if (argc > 1)
		device = argv[1];

	/* Open the Device with non-blocking reads. In real life,
	   don't use a hard coded path; use libudev instead. */
	fd = open(device, O_RDWR|O_NONBLOCK);
	//fd = open(device, O_RDWR);

	if (fd < 0) {
		perror("Unable to open device");
		return 1;
	}

	memset(&rpt_desc, 0x0, sizeof(rpt_desc));
	memset(&info, 0x0, sizeof(info));
	memset(buf, 0x0, sizeof(buf));

	/* Get Report Descriptor Size */
	res = ioctl(fd, HIDIOCGRDESCSIZE, &desc_size);
	if (res < 0)
		perror("HIDIOCGRDESCSIZE");
	else
		printf("Report Descriptor Size: %d\n", desc_size);

	/* Get Report Descriptor */
	rpt_desc.size = desc_size;
	res = ioctl(fd, HIDIOCGRDESC, &rpt_desc);
	if (res < 0) {
		perror("HIDIOCGRDESC");
	} else {
		printf("Report Descriptor:\n");
		for (i = 0; i < rpt_desc.size; i++)
			printf("%hhx ", rpt_desc.value[i]);
		puts("\n");
	}

	/* Get Raw Name */
	res = ioctl(fd, HIDIOCGRAWNAME(256), buf);
	if (res < 0)
		perror("HIDIOCGRAWNAME");
	else
		printf("Raw Name: %s\n", buf);

	/* Get Physical Location */
	res = ioctl(fd, HIDIOCGRAWPHYS(256), buf);
	if (res < 0)
		perror("HIDIOCGRAWPHYS");
	else
		printf("Raw Phys: %s\n", buf);

	/* Get Raw Info */
	res = ioctl(fd, HIDIOCGRAWINFO, &info);
	if (res < 0) {
		perror("HIDIOCGRAWINFO");
	} else {
		printf("Raw Info:\n");
		printf("\tbustype: %d (%s)\n",
			info.bustype, bus_str(info.bustype));
		printf("\tvendor: 0x%04hx\n", info.vendor);
		printf("\tproduct: 0x%04hx\n", info.product);
	}

printf("reset to all off\n");

    hid_cmd.cmd=HID_CMD_ERASE;
    hid_cmd.len=sizeof(hid_cmd) - sizeof(hid_cmd.checksum);             //(- checksum)
    hid_cmd.arg1=0x00000071;
    hid_cmd.arg2=0x11110000;
    hid_cmd.signature =HID_CMD_SIGNATURE;
    hid_cmd.checksum=0;

    send_command(fd,&hid_cmd);

printf("write loop\n");

for(i=0;i<16;i++)
{
    hid_cmd.cmd=HID_CMD_WRITE;
    hid_cmd.len=sizeof(hid_cmd) - sizeof(hid_cmd.checksum);
    hid_cmd.arg1=i;//(1<<i);
    hid_cmd.arg2=0x11110000;
    hid_cmd.signature =HID_CMD_SIGNATURE;
    hid_cmd.checksum=0;

    send_command(fd,&hid_cmd);    
    //sleep(1);
    res=read_current_state(fd);
    printf("wrote %x read %x\n",i,res);
    sleep(1);
}

printf("read current state\n");

    hid_cmd.cmd=HID_CMD_READ;
    hid_cmd.len=sizeof(hid_cmd) - sizeof(hid_cmd.checksum);
    hid_cmd.arg1=0x11111111;
    hid_cmd.arg2=0x11111111;
    hid_cmd.signature =HID_CMD_SIGNATURE;
    hid_cmd.checksum=0;

    send_command(fd,&hid_cmd);
    res=read_current_state(fd);
    printf("read %x\n",res);


#if 0
	/* Set Feature */
	buf[0] = 0x9; /* Report Number */
	buf[1] = 0xff;
	buf[2] = 0xff;
	buf[3] = 0xff;
	res = ioctl(fd, HIDIOCSFEATURE(4), buf);
	if (res < 0)
		perror("HIDIOCSFEATURE");
	else
		printf("ioctl HIDIOCGFEATURE returned: %d\n", res);

printf("test3\n");
	/* Get Feature */
	buf[0] = 0x9; /* Report Number */
	res = ioctl(fd, HIDIOCGFEATURE(256), buf);
	if (res < 0) {
		perror("HIDIOCGFEATURE");
	} else {
		printf("ioctl HIDIOCGFEATURE returned: %d\n", res);
		printf("Report data (not containing the report number):\n\t");
		for (i = 0; i < res; i++)
			printf("%hhx ", buf[i]);
		puts("\n");
	}


printf("test4\n");
	/* Send a Report to the Device */
	buf[0] = 0x1; /* Report Number */
	buf[1] = 0x77;
	
	res = write(fd, resetCmd, sizeof(resetCmd));
	if (res < 0) {
		printf("Error: %d\n", errno);
		perror("write");
	} else {
		printf("write() wrote %d bytes\n", res);
	}

	/* Get a report from the device */
	res = read(fd, buf, 26);
	if (res < 0) {
		perror("read");
	} else {
		printf("read() read %d bytes:\n\t", res);
		for (i = 0; i < res; i++)
			printf("%hhx ", buf[i]);
		puts("\n");
	}
#endif

printf("shutdown\n");

	close(fd);
	return 0;
}

const char *
bus_str(int bus)
{
	switch (bus) {
	case BUS_USB:
		return "USB";
		break;
	case BUS_HIL:
		return "HIL";
		break;
	case BUS_BLUETOOTH:
		return "Bluetooth";
		break;
	case BUS_VIRTUAL:
		return "Virtual";
		break;
	default:
		return "Other";
		break;
	}
}


