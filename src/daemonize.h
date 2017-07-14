#ifndef __DAEMONIZE_H__
#define __DAEMONIZE_H__
/*! \file daemonize.h
    \brief Daemonize code for going into background
*/

#if defined(WIN32)

//assert("No support for daemonize on WIN32\n");

#else


//int daemonize(char* path, char* outfile, char* errfile, char* infile );
int
daemonize(char *pidfile, char *user, char *dir, char* path, char* outfile, char* errfile, char* infile );

#endif

#endif
