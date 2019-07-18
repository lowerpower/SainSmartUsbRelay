/*
 * SainSmart USB Relay control using hidraw
 *
 * Copyright (c) 2017 mycal 
 *
 */

#include "usb-relay.h"
//#include "net.h"
//#include "yselect.h"

// global go flag
volatile sig_atomic_t 	go  =1;
int                     is_daemon=0;


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

#if defined(LINUX)
int
send_command(int fd, HID_COMMAND *hid_cmd)
{
    U8 *buffer;
    //U8  *buf;
  //  int i;
    int ret;

    //buf=(U8*)hid_cmd;
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
       // printf("write() wrote %d bytes\n", ret);
    }
    return(ret);
}



int write_state(int fd,int bitmask,int verify)
{
HID_COMMAND hid_cmd;
int count,res;
int ret=0;


    hid_cmd.cmd=HID_CMD_WRITE;
    hid_cmd.len=sizeof(hid_cmd) - sizeof(hid_cmd.checksum);
    hid_cmd.arg1=bitmask;
    hid_cmd.arg2=0x11110000;
    hid_cmd.signature =HID_CMD_SIGNATURE;
    hid_cmd.checksum=0;

    send_command(fd,&hid_cmd);   

    if(verify)
    {
        count=0;
        while(bitmask!=(res=read_current_state(fd)) )
        {
            usleep(100000);
            if(count++>5)
            {
                printf("failed to verify\n");
                ret=-1;
                break;
            }
            printf("retry %d\n",count);
        }
    }

    return(ret);
}

