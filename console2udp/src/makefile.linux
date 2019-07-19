
TARGET	= udptelnet	

all: $(TARGET)

# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = 

#deines for build env  -DWEB_PROXY if you are building a web proxy for linux, reduces memory

# Use this for Linux client software
CDEFINE = -DLINUX 

# Options for development
CFLAGS		= $(INCLUDE) $(CDEFINE) -g -Wall -Os 
LDFLAGS		=
LINKFLAGS	= -o
#OSXLINKFLAGS	= -arch i386 -arch ppc

LIBS            = 
LIBS_INC        = 
# Options for release
# CFLAGS = -O -Wall -ansi

#Dependancies for all
DEPENDALL	= config.h mytypes.h debug.h

#Objects
OBJ	= arch.o webio.o net.o telnet_udp.o

$(TARGET): $(OBJ) 
	$(CC) $(LINKFLAGS) $(TARGET) $(OBJ) $(LIBS)

clean:
	-rm *.o $(MYLIB)
	 rm -f *.map
	 rm -f $(TARGET)


