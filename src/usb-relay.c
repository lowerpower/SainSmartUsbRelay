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

int
send_command(int fd, HID_COMMAND *hid_cmd)
{
    U8 *buffer;
    U8  *buf;
  //  int i;
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
        ret=select(fd+1, &fds, 0, NULL, &tv);
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
    char    buffer[128];
    int     ret, value=0;

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

int find_relay_devices(RELAY_CONFIG *config)
{
    int ret=-1;
	int fd;
    DIR *dp;
    struct dirent *ep;     

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
                        config->board_count++;

                        if(config->verbose) printf("setup relay board %d.\n",config->board_count);
                    }
                    else
                    {
                        perror ("Could not malloc relay device name.");
                    }

                }
            }
        }
        (void) closedir (dp);
    }
    else
        perror ("Couldn't open the directory");

    return(ret);
}


// currently supports 4 board when compiled with 64 bit
long long
write_bitmask(RELAY_CONFIG *config,long long bitmask)
{
int i;
    // currently supports 4 board when compiled with 64 bit
    for(i=0;i<config->board_count;i++)
    {
        if(config->verbose>1) printf("writing to board %d bitmask %llx from raw %llx\n",i,(bitmask>>(i*16))&0xffff,bitmask);
        write_state(config->relays[i].fd,(bitmask>>(i*16))&0xffff,1);
    }
    return(read_bitmask(config));
}

long long
read_bitmask(RELAY_CONFIG *config)
{
int i;
long long t,ret=0;
    // currently supports 4 board when compiled with 64 bit
    for(i=0;i<config->board_count;i++)
    {
        t=read_current_state(config->relays[i].fd);
        ret|=(long long)(t<<(i*16));
    }
    return(ret);
}

int
sanity_test(RELAY_CONFIG *config)
{
    int i;

    for(i=0;i<config->board_count*16;i++)
    {
        write_bitmask(config,(long long)1<<i);
        ysleep_usec(500000);
    }
    return(0);
}


// currently supports 4 board when compiled with 64 bit
char
*process_command(RELAY_CONFIG *config, char *cmd)
{
    char	*subst;
    char	*strt_p;
    static char    return_message[1024];

    return_message[0]=0;
    // parse in_buffer for command, we use while here so we can break on end of parse or error
	while(strlen((char *) cmd)>0)
    {
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
            strcpy(return_message,"OK - shutting down\n");
        }
        else if(0==strcmp("get",subst))
        {
            // Get Current State
            sprintf(return_message,"%llx\n",read_bitmask(config));
        }
        else if(0==strcmp("set",subst))
        {
            // pasrse off the value
            subst=strtok_r(NULL," \n",&strt_p);  
            if(0==subst)
            {
                sprintf(return_message,"error - no value to set\n");
            }
            else
            {
                long long value;
                // set value
                value=strtoll(subst, NULL, 16);
                write_bitmask(config,value);
                // read back
                sprintf(return_message,"%llx\n",read_bitmask(config));
            }
        }
        else if(0==strcmp("play",subst))
        {
            // Play A File
            //sprintf(return_message,"%x\n",read_bitmask(config));
        }
        break;
    }
    return(return_message);
}


void
termination_handler (int signum)
{

	go=0;	

    if((SIGFPE==signum) || (SIGSEGV==signum) || (11==signum))
    {
        yprintf("Terminated from Signal %d\n",signum);
		if(is_daemon) syslog(LOG_ERR,"Terminated from Signal 11\n");

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
                    if(is_daemon)  syslog(LOG_ERR,"T->%s\n", strs[i]);
                }
                free(strs);
                fflush(stdout);
              }
#endif
        exit(11);
    }
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
  printf("\t -v verbosity\n");
  printf("\n -t run test\n");
  printf("\t -c command port (defaults 1026)\n");

  exit(2);
}

int main(int argc, char **argv)
{
    //HID_COMMAND hid_cmd;
	//int fd;
	//char buf[256];
	//struct hidraw_report_descriptor rpt_desc;
	//struct hidraw_devinfo info;
	//char *device = "/dev/hidraw0";

	int c, test=0;
    RELAY_CONFIG config;

    
    // Initialize config
    memset(&config,0,sizeof(RELAY_CONFIG));    
    strcpy(config.dev_dir,"/dev/");
    config.max_on_time=10;
    //config.control_port=1026;                       // default UDP port 1026

    // Parse Command Line
	while ((c = getopt(argc, argv, "c:tvh")) != EOF)
	{
    		switch (c) 
			{
    		case 'c':
    		    //control port
    		    config.control_port = atoi(optarg);
                if(config.verbose) printf ("setting control port to %d\n",config.control_port);
    			break;
    		case 'v':
    			config.verbose++;
    			break;
    		case 't':
                test=1;
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



    // First search /dev/hidraw* for relay devices
    if(config.verbose) printf ("Discover Relay Boards on USB bus\n");
    find_relay_devices(&config);
    if(config.verbose) printf ("%d SainSmart USB Relay Boards Found.\n",config.board_count);

    if(config.control_port)
    {
	    config.control_soc=udp_listener(config.control_port,config.control_bind_ip);

	    if(config.control_soc!=SOCKET_ERROR)
	    {
		    if(config.verbose) printf("Control port %d Bound to port %d.%d.%d.%d:%d\n",config.control_port,config.control_bind_ip.ipb1,config.control_bind_ip.ipb2,config.control_bind_ip.ipb3,config.control_bind_ip.ipb4,config.control_port);
            // nonblock on sock
	        set_sock_nonblock(config.control_soc);
            // Add to select
            Yoics_Set_Select_rx(config.control_soc);
	    }
	    else
	    {
		    if(config.verbose) printf("Failed to bindt %d, error %d cannot Startup\n",config.control_port,get_last_error());
		    perror("bind\n");
            exit(2);
	    }
    }
    // cycle test
    if(test)
    {
        if(config.verbose) printf("Testing each relay of all boards (%d) in order\n",config.board_count);
             
        sanity_test(&config);
        exit(0);
    }

    //
    // Should Daemonize here, only if we are UDP enabled
    //
	if(config.daemonize)
	{
            if(config.verbose) printf("starting as daemon\n");
            if(config.control_port)
            {
                // Daemonize this
                daemonize(config.pidfile,0,0,0,0,0,0);

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


    // While active, take command and set relays
	if(config.verbose) printf("Starting interactive command loop\n");	
    while(go)
    {
        int active;

        if(config.control_port)
        {
            //
            // Wait on select, 100ms, chance YS to ms paramters
            //
            printf("call select\n");
            active = Yoics_Select(1000);
            if(active)
            {
                char    *ret_str,cmd[127];
                // Read command
                ret_str=process_command(&config,cmd);
                // back to socket not printf
                printf("%s",ret_str);
            }
        }
        else
        {
            char    *ret_str,cmd[127];
            // stdio command processor
            if(kbhit())
            {
                            if(config.verbose) printf("kbhit\n");
                readln_from_a_file((FILE*)stdin, (char *)cmd, 128);
                ret_str=process_command(&config,cmd);
                printf("%s",ret_str);
		fflush(stdout);
            }
        }



        //config.max_on_time
        // Make sure everything is off based on max on time
	/*	if((second_count()-timestamp)>config.max_on_time)
		{
            int value;
			timestamp=second_count();
            // failsafe off
            value=read_bitmask(RELAY_CONFIG *config);
            if(value)

		}
        */
    }

    exit(0);


}




