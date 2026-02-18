/*!																www.mycal.net			
 *---------------------------------------------------------------------------
 *! \file daemonize.c
 *  \brief Function to daemonize a process
 *																			
 *---------------------------------------------------------------------------
 * Version                                                                  -
 *		0.1 Original Version June 3, 2006									-        
 *
 *---------------------------------------------------------------------------    
 * Version                                                                  -
 * 0.1 Original Version August 31, 2006     							    -
 *																			-
 * (c)2006 mycal.net								-
 *---------------------------------------------------------------------------
 *
 */
#if !defined(WIN32)             // For unix only

#include <stdio.h>    //printf(3)
#include <stdlib.h>   //exit(3)
#include <unistd.h>   //fork(3), chdir(3), sysconf(3)
#include <signal.h>   //signal(3)
#include <sys/stat.h> //umask(3)
#include <sys/types.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>        // getepwname(3)
#include <syslog.h>   //syslog(3), openlog(3), closelog(3)
#include <string.h>
#include <errno.h>
#include "daemonize.h"



/*! \fn int daemonize(user,path,outfile,errorfile,infile)
    \brief Daemonize a process

    \param 

	\return 
*/

int
daemonize(char *pidfile, char *user, char *dir, char* path, char* outfile, char* errfile, char* infile )
{
    int ret;
    pid_t child;
    struct passwd *pw;
	FILE *pidfd=NULL;



    // Fill in defaults if not specified
    if(!path) { path="/"; }
    if(!dir) { dir="/"; }
    if(!infile) { infile="/dev/null"; }
    if(!outfile) { outfile="/dev/null"; }
    if(!errfile) { errfile="/dev/null"; }
    if(!user) { user="nobody"; }

    // pidfile is optional
    if( (0!=pidfile) && (0!=strlen(pidfile)) )
    {
        if ((pidfd = fopen(pidfile, "w")) == NULL) 
        {
            fprintf(stderr, "Failed to open pid file %s: %s\n",pidfile, strerror(errno));
        }
    }

    if ((pw = getpwnam(user)) == NULL) {
        fprintf(stderr, "getpwnam(%s) failed: %s\n",
                user, strerror(errno));
                exit(EXIT_FAILURE);
    }

    //fork, detach from process group leader
    if( (child=fork())<0 ) { //failed fork
        fprintf(stderr,"error: failed fork\n");
        exit(EXIT_FAILURE);
    }
    if (child>0) { //parent
        exit(EXIT_SUCCESS);
    }
    if( setsid()<0 ) { //failed to become session leader
        fprintf(stderr,"error: failed setsid\n");
        exit(EXIT_FAILURE);
    }

    //catch/ignore signals
    signal(SIGCHLD,SIG_IGN);
    signal(SIGHUP,SIG_IGN);

    //fork second time
    if ( (child=fork())<0) { //failed fork
        fprintf(stderr,"error: failed fork\n");
        exit(EXIT_FAILURE);
    }
    if( child>0 ) { //parent
         if (pidfd != NULL) {
            fprintf(pidfd, "%d\n", child);
            fclose(pidfd);
		}
        exit(EXIT_SUCCESS);
    }

    //new file permissions
    umask(0);


    //reopen stdin, stdout, stderr
    if(!freopen(infile,"r",stdin)   ||   //fd=0
       !freopen(outfile,"w+",stdout) ||  //fd=1
       !freopen(errfile,"w+",stderr)) {  //fd=2
        
        syslog(LOG_WARNING, "close std fds %s failed: %s\n",
            dir, strerror(errno));    
    }


    if (chroot(dir) < 0) {
        syslog(LOG_ERR, "chroot(%s) failed: %s\n",
                            dir, strerror(errno));
        fprintf( stderr, "\nchroot(%s) failed: %s\n",dir, strerror(errno));
        exit(EXIT_FAILURE);
    }

    //change to path directory
    if (chdir(path) < 0) {
        syslog(LOG_ERR, "chdir(\"/\") failed: %s\n",
                            strerror(errno));
        fprintf( stderr, "\nchdir(\"/\") failed: %s\n",strerror(errno)); 
        exit(EXIT_FAILURE);
    }

    if (setgroups(1, &pw->pw_gid) < 0) {
        syslog(LOG_ERR, "setgroups() failed: %s\n",
                            strerror(errno));
        fprintf( stderr,  "\nsetgroups() failed: %s\n",strerror(errno)); 
        exit(EXIT_FAILURE);
    }

    if (setgid(pw->pw_gid)) {
            syslog(LOG_ERR, "setgid %i (user=%s) failed: %s\n",
                            pw->pw_gid, user, strerror(errno));
            exit(EXIT_FAILURE);
    }

    if (setuid(pw->pw_uid)) {
            syslog(LOG_ERR, "setuid %i (user=%s) failed: %s\n",
                            pw->pw_uid, user, strerror(errno));
            exit(EXIT_FAILURE);
    }

    ret=1;
    return(ret);
}

#endif
