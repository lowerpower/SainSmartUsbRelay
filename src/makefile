
TARGET	= usb-relay

all: $(TARGET)

# Which compiler
CC = gcc

# Where are include files kept
INCLUDE =

# Use this for Linux client software 
CDEFINE = -DLINUX -DI386 -D_GNU_SOURCE -DBACKTRACE_SYMBOLS 


# Options for development
CFLAGS		= $(INCLUDE) $(CDEFINE) -g -Wall -Os $(INCLUDE)
LDFLAGS		= 
LINKFLAGS	= -o 

LIBS            = 
LIBS_INC        = 

# Options for release
# CFLAGS = -O -Wall -ansi

#Dependancies for all
DEPENDALL	= mytypes.h yselect.h net.h webio.h yselect.h usb-relay.h daemonize.h

#Objects
OBJ	= usb-relay.o net.o webio.o arch.o log.o daemonize.o
OBJ    += yselect.o

$(TARGET): $(OBJ) 
	$(CC) $(CFLAGS) $(LINKFLAGS) $(TARGET) $(OBJ) $(LIBS)
	cp $(TARGET) .. 

clean:
	-rm *.o $(MYLIB)
	 rm -f *.map
	 rm -f $(TARGET)


