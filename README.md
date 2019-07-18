# I broke this, will fix soon

# SainSmartUsbRelay
C program to enumerate and control SainSmart 16 channel relay boards.  I created this software for the Reared In Steel Flower Tower Fire Effects for Burning Man.   I needed to gang multiple boards and control them as one unit.   This software enumerates all SainSmart USB boards and lets you controll up to 4 boards with one command.

This software is still in flux and the UDP command control and Daemonize functions are completly untested.

## To Build
Clone into a directory, enter the source directory and type 'make'.  This will generate a binary called 'usb-relay'. You should not need any non standard or special librarys to build this software as it directly uses hidraw.  This has been build on raspberry pi's and Ubuntu desktops, but it should build and run on most linux systems.

## Using the Software
Currently this software is can be drivin the command line in interactive mode or by a UDP socket.  I primarly use it via websockets with 
[websocketd](https://github.com/joewalnes/websocketd) controlling it with a [web interface](https://github.com/lowerpower/fire-controller-web)

```
SainSmartUsbRelay Aug 15 2017 at 04:44:18
   Version 0.1 - https://github.com/lowerpower/SainSmartUsbRelay
usage: ./usb-relay [-h] [-v(erbose)] [-c udp_command_port] bitmask 
         -h this output.
         -v verbosity.
         -t run test.
         -c command port (defaults 1026)
         -e emulate number of boards (no hardware needed)
```

### Interactive  Mode
When run without options the software will run in interactive mode.  It will enumerate all the relay boards and allow them to be set and read.   If one board is found the bit mask the bitmask would be 4 hex digits long, two boards are found the bitmask would be 8 hex digits long and so on.

Interactive commands are in the following format:
```
[command] <bitmask> <hold time>
```
Command that are supported are set, get, quit.

'get' by itself will return the current relay state.

'set' requires a bitmask, and an optional 'hold time' in 100's of MS incerments.

Example set relays 16-13 and 5-8
```
set F0F0[enter]
-returns
F0F0
```

Example set relay 1 on second board
```
set 10000[enter]
-returns
10000
```

Example set relay 1 on first board with 500ms hold time
```
set 1 5[enter]
-returns
1
```

When a hold time is specified, the state will be immediatly returned, but the next command will not be processed until hold time has expired.  This allows control programes using websockets to be totally event drivin without having to rely on timer call backs.

### Emulating Relay Boards
You can run the software without hardware when started in emulation mode.  This is enabled by specifing the -3 option on the command line with the number of boards you wish to emulate (valid range 1-4).  The number of relay boards will be virtually enumerated and allowed to be virtually set and read.

### Using Websocketd
I discovered websocketd and used this instead of a more complex scheam I had initially contrived using UDP sockets and php.  Websocketd allows you to 'websocket' enable a stdio program.   Since I had built the interactive mode into this software, this mapped nicely to websockets.  You can enable this software with the following command:

```
/path/to/websocketd --port=8080 /path/to/usb-relay
```
or emulating 3 relay boards
```
/path/to/websocketd --port=8080 /path/to/usb-relay -e 3
```
or run in background with loging to /tmp
```
nohup /path/to/websocketd --port=8080 /path/to/usb-relay > /tmp/websocketd.log 2>&1  &
```

## Required Hardware
Search Amazon for "SainSmart 16-CH USB HID Programmable Control Relay Module +
Relay".  The unit looks like this:

<p align="center">
<img src="https://github.com/mvines/relay/raw/master/relay.jpg"/>
</p>

You will also need a 12V power supply (~1A) connected to the **blue**
connector on the main relay board.  Do not connect anything to the **green** connector
on the USB HID board.

## Links
* https://github.com/lowerpower/fire-controller-web - Web UI that can drive this software.
* https://www.facebook.com/rearedinsteel/ - Reared In Steel facebook.  
* https://www.facebook.com/rearedinsteel/videos/713271008877496/ - project in action on flower tower
* [Video Demo](https://www.youtube.com/watch?v=d_1EEWdWekI) of this project integrated into a solution.
* [websocketd](https://github.com/joewalnes/websocketd) - allows websocket control of a stdio program.

## Acknolegements
* https://github.com/mvines/relay :
I've borrowed the [required hardware](https://github.com/lowerpower/SainSmartUsbRelay/new/master?readme=1#required-hardware) section of this document from this site.  I also used this software here to help me figure out 
the USB protocol for these reley boards, this is great software but I needed somehting a bit different and was more cofortable 
doing this in C.  I highly recommend you look at this software to see if it fits your purpose betther than the software here, and for general knowlege.
* 
