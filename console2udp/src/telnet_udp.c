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
#include "yselect.h"

#define CMD_MAX_SIZE 4096
#define MAX_MESSAGE 65535


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


// do not fragment = fragment =1
int
set_sock_doNotFragment(SOCKET lsock, int fragment)
{
	int ret = 0, i = 0;

	if (fragment)
		i = 1;
#if defined(WIN32)
	{
		DWORD frag = i;
		
		ret = setsockopt(lsock, IPPROTO_IP, IP_DONTFRAGMENT, (const char*)&frag, sizeof(frag));		
	}
#elif defined(LINUX)
	if (fragment)
		i = IP_PMTUDISC_DO;
	else
		i = IP_PMTUDISC_DONT;
	ret = setsockopt(lsock, IPPROTO_IP, IP_MTU_DISCOVER, (const void*)&i, sizeof(int));

#elif defined(MACOSX) || defined(IOS)
	// do nothings mac does not support this	
	ret = -1;
#pragma message("OSX does not support IP_DONTFRAGMENT")
#elif	defined(BSD_TYPE)
	ret = setsockopt(lsock, IPPROTO_IP, IP_DONTFRAGMENT, &i, sizeof(int));
#endif
	printf("do not fragment returened %d - error %d\n", ret, get_last_error());
	return(ret);
}




void usage(int argc, char **argv)
{
  printf("usage: %s [-p port] [-h host] [-v(erbose)] [-?(this message)]\n",argv[0]);
  printf("	Defaults port=1024, host=127.0.0.1, verbose off\n");
  exit(0);
}


