/*!								www.mycal.net
-----------------------------------------------------------------------------
telnet_udp.c

Sends command line UDP messages and waits for a reply, or operates interactivly
like telnet except uses UDP as a transport.

Command line can send broadcast in addition to unicast

(c)2009-2011 Mycal.net
-----------------------------------------------------------------------------
*/
#include "config.h"
#include "net.h"
#include "arch.h"

#define CMD_MAX_SIZE 1024


#if defined(LINUX) || defined(MACOSX)
int _kbhit(void)
{
	struct timeval tv;
	fd_set read_fd;

	tv.tv_sec=0;
	tv.tv_usec=100;

	FD_ZERO(&read_fd);
	FD_SET(0,&read_fd);

	if(select(1, &read_fd, NULL, NULL, &tv) == -1)
		return 0;

	if(FD_ISSET(0,&read_fd))
		return 1;

	return 0;
}
#endif


void usage(int argc, char **argv)
{
  printf("usage: %s [-p port] [-h host] [-v(erbose)] [-?(this message)]\n",argv[0]);
  printf("	Defaults port=1024, host=127.0.0.1, verbose off\n");
  exit(0);
}


int main(int argc, char *argv[])
{
	int					ci,verbose=0,send_broadcast=0,go,slen;
	int					ret,sd;
	U16					destport;
	struct sockaddr_in	server;
    struct sockaddr_in	client;
	struct hostent		*host_info;
	IPADDR				ip,our_ip;
	char				message[4096];
    char                command_buffer[CMD_MAX_SIZE];
    char	            *strt_p;
	int                 broadcast = 1;	

	// Set defaults
    our_ip.ip32=0;
	ip.ipb1=127;
	ip.ipb2=0;
	ip.ipb3=0;
	ip.ipb4=1;
	destport=1024;
	// Standard Config
	memset(message,0,4096);

	//------------------------------------------------------------------
	// Argument Scan command line args, first for -h -b
	//------------------------------------------------------------------

	for(ci=1;ci<argc;ci++)
	{
		if(0 == strncmp(argv[ci], "-p", 2))
		{
			// Get port
			ci++;
			if(argc==ci)
				usage(argc,argv);

			destport=atoi(argv[ci]);
		}
		else if(0 == strncmp(argv[ci], "-h", 2))
		{
			U8		*subst;		
	
			// Get host, check if IP or Name
			ci++;
			if(argc==ci)
				usage(argc,argv);
			
			if(isalpha(argv[ci][0]))
			{
				// convert name to IP
				host_info=gethostbyname(argv[ci]);
				if(host_info!=0)
				{
					ip.ip32=*((unsigned long*)host_info->h_addr);
				}
				else
				{
					printf("Failed to resolve %s\n",argv[ci]);
					exit(0);
				}
				//if(verbose)
                                //        printf("Sending to %s or %d.%d.%d.%d\n",argv[ci],ip.ipb1,ip.ipb2,ip.ipb3,ip.ipb4);
			}	
			else
			{
				// get proxy target IP		
                subst=(U8 *) strtok_r(argv[ci],".\n",&strt_p);		
				if(strlen((char *) subst))
					ip.ipb1=atoi((char *) subst);
				subst=(U8 *) strtok_r(NULL,".\n",&strt_p);
				if(strlen((char *) subst))
					ip.ipb2=atoi((char *) subst);
				subst=(U8 *) strtok_r(NULL,".\n",&strt_p);
				if(subst)
					ip.ipb3=atoi((char *) subst);
				subst=(U8 *) strtok_r(NULL,".\n",&strt_p);
				if(subst)
					ip.ipb4=atoi((char *) subst);
				//if(verbose)
				//	printf("Sending to %d.%d.%d.%d\n",ip.ipb1,ip.ipb2,ip.ipb3,ip.ipb4);
			}
		}
        else if(0 == strncmp(argv[ci], "-b", 2))
        {
            send_broadcast=1;
        	ip.ipb1=255;
        	ip.ipb2=255;
        	ip.ipb3=255;
        	ip.ipb4=255;
	    }
		else if(0 == strncmp(argv[ci], "-v", 2))
		{
			// Verbose
			verbose=1;
			printf("verbose on\n");
		}
		else if(0 == strncmp(argv[ci], "-?", 2))
		{
			// print help
			usage(argc,argv);
		}
		else
		{
			// No more args break
			break;
		}
	}

    // we should verify we have a minimum set.




	if(argc > ci)
	{
		int i;

		//
		// Build Message
		//
		for(i=ci;i<argc;i++)
		{

			// Check if we fit in tweet
			// stlen argv+1 for space
			if((strlen(argv[i])+strlen(message)+1)<4096)
			{
				if(!strlen(message))
                {
					strncpy(message,argv[i],4096);
                    message[4095]=0;
				}
                else
				{
					strcat(message," ");
					strcat(message,argv[i]);
				}
			}
			else
			{
				// Build end/Partial
				int	tlen=(4096-1)-strlen(message);

				if(tlen)
				{
					if(!strlen(message))
					{
						strncpy(message,argv[i],tlen);
					}
					else
					{
						if(tlen>3)
						{
							strcat(message," ");
							strncat(message,argv[i],tlen-1);
						}
					}
				}
				break;
			}
		}
	}
    // Manditory for windows
    network_init();
    // lets bind a socket for receive
    sd=udp_listener(0, our_ip);
    if(sd<0)
    {
        printf("Failed to bind socket\n");
        exit(1);
    }

    // set nonblock
    set_sock_nonblock(sd);

	if(strlen(message))
	{
		strcat(message,"\n");
		sd = socket(AF_INET, SOCK_DGRAM, 0);
		if (sd == -1)
		{
			printf("Could not create socket.\n");
			exit(1);
		}

		// Broadcast?
		if(send_broadcast)
		{
             if(verbose)
				 printf("Setting socket to broadcast.\n");
			// this call is what allows broadcast packets to be sent:
			if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, (char *)&broadcast, sizeof broadcast) == -1)
			{
				perror("Could not set broadcast (Not Root?)\n");
				exit(1);
			} 
		}

		server.sin_family		= AF_INET;
		server.sin_addr.s_addr	= ip.ip32;
		server.sin_port			= htons((U16)(destport));

		ret=sendto(sd, (char *)message, strlen(message), 0, (struct sockaddr *)&server, sizeof(struct sockaddr));

		if(ret<0)
		{
			if(verbose)
				printf("failed to send message %s\n",message);
			exit(1);
		}
		else
		{
			if(verbose)
			{
				if(send_broadcast)
					printf("sent brodcast to %d.%d.%d.%d:%d the message : %s\n",ip.ipb1,ip.ipb2,ip.ipb3,ip.ipb4,destport,message);
				else
					printf("sent to %d.%d.%d.%d:%d the message : %s\n",ip.ipb1,ip.ipb2,ip.ipb3,ip.ipb4,destport,message);
			}
			exit(0);	
		}
	}
	else
	{
        // try interactive

        go=1;
        while(go)
        {
            // check for UDP socket read
            memset(&client,'\0',sizeof(struct sockaddr));
            slen=sizeof(struct sockaddr_in);
            ret=recvfrom(sd, (char *)message, 1024, 0, (struct sockaddr *)&client, (socklen_t *) &slen);
            if(ret>0)
            {
                message[ret]=0;
                //printf("from-%s >> %s\n",inet_ntoa(client.sin_addr),message);
                printf("%s",message);
                fflush(stdout);
            }

            // check for input event (one for windows, one for linux/osx/unix)
            if(_kbhit())
            { 
                // for now just readln
                ret=readln_from_a_file((FILE*)stdin, (char *)command_buffer, CMD_MAX_SIZE);
                
                if(ret)
                {
                    // check for exit command?

                    //
                    server.sin_family		= AF_INET;
		            server.sin_addr.s_addr		= ip.ip32;
		            server.sin_port			= htons((U16)(destport));
                    // exit command?

		            ret=sendto(sd, (char *)command_buffer, strlen(command_buffer), 0, (struct sockaddr *)&server, sizeof(struct sockaddr));
                }
            }
        }

    }


	exit(0);
}