//
// Timeout in ms, 0 for nonblock
//
// Returns length of read and fills buffer
//
int
read_data(int fd,char *buf, int buflen, int timeout)
{
    int             res,ret;
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
        ret=select(fd+1, &fds, 0, NULL, &tv);
        if (ret)
        {
            DEBUG1("select returned %d, fd_max %d\n", ret, fd + 1);
        }
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
// read the relays current state of one board
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
    //printf("buffer = %x:%x \n",buffer[2],buffer[3]);
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




int 
reset_board(int fd)
{
    HID_COMMAND hid_cmd;
    //char    buffer[128];

    hid_cmd.cmd=HID_CMD_ERASE;
    hid_cmd.len=sizeof(hid_cmd) - sizeof(hid_cmd.checksum);             //(- checksum)
    hid_cmd.arg1=0x00000071;
    hid_cmd.arg2=0x11110000;
    hid_cmd.signature =HID_CMD_SIGNATURE;
    hid_cmd.checksum=0;

    send_command(fd,&hid_cmd);

    return(0);
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


int is_device_relay(char *device_directory, char *device_name)
{
    int ret=0;
	int fd;
	int res;
    char device[256];
    struct hidraw_devinfo info;

    //strcpy(device,"/dev/");
    strcpy(device,device_directory);
    strcat(device,device_name);

	fd = open(device, O_RDWR|O_NONBLOCK);
	//fd = open(device, O_RDWR);

	if (fd < 0) {
		perror("Unable to open device");
		return (-1);
	}

	memset(&info, 0x0, sizeof(info));

	/* Get Raw Info */
	res = ioctl(fd, HIDIOCGRAWINFO, &info);
	if (res < 0) {
		perror("HIDIOCGRAWINFO");
	} else {
		//printf("Raw Info:\n");
		//printf("\tbustype: %d (%s)\n",
		//	info.bustype, bus_str(info.bustype));
        
        // check vendor and product for what we are looking for
        
        if((USB_VID==info.vendor) && (USB_PID==info.product))
            ret=1;
	}
    close(fd);
    return(ret);
}

#endif

int find_relay_devices(RELAY_CONFIG *config)
{
    int ret=-1;
	int fd;
#if defined(LINUX)
    DIR *dp;
    struct dirent *ep;     
#endif

    // if emulation, just set the estate and relay to active for the number of boards 
    //  specified.
    if(config->emulate)
    {
        int i;

        for(i=0;i<config->emulate;i++)
        {
            // set board as active
            config->relays[config->board_count].active=1;
            // zero device state
            config->relays[config->board_count].estate=0;
            // Inc count
            config->board_count++;
        }
        return(0);
    }
#if defined(LINUX)
    // Open the devices directory
    dp = opendir (config->dev_dir);

    if (dp != NULL)
    {
        while ( (ep = readdir (dp)) )
        {
            // check if we got a device we are looking for hidraw*
            if(0==strncmp(ep->d_name,"hidraw",strlen("hidraw")) )
            {
                if(is_device_relay(config->dev_dir,ep->d_name))
                {
                    // this is one of ours, lets store it
                    config->relays[config->board_count].device=malloc(strlen(ep->d_name)+strlen(config->dev_dir)+1);
                    if(config->relays[config->board_count].device)
                    {
                        // create device name
                        strcpy(config->relays[config->board_count].device,config->dev_dir);
                        strcat(config->relays[config->board_count].device,ep->d_name);
                        // initialze FD for this device
	                    fd = open(config->relays[config->board_count].device, O_RDWR|O_NONBLOCK);

	                    if (fd < 0) {
		                    perror("Unable to open device");
                            free(config->relays[config->board_count].device);
                            config->relays[config->board_count].device=0;
		                    continue;
	                    }
                        config->relays[config->board_count].fd=fd;

                        reset_board(fd);

                        // Set as active
                        config->relays[config->board_count].active=1;

                        // Inc count
                        ret=config->board_count++;

                        if(config->verbose) printf("setup relay board %d.\n",config->board_count);
                    }
                    else
                    {
                        perror ("Could not malloc relay device name.");
                        ret=-1;
                        break;
                    }
                }
            }
        }
        (void) closedir (dp);
    }
    else
        perror ("Couldn't open the directory");
#endif
    return(ret);
}

int
extract_board_state(char *set_state, int board_number)
{
    int start,i;
    int value = 0,tv;
    int set_state_len = strlen(set_state);

    // get the 4 bytes from the set state that corrispond to the board number
    // IE set 3000200010000000 would be 4 boards with board 0=0000 and board 3=3000
    start = board_number * 4;
    for (i = start; (i < start + 4 && i<set_state_len); i++)
    {
        tv = hex2bin(set_state[i]);
        tv = tv << ((3-(i-start)) * 4);
        value += tv;
    }
   
    return(value);
}

// support 16 boards, or increae the string
char
*write_bitmask(RELAY_CONFIG *config, char *set_state)
{
    int i;
    int board_set_state;

    for (i = 0; i < config->board_count; i++)
    {
        // get current state from set_state for board
        board_set_state = extract_board_state(set_state, i);


        if (config->verbose > 1) printf("writing to board %d bitmask %x from raw %x\n", i, board_set_state, board_set_state);

        if (config->emulate)
        {
            // estate is the state for the particular relay
            //config->relays[i].estate = 
            config->relays[i].estate = board_set_state;
            ysleep_usec(5000);
        }
        else
        {
#if defined(LINUX)
            write_state(config->relays[i].fd, board_set_state, 1);
#endif
        }
    }

    return(read_bitmask(config));
}

// currently supports 4 board when compiled with 64 bit
/*
long long
write_bitmask(RELAY_CONFIG *config,long long bitmask)
{
int i;
    // currently supports 4 board when compiled with 64 bit
    for(i=0;i<config->board_count;i++)
    {
        if(config->verbose>1) printf("writing to board %d bitmask %llx from raw %llx\n",i,(bitmask>>(i*16))&0xffff,bitmask);

        if(config->emulate)
        {
            config->relays[i].estate=(bitmask>>(i*16))&0xffff;
            ysleep_usec(5000);
        }
        else
            write_state(config->relays[i].fd,(bitmask>>(i*16))&0xffff,1);
    }
    return(read_bitmask(config));
}
*/

char *
read_bitmask(RELAY_CONFIG *config)
{
    int i,j;
    char four_bytes[5];
    int t, ret = 0;
    int index = 0;
    

    // currently supports 4 board when compiled with 64 bit
    //for (i = 0; i < config->board_count; i++)
    for (i = config->board_count,j=0; i>0 ;i--,j++)
    {
        if (config->emulate)
        {
            // we flip to network order here because a string is like network order
            t = htons(config->relays[i-1].estate);
            printf("=>%x\n", t);

            bin2hexstr(&t, &config->emulation_state_string[j * 4], 2);
            ysleep_usec(1000);
        }
        else
        {
            //   t = read_current_state(config->relays[i].fd);
            t = 0;
        }
       // ret |= (long long)(t << (i * 16));
    }
    return(config->emulation_state_string);
}

/*
long long
read_bitmask(RELAY_CONFIG *config)
{
int i;
long long t,ret=0;
    // currently supports 4 board when compiled with 64 bit
    for(i=0;i<config->board_count;i++)
    {
        if(config->emulate)
        {
            t=config->relays[i].estate;
            ysleep_usec(1000);
        }
        else
            t=read_current_state(config->relays[i].fd);
        
        ret|=(long long)(t<<(i*16));
    }
    return(ret);
}
*/


int
sanity_test(RELAY_CONFIG *config)
{
    int i;
    char bit_string[STATE_SIZE];

    for(i=0;i<config->board_count*16;i++)
    {
       // write_bitmask(config,(long long)1<<i);
        sprintf(bit_string, "%lx", (long long)1 << i);
        write_bitmask(config, bit_string);
        ysleep_usec(500000);
    }
    return(0);
}
//
// Warning this assumes that passed string has buffer space of at least 3 chars before the passed pointer, in this case
// we should only get here when there are 4 characters (IE set_)
//
char *padd_set_string(char *subst)
{
    int i,len,mod;
    char *ret = subst;

    len = strlen(subst);
    // Padd
    mod = len % 4;
    for (i = 0; i < (4-mod); i++)
    {
        *--ret = '0';
    }
    return(ret);
}

// currently supports 4 board when compiled with 64 bit

// propose:
//int *process_command(RELAY_CONFIG *config, char *cmd, chat *replybuffer)
//
// Returns -1 do not send anything
//          0 send back to only the sender
//          1 send back to all
//
int 
process_command(RELAY_CONFIG *config, char *cmd, char *replybuffer)
//char
//*process_command(RELAY_CONFIG *config, char *cmd)
{
    int     ret = -1;
    char	*subst;
    char	*strt_p;

    replybuffer[0]=0;

    // parse in_buffer for command, we use while here so we can break on end of parse or error
	while(strlen((char *) cmd)>0)
    {
        if(config->verbose>1)  printf("process command %s\n",cmd);
        // Lets parse, if no subst, exit
        subst=strtok_r((char *) cmd," \n",&strt_p);
        if(0==subst)
            break;
        // Check Commands
        if(0==strcmp("quit",subst))
        {
            if(config->verbose)
            {
                if(config->daemonize)
                {
                    syslog(LOG_INFO,"shutting down by quit command\n");
                }
                else
                    printf("shutting down by quit command\n");
            }
            go=0;
            strcpy(replybuffer,"shutting down (quit sent)\n");
            ret = 1;
        }
        else if (0 == strcmp("disconnect", subst))
        {
            strcpy(replybuffer, "disconnect\n");
            ret = 0;
        }
        else if (0 == strcmp("ping", subst))
        {
            strcpy(replybuffer, "pong\n");
            ret = 0;
        }
        else if(0==strcmp("get",subst))
        {
            // Get Current State
            sprintf(replybuffer, "%s\n", read_bitmask(config));
            //sprintf(replybuffer,"%llx\n",read_bitmask(config));
            ret = 0;
        }
        else if(0==strcmp("set",subst))
        {
            // pasrse off the value
            subst=strtok_r(NULL," \n",&strt_p);  
            if(0==subst)
            {
                sprintf(replybuffer,"error - no value to set\n");
            }
            else
            {
                int tlen;
                char *set_string;
                //subst needs to be padded out to multiple of 4
                set_string = padd_set_string(subst);
                // set value
                //value=strtoll(subst, NULL, 16);
                write_bitmask(config,set_string);
                // read back
                //sprintf(replybuffer,"%llx\n",read_bitmask(config));
                sprintf(replybuffer, "%s\n", read_bitmask(config));
                ret = 1;

                // See if there is a hold time
                subst=strtok_r(NULL," \n",&strt_p);  
                if(0!=subst)
                {
                    // Get the hold time
                    config->hold_time=atoi(subst);
                    //config->hold_start=hund_ms_count();
                    config->hold_start=ms_count();
                    syslog(LOG_INFO,"hold time %d",config->hold_time);
                    if(config->verbose>1)  printf(" hold time %d\n",config->hold_time);
                }
                // we set on-time-start on any set
                config->on_time_start = ms_count();
            }
        }
        else if(0==strcmp("play",subst))
        {
            // Play A File
            //sprintf(replybuffer,"%x\n",read_bitmask(config));
        }
        break;
    }
    return(ret);
}


void
clear_all(RELAY_CONFIG *config)
{
    char        *state;
    int         ret,i;

	config->on_time_start=0;

    // check to make sure we are all zero

    state=read_bitmask(config);

    for(i=0;i<strlen(state);i++)
    {
        if ('0' == state[i])
            continue;
        else
        {
            char    ret_str[256], cmd[127];
            strcpy(cmd, "set 0");
            ret = process_command(config, cmd, ret_str);

            DEBUG1("process command of set 0 returned %d\n", ret);

            if (config->control_port)
            {
                if (ret >= 0)
                    send_status(config, ret_str);
            }
            else
            {
                printf("%s", ret_str);
                fflush(stdout);
            }
        }
        break;
    }
}

//
// Add udp client or update its last use.
//
int
//add_client(RELAY_CONFIG *config, IPADDR ip, U16 port)
add_client(RELAY_CONFIG *config, struct sockaddr_in *client)
{
    int ret = 2;
    IPADDR ip;
    U16 port=htons(client->sin_port);
    CONNECTIONS *tconn = config->connections;

    ip.ip32=client->sin_addr.s_addr;

    while (tconn)
    {
        if ((tconn->port == port) && (tconn->ip.ip32==ip.ip32))
        {
            tconn->last_used = second_count();
            ret=0;
            if(config->verbose>1)  printf("timer reset, endpoint %d.%d.%d.%d:%d already exists\n", ip.ipb1,ip.ipb2,ip.ipb3,ip.ipb4,port);
            break;
        }
        tconn = tconn->next;
    }
    if (2 == ret)
    {
       // Malloc and add to head of list
        tconn = malloc(sizeof(CONNECTIONS));
        if (tconn)
        {
            tconn->port = port;
            tconn->ip.ip32 = ip.ip32;
            tconn->next = config->connections;
            tconn->last_used = second_count();
            config->connections = tconn;
            if(config->verbose>1)  printf("added endpoint %d.%d.%d.%d:%d\n", ip.ipb1,ip.ipb2,ip.ipb3,ip.ipb4,port);
        }
        else
        {
           if(config->verbose>1)  printf("FAILED: to added endpoint %d.%d.%d.%d:%d\n", ip.ipb1,ip.ipb2,ip.ipb3,ip.ipb4,port);
           ret = -1;
        }
    }
    return(ret);
}

CONNECTIONS *
lookup_client(RELAY_CONFIG *config, IPADDR ip, U16 port)
{
    return(0);
}

int
remove_connection(RELAY_CONFIG *config, CONNECTIONS *remove_connection)
{
    CONNECTIONS *tconnection;
    int         extracted = 0;

    tconnection=config->connections;
    // first extract the connection from the list
    if ((tconnection) && (remove_connection))
    {
        if (tconnection == remove_connection)
        {
            // easy, at the head of the list
            config->connections = tconnection->next;
            extracted = 1;
        }
        else
        {
            // lets search through the list to remove the connection
            while (tconnection->next)
            {
                if (tconnection->next == remove_connection)
                {
                    // found, remove
                    tconnection->next = remove_connection->next;
                    extracted = 1;
                    break;
                }
                tconnection = tconnection->next;
            }
        }
        if (extracted)
        {
            free(remove_connection);
        }
    }
    else
    {
        printf("remove_connections: input is invalid, do nothing.\n");
    }
    return(extracted);
}

int
expire_clients(RELAY_CONFIG *config, int timeout_in_seconds)
{
    CONNECTIONS *tconnection, *current_connection;
    int ret = 0;

    if (config->verbose > 1) printf("expire_called\n");

    // loop through active connections and see if there is any processing to do.
    tconnection = config->connections;

    while (tconnection)
    {
        // setup working connection
        current_connection = tconnection;
        // and next connection (this is so we can cleanup easy witout loosing next item to process)
        tconnection = tconnection->next;

        // Check Timeout
        if (((second_count() - current_connection->last_used) > timeout_in_seconds) || (0 == timeout_in_seconds))
        {
            if (config->verbose > 1) printf("connection has expired (port %d), lets cleanup\n",current_connection->port);
            if (remove_connection(config, current_connection))
            {
                ret++;
            }
            else
            {
                printf("this should not happen, expire_clients\n");
            }
        }
    }
    return(ret);
}


int
send_status_single(RELAY_CONFIG *config, char *message, IPADDR ip, U16 port)
{
    int     ret;
    struct sockaddr_in peer;

    // Setup Send
    memset((void *)&peer, '\0', sizeof(struct sockaddr));
    peer.sin_family = AF_INET;
    peer.sin_addr.s_addr = ip.ip32;
    peer.sin_port = htons(port);

    // Do the send
    ret = sendto(config->control_soc, (char *)message, strlen(message), 0, (struct sockaddr *)&peer, sizeof(struct sockaddr));
    if (ret)
    {
    }
    else if (ret < 1)
    {
    }
    return(ret);
}

int
send_status(RELAY_CONFIG *config, char *message)
{
    int                 ret;
    // Loop through and send back to all UDP endpoints that we have a listing for
    CONNECTIONS         *tconn = config->connections;
    int                 count = 0;

    if (strlen(message))
    {
        if (config->verbose > 1)  printf("send->%s\n", message);
        // Loop through and send back to all UDP endpoints that we have a listing for
        while (tconn)
        {
            ret = send_status_single(config, message, tconn->ip, tconn->port);
            if (ret < 0)
            { 
                // Failed to send
            }
            else
            {
                count++;
                //DEBUG1("count at %d\n", count);
            }
            tconn = tconn->next;
        }
    }
    DEBUG1("out\n");
    return(count);
}



void
startup_banner()
{
	//------------------------------------------------------------------
	// Print Banner
	//------------------------------------------------------------------
	printf("SainSmartUsbRelay " __DATE__ " at " __TIME__ "\n");
	printf("   Version " VERSION " - https://github.com/lowerpower/SainSmartUsbRelay\n");
	fflush(stdout);	
}


void usage(int argc, char **argv)
{
  startup_banner();

  printf("usage: %s [-h] [-v(erbose)] [-c udp_command_port] bitmask \n",argv[0]);
  printf("\t -h this output.\n");
  printf("\t -v verbosity.\n");
  printf("\t -t run test.\n");
  printf("\t -c command port (defaults 1026)\n");
  printf("\t -e emulate number of boards (no hardware needed)\n");
  printf("\t -c command port (defaults 1026)\n");

  exit(2);
} 

void
termination_handler(int signum)
{

    go = 0;

    if ((SIGFPE == signum) || (SIGSEGV == signum) || (11 == signum))
    {
        yprintf("Terminated from Signal %d\n", signum);
        if (is_daemon) syslog(LOG_ERR, "Terminated from Signal 11\n");

#if defined(BACKTRACE_SYMBOLS)
        {
            // addr2line?                
            void* callstack[128];
            int i, frames = backtrace(callstack, 128);
            char** strs = backtrace_symbols(callstack, frames);
            yprintf("backtrace:\n");
            for (i = 0; i < frames; ++i)
            {
                yprintf("T->%s\n", strs[i]);
                if (is_daemon)  syslog(LOG_ERR, "T->%s\n", strs[i]);
            }
            free(strs);
            fflush(stdout);
        }
#endif
        exit(11);
    }
}

int main(int argc, char **argv)
{
	int c, test=0;
    int active;
    U32                   timer15;
    RELAY_CONFIG config_s;
    RELAY_CONFIG *config = &config_s;
   

   /* printf("mscount = %ld\n",ms_count());
    printf("mscount = %ld\n",ms_count());
    ysleep_seconds(1);
    printf("mscount = %ld\n",ms_count());
   exit(1); 
    */
    
    // Initialize config
    memset(config,0,sizeof(RELAY_CONFIG));    
    strcpy(config->dev_dir,"/dev/");
    config->max_on_time=2500;                          // 2 seconds
    //config->control_port=1026;                       // default UDP port 0 (off)
    
    // Parse Command Line
	while ((c = getopt(argc, argv, "c:e:tvh")) != EOF)
	{
    		switch (c) 
			{
    		case 'c':
    		    //control port
    		    config->control_port = atoi(optarg);
                if(config->verbose) printf ("setting control port to %d\n",config->control_port);
    			break;
    		case 'v':
    			config->verbose++;
    			break;
    		case 't':
                test=1;
    			break;
            case 'e':
                config->emulate=atoi(optarg);
                if(config->emulate)
                {
                    if(config->emulate>MAX_RELAY_BOARDS)
                    {
                        if(config->verbose) printf ("requested to emulating %d relay boards, set to max %d\n",config->emulate, MAX_RELAY_BOARDS);
                        config->emulate=MAX_RELAY_BOARDS;
                    }
                    else
                        if(config->verbose) printf ("emulating %d relay boards\n",config->emulate);
                }
                else
                {
                    if(config->verbose) printf ("emulate specified but set to 0 boards\n");
                }
                break;
    		case 'h':
    			usage (argc,argv);
    			break;
    		default:
    			usage (argc,argv);
				break;
    	}
    }
	argc -= optind;
	argv += optind;

    network_init();

    // First search /dev/hidraw* for relay devices
    if(config->verbose) printf ("Discover Relay Boards on USB bus\n");
    find_relay_devices(config);
    if(config->verbose) printf ("%d SainSmart USB Relay Boards Found.\n",config->board_count);

    if(config->control_port)
    {
	    config->control_soc=udp_listener(config->control_port,config->control_bind_ip);

	    if(config->control_soc!=SOCKET_ERROR)
	    {
		    if(config->verbose) printf("Control port %d Bound to port %d.%d.%d.%d:%d\n",config->control_port,config->control_bind_ip.ipb1,config->control_bind_ip.ipb2,config->control_bind_ip.ipb3,config->control_bind_ip.ipb4,config->control_port);
            // nonblock on sock
	        set_sock_nonblock(config->control_soc);
            // Add to select
            Yoics_Set_Select_rx(config->control_soc);
	    }
	    else
	    {
		    if(config->verbose) printf("Failed to bindt %d, error %d cannot Startup\n",config->control_port,get_last_error());
		    perror("bind udp socket\n");
            exit(2);
	    }
    }
    // cycle test
    if(test)
    {
        if(config->verbose) printf("Testing each relay of all boards (%d) in order\n",config->board_count);
             
        sanity_test(config);
        exit(0);
    }

#if defined(LINUX)
    //
    // Should Daemonize here, only if we are UDP enabled
    //
	if(config->daemonize)
	{
            if(config->verbose) printf("starting as daemon\n");
            if(config->control_port)
            {
                // Daemonize this
                daemonize(config->pidfile,0,0,0,0,0,0);

                // set global daemon flag
                is_daemon=1;
                // Setup logging
			    openlog("usb-relay",LOG_PID|LOG_CONS,LOG_USER);
			    syslog(LOG_INFO,"SainSmartUsbRelay "__DATE__ " at " __TIME__ "\n");
			    syslog(LOG_INFO,"   Version " VERSION " - https://github.com/lowerpower/SainSmartUsbRelay\n");
			    syslog(LOG_INFO,"Starting up as daemon\n");
            }
            else
            {
                perror("Must be UDP server to Daemonize\n");
                exit(1);
            }
    }

	//------------------------------------------------------------------
	// Initialize error handling and signals
	//------------------------------------------------------------------
	if (signal (SIGINT, termination_handler) == SIG_IGN)
		signal (SIGINT, SIG_IGN);
	if (signal (SIGTERM, termination_handler) == SIG_IGN)
		signal (SIGTERM, SIG_IGN);
	if (signal (SIGILL , termination_handler) == SIG_IGN)
		signal (SIGILL , SIG_IGN);
	if (signal (SIGFPE , termination_handler) == SIG_IGN)
		signal (SIGFPE , SIG_IGN);
	if (signal (SIGSEGV , termination_handler) == SIG_IGN)
		signal (SIGSEGV , SIG_IGN);
	if (signal (SIGXCPU , termination_handler) == SIG_IGN)
		signal (SIGXCPU , SIG_IGN);
	if (signal (SIGXFSZ , termination_handler) == SIG_IGN)
		signal (SIGXFSZ , SIG_IGN);
#endif

    timer15 = second_count();

    // While active, take command and set relays
	if(config->verbose) printf("Starting interactive command loop\n");	
    while(go)
    {
        // do not process any more until hold time is over
        if ((0 == config->hold_time) || (ms_count() - config->hold_start) > config->hold_time)
        {
            if (config->control_port)
            {
                //
                // Wait on select, 100ms, chance YS to ms paramters
                //
                if (config->verbose > 2)  printf("call select\n");
                active = Yoics_Select(1000);
                if (active)
                {
                    char    *ret_str, cmd[1024],replybuffer[1024];
                    int     slen, ret;
                    struct sockaddr_in	client;
                    // Read command from udp socket
                    memset(&client, '\0', sizeof(struct sockaddr));
                    slen = sizeof(struct sockaddr_in);
                    ret = recvfrom(config->control_soc, cmd, 1024 - 2, 0, (struct sockaddr *)&client, (socklen_t *)&slen);
                    if (ret > 0)
                    {
                        cmd[ret] = 0;
                        if (config->verbose > 1)  printf("Got %d bytes.\n", ret);
                        //int add_client(RELAY_CONFIG *config, IPADDR ip, U16 port);
                        add_client(config, &client);

                        // we have a packet, let process it
                        ret = process_command(config, cmd, replybuffer);
                        if (1==ret)
                            send_status(config, replybuffer);
                        else
                        {
                            IPADDR ip;
                            U16     port;
                            ip.ip32 = client.sin_addr.s_addr;
                            port = htons(client.sin_port);
                            // send only to current
                            send_status_single(config, replybuffer, ip, port);
                        }
                    }
                    else
                    {
                        if (config->verbose > 1)  printf("no read active=%d\n", active);
                    }


                    //ret_str=process_command(config,cmd);
                    // back to socket not printf
                    if (config->verbose > 1) printf("-->%s", replybuffer);
                }
            }
            else
            {
                char    cmd[1024],replybuffer[1024];
                int     ret;
                // stdio command processor

                // Sleep for 10ms becaue we are not waiting on select, kbhit should be modified to take a timeout
                // and we wouldnt need this
                //
                //ysleep_usec(10000);

                // only check after hold time
                //if((0==config->hold_time) ||  (hund_ms_count()-config->hold_start)>config->hold_time)
               // if((0==config->hold_time) ||  (ms_count()-config->hold_start)>config->hold_time)
                //{
                config->hold_time = config->hold_start = 0;
                if (kbhit())
                {
                    if (config->verbose) printf("kbhit\n");
                    readln_from_a_file((FILE*)stdin, (char *)cmd, 1024 - 2);
                    //ret_str = process_command(config, cmd);

                    ret=process_command(config, cmd, replybuffer);
                    if (ret>=0)
                    {
                        if (config->verbose > 1) printf("blabal\n");
                        printf("%s", replybuffer);
                        //printf("%s", ret_str);
                        fflush(stdout);
                    }
                    //config->on_time_start=hund_ms_count();
                    //config->on_time_start = ms_count();
                }
                else
                {
                    //clear?
                    clear_all(config);
                }
                //   }
            }
        }

        //config->max_on_time
        // Make sure everything is off based on max on time
        //if((config->on_time_start) && ((hund_ms_count()-config->on_time_start)>config->max_on_time) )
        if((config->on_time_start) && ((ms_count()-config->on_time_start)>config->max_on_time) )
		{			
            if (config->verbose > 1) printf("clear config all\n");
			clear_all(config);	
		}
        //
        // Expire
        //
        // Every 15 seconds
        if ((second_count() - timer15) >= 15)
        {
            timer15 = second_count();
            expire_clients(config, 360);
        }
    }

    exit(0);
}