int main(int argc, char* argv[])
{
	int					ci, verbose = 0, send_broadcast = 0, go, slen;
	int					ret, sd;
	int					mtu_test = 0;
	U16					destport;
	struct sockaddr_in	server;
	struct sockaddr_in	client;
	struct hostent* host_info;
	IPADDR				ip, our_ip;
	char				message[MAX_MESSAGE+1];
	char                command_buffer[CMD_MAX_SIZE];
	char* strt_p;
	int                 broadcast = 1;
	int					current_mtu=0, last_good_mtu = 0;
	int					current_mru = 0, last_mru =0;

	// Set defaults
	our_ip.ip32 = 0;
	ip.ipb1 = 127;
	ip.ipb2 = 0;
	ip.ipb3 = 0;
	ip.ipb4 = 1;
	destport = 1024;
	// Standard Config
	memset(message, 0, 4096);
	//
	Y_Init_Select();

	//------------------------------------------------------------------
	// Argument Scan command line args, first for -h -b
	//------------------------------------------------------------------

	for (ci = 1; ci < argc; ci++)
	{
		if (0 == strncmp(argv[ci], "-p", 2))
		{
			// Get port
			ci++;
			if (argc == ci)
				usage(argc, argv);

			destport = atoi(argv[ci]);
		}
		else if (0 == strncmp(argv[ci], "-h", 2))
		{
			U8* subst;

			// Get host, check if IP or Name
			ci++;
			if (argc == ci)
				usage(argc, argv);

			if (isalpha(argv[ci][0]))
			{
				// convert name to IP
				host_info = gethostbyname(argv[ci]);
				if (host_info != 0)
				{
					ip.ip32 = *((unsigned long*)host_info->h_addr);
				}
				else
				{
					printf("Failed to resolve %s\n", argv[ci]);
					exit(0);
				}
				//if(verbose)
								//        printf("Sending to %s or %d.%d.%d.%d\n",argv[ci],ip.ipb1,ip.ipb2,ip.ipb3,ip.ipb4);
			}
			else
			{
				// get proxy target IP		
				subst = (U8*)strtok_r(argv[ci], ".\n", &strt_p);
				if (strlen((char*)subst))
					ip.ipb1 = atoi((char*)subst);
				subst = (U8*)strtok_r(NULL, ".\n", &strt_p);
				if (strlen((char*)subst))
					ip.ipb2 = atoi((char*)subst);
				subst = (U8*)strtok_r(NULL, ".\n", &strt_p);
				if (subst)
					ip.ipb3 = atoi((char*)subst);
				subst = (U8*)strtok_r(NULL, ".\n", &strt_p);
				if (subst)
					ip.ipb4 = atoi((char*)subst);
				//if(verbose)
				//	printf("Sending to %d.%d.%d.%d\n",ip.ipb1,ip.ipb2,ip.ipb3,ip.ipb4);
			}
		}
		else if (0 == strncmp(argv[ci], "-b", 2))
		{
			send_broadcast = 1;
			ip.ipb1 = 255;
			ip.ipb2 = 255;
			ip.ipb3 = 255;
			ip.ipb4 = 255;
		}
		else if (0 == strncmp(argv[ci], "-m", 2))
		{
			// udp mtu test
			mtu_test = 1;
		}
		else if (0 == strncmp(argv[ci], "-v", 2))
		{
			// Verbose
			verbose = 1;
			printf("verbose on\n");
		}
		else if (0 == strncmp(argv[ci], "-?", 2))
		{
			// print help
			usage(argc, argv);
		}
		else
		{
			// No more args break
			break;
		}
	}

	// Manditory for windows, initialize network
	network_init();
	// lets bind a socket for receive
	sd = udp_listener(0, our_ip);
	if (sd < 0)
	{
		printf("Failed to bind socket\n");
		exit(1);
	}

	// set nonblock
	set_sock_nonblock(sd);
	Y_Set_Select_rx(sd);


	// we should verify we have a minimum set.
	if (mtu_test)
	{
		int step = 500;
		// do an MTU test to Bouncer peer first.
		// start at 512 and start sending incrementing starts at 500 bytes
		current_mtu = 512;
		// no fragments
		ret=set_sock_doNotFragment(sd, 1);

		if (-1 == ret)
		{
			printf("Cannot set DF flag (do not fragment), cannot accuetly calculate MTU.\n");
		}

		while (1)
		{
			int rdy;

			// build message.
			memset(message, '.', sizeof(CMD_MAX_SIZE));
			// add the 't'
			message[0] = 't';
			//
			memset(&server, '\0', sizeof(struct sockaddr_in));
			slen = sizeof(struct sockaddr_in);
			server.sin_family = AF_INET;
			server.sin_addr.s_addr = ip.ip32;
			server.sin_port = htons((U16)(destport));

			ret = sendto(sd, (char*)message, current_mtu, 0, (struct sockaddr*)&server, sizeof(struct sockaddr));
			if (ret != current_mtu)
			{
				if (EMSGSIZE == get_last_error())
				{
					// Adust, we are to big
					step = (current_mtu - last_good_mtu) / 2;

					printf("Cannot send, New Step calculated to %d, current MTU %d, last good MTU %d\n", step, current_mtu, last_good_mtu);

					// binary search down.
					if (step)
						current_mtu -= step;
					else
					{
						printf("Done MTU is %d (calculated + IP +UDP)\n", last_good_mtu+28);
						break;
					}
					continue;
				}
				printf("error on sendto: current_mtu %d  error %d\n",current_mtu, get_last_error());
				break;
			}
			// wait for response up to 1 second
			rdy = Y_Select(1000);
			if (0 == rdy)
			{
				// we have timed out, update and continue
				if((current_mtu-1)==last_good_mtu)
				{
					printf("MTU calculated to %d\n", last_good_mtu);
					break;
				}
				else if (current_mtu>last_good_mtu)
				{
					step = (current_mtu-last_good_mtu) / 2;

					printf("Step calculated to %d, current MTU %d, last good MTU %d\n", step, current_mtu, last_good_mtu);

					// binary search down.
					if (step)
						current_mtu -= step;
					else
					{
						// done calculating
						printf("Done MTU is %d\n", last_good_mtu);
						break;
					}
					
					continue;
				}
				else
				{
					printf("woops\n");
				}
			}
			// check for UDP socket read
			memset(&client, '\0', sizeof(struct sockaddr));
			slen = sizeof(struct sockaddr_in);
			ret = recvfrom(sd, (char*)message, CMD_MAX_SIZE, 0, (struct sockaddr*)&client, (socklen_t*)&slen);
			if (ret > 0)
			{
				if ('t' == message[0])
				{
					int size;
					// we have a good length response, lets parse the incoming size
					size = atoi(&message[1]);
					if (size == current_mtu)
					{
						// got a good reply back, should always increase
						if (last_good_mtu < current_mtu)
						{
							last_good_mtu = current_mtu;
							current_mtu += step;						// next try
							printf("MTU tested good at %d bytes, trying %d bytes next.\n", last_good_mtu, current_mtu);
						}
						else 
						{
							printf("current mtu %s  is less then last good mtu %d should not happen\n", current_mtu, last_good_mtu);			
						}
					}
					else
					{
						printf("bad size returned = %d, expecting %d\n", size, current_mtu);
						break;
					}
				}
			}
			else
			{
					printf("error on recvfrom: current_mtu %d, error %d\n", current_mtu, get_last_error());
			}
		}
		//
		// Next check if fragment over calculated MTU works
		//


	


		// do an MRU test from Bouncer peer next.
	}
	else
	{
		if (argc > ci)
		{
			int i;

			//
			// Build Message
			//
			for (i = ci; i < argc; i++)
			{

				// Check if we fit in tweet
				// stlen argv+1 for space
				if ((strlen(argv[i]) + strlen(message) + 1) < 4096)
				{
					if (!strlen(message))
					{
						strncpy(message, argv[i], 4096);
						message[4095] = 0;
					}
					else
					{
						strcat(message, " ");
						strcat(message, argv[i]);
					}
				}
				else
				{
					// Build end/Partial
					int	tlen = (4096 - 1) - strlen(message);

					if (tlen)
					{
						if (!strlen(message))
						{
							strncpy(message, argv[i], tlen);
						}
						else
						{
							if (tlen > 3)
							{
								strcat(message, " ");
								strncat(message, argv[i], tlen - 1);
							}
						}
					}
					break;
				}
			}
		}


		// Add Stdin also
		Y_Set_Select_rx(0);

		if (strlen(message))
		{
			strcat(message, "\n");
			sd = socket(AF_INET, SOCK_DGRAM, 0);
			if (sd == -1)
			{
				printf("Could not create socket.\n");
				exit(1);
			}

			// Broadcast?
			if (send_broadcast)
			{
				if (verbose)
					printf("Setting socket to broadcast.\n");
				// this call is what allows broadcast packets to be sent:
				if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof broadcast) == -1)
				{
					perror("Could not set broadcast (Not Root?)\n");
					exit(1);
				}
			}

			server.sin_family = AF_INET;
			server.sin_addr.s_addr = ip.ip32;
			server.sin_port = htons((U16)(destport));

			ret = sendto(sd, (char*)message, strlen(message), 0, (struct sockaddr*)&server, sizeof(struct sockaddr));

			if (ret < 0)
			{
				if (verbose)
					printf("failed to send message %s\n", message);
				exit(1);
			}
			else
			{
				if (verbose)
				{
					if (send_broadcast)
						printf("sent brodcast to %d.%d.%d.%d:%d the message : %s\n", ip.ipb1, ip.ipb2, ip.ipb3, ip.ipb4, destport, message);
					else
						printf("sent to %d.%d.%d.%d:%d the message : %s\n", ip.ipb1, ip.ipb2, ip.ipb3, ip.ipb4, destport, message);
				}
				exit(0);
			}
		}
		else
		{
			// try interactive
			if (verbose) printf("Enter Interactive Mode\n");

			go = 1;
			while (go)
			{
				int rdy;

				rdy = Y_Select(1000);
				if (rdy)
				{

					// check for UDP socket read
					memset(&client, '\0', sizeof(struct sockaddr));
					slen = sizeof(struct sockaddr_in);
					ret = recvfrom(sd, (char*)message, CMD_MAX_SIZE, 0, (struct sockaddr*)&client, (socklen_t*)&slen);
					if (ret > 0)
					{
						message[ret] = 0;
						if (verbose)
							printf("from-%s:%d len %d >> %s\n", inet_ntoa(client.sin_addr), htons(client.sin_port), ret, message);
						else
							printf("%s", message);
					}

					// check for input event (one for windows, one for linux/osx/unix)
					if (_kbhit())
					{
						// for now just readln
						ret = readln_from_a_file((FILE*)stdin, (char*)command_buffer, CMD_MAX_SIZE - 1);

						if (ret)
						{
							// check for exit command?

							//
							server.sin_family = AF_INET;
							server.sin_addr.s_addr = ip.ip32;
							server.sin_port = htons((U16)(destport));
							// exit command?
							if (verbose)
								printf("Sending Length %ld >>%s\n", strlen(command_buffer), command_buffer);

							ret = sendto(sd, (char*)command_buffer, strlen(command_buffer), 0, (struct sockaddr*)&server, sizeof(struct sockaddr));
						}
					}
				}
			}

#if 0
			while (go)
			{
				// check for UDP socket read
				memset(&client, '\0', sizeof(struct sockaddr));
				slen = sizeof(struct sockaddr_in);
				ret = recvfrom(sd, (char*)message, 1024, 0, (struct sockaddr*)&client, (socklen_t*)&slen);
				if (ret > 0)
				{
					message[ret] = 0;
					//printf("from-%s >> %s\n",inet_ntoa(client.sin_addr),message);
					printf("%s", message);
					fflush(stdout);
				}

				// check for input event (one for windows, one for linux/osx/unix)
				if (_kbhit())
				{
					// for now just readln
					ret = readln_from_a_file((FILE*)stdin, (char*)command_buffer, CMD_MAX_SIZE);

					if (ret)
					{
						// check for exit command?

						//
						server.sin_family = AF_INET;
						server.sin_addr.s_addr = ip.ip32;
						server.sin_port = htons((U16)(destport));
						// exit command?

						ret = sendto(sd, (char*)command_buffer, strlen(command_buffer), 0, (struct sockaddr*)&server, sizeof(struct sockaddr));
					}
				}
			}
#endif

		}
	}

	exit(0);
}

