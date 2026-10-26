
# SainSmartUsbRelay
*** UPDATE ***
Realtime UDP control now fully tested and latency between command and control has been completely eliminated.  Many UDP controllers can be connected at the same time to get instantaneous updates and control.   We are currently using 7 16 channel relay boards in our current setup driving the relays with a 88 key MIDI keyboard, a MIDI controller and a realtime web interface using a combination of udptelnet with websocketd.  The startup 50ms latency problem has been resolved, latency is virtually eliminated while using UDP sockets to drive this software.

-------

C program to enumerate and control SainSmart 16 channel relay boards.  I created this software for the Reared In Steel Flower Tower Fire Effects for Burning Man.   I needed to gang multiple boards and control them as one unit.   This software enumerates all SainSmart USB boards and lets you control up to 4 boards with one command.

This software is still in flux and the Daemonize functions are not yet fully tested.

## To Build
Clone into a directory, enter the source directory and type 'make'.  This will generate a binary called 'usb-relay'. You should not need any non standard or special libraries to build this software as it directly uses hidraw.  This has been built on raspberry pi's and Ubuntu desktops, but it should build and run on most linux systems.

## Using the Software
Currently this software can be driven from the command line in interactive mode or by a UDP socket.  I primarily use it via websockets with
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
         -a write all boards on every set (Pi 3: avoids idle-board USB stalls)
         -n no verify: skip read-back after each board write (HID boards)
         -b id,id,... relay16 board IDs for positions 1,2,... (default: board ID N is position N)
         -f ms relay16 board failsafe: relays off after ms without traffic (default 1000, 0 = off)
         -s port also probe this serial port for a relay16 board (repeatable)
```

### Interactive  Mode
When run without options the software will run in interactive mode.  It will enumerate all the relay boards and allow them to be set and read.   If one board is found the bitmask would be 4 hex digits long, two boards are found the bitmask would be 8 hex digits long and so on.

Interactive commands are in the following format:
```
[command] <bitmask> <hold time>
```
Commands that are supported are set, get, quit.

'get' by itself will return the current relay state.

'set' requires a bitmask, and an optional 'hold time' in 100's of MS increments.

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

When a hold time is specified, the state will be immediately returned, but the next command will not be processed until hold time has expired.  This allows control programs using websockets to be totally event driven without having to rely on timer callbacks.

### SainSmart Serial Boards (relay16 firmware)
SainSmart's HID boards are no longer made. The current SainSmart 16-channel
board (101-70-208) has a CH340 serial port and slow stock firmware; reflashed
with [relay16](https://github.com/lowerpower/sainsmart-relay16) it is on par
with the HID boards, and usb-relay drives it with the same `set`/`get`
commands. HID and relay16 boards can be mixed.

usb-relay finds relay16 boards on their own (any `/dev/ttyUSB*` or
`/dev/ttyACM*` that is a CH340) and places each one by the **ID stored on the
board**, never by USB port or plug-in order:

- by default, board ID N is position N: `set 00010000` turns on relay 1 of
  board ID 2
- `-b 11,12,13` puts board ID 11 at position 1, 12 at position 2, 13 at
  position 3
- a missing board leaves its position empty and the other boards stay where
  they are; two boards with the same ID stop usb-relay at startup
- a board that drops off USB is picked up again by its ID when it comes back

#### Board IDs and board order
Give each board its ID once, before it goes in a rig. Stop usb-relay (it
holds the ports open), plug in one board by itself, and use the relay16 host
tool:

```
relay16.py scan                     # "unassigned /dev/ttyUSB0"
relay16.py -p /dev/ttyUSB0 i 3      # set ID 3
relay16.py -p /dev/ttyUSB0 w        # save to EEPROM (all relays must be off)
```

Label the board with its ID. The set mask has 4 hex digits per position,
rightmost first, and each position is one board chosen by its ID:

| Position | Mask digits (from the right) | Relay 1 on | Default board | With `-b 11,12,13` |
|---|---|---|---|---|
| 1 | 1-4 | `set 0001` | ID 1 | ID 11 |
| 2 | 5-8 | `set 00010000` | ID 2 | ID 12 |
| 3 | 9-12 | `set 000100000000` | ID 3 | ID 13 |

Run with `-v` to see which board landed where, and `-t` to walk every relay
in position order. To replace a board, give the spare the old board's ID.
Choosing IDs across several rigs, what happens with missing, duplicate or
unlisted boards, and a pre-burn check are in the relay16
[board ID guide](https://github.com/lowerpower/sainsmart-relay16/blob/main/docs/BOARD-IDS.md).

Failsafes for relay16 boards, on top of the max on-time and hold logic:

- each board is set all-off when usb-relay attaches it, and every relay is
  cleared when usb-relay exits (quit, SIGINT or SIGTERM)
- `-f MS` (default 1000) sets each board's own failsafe: if usb-relay stops
  talking to a board for that long (crash, hang, unplugged cable) the board
  turns its relays off by itself. usb-relay sends idle boards a keepalive
  every MS/4. `-f 0` turns the board failsafe off. The setting is not saved
  on the board.

```
-b id,id,...   relay16 board IDs for positions 1,2,... (default: ID N = position N)
-f ms          relay16 board failsafe (default 1000, 0 = off)
-s port        also probe this serial port (repeatable), e.g. a non-CH340 adapter
```

The user running usb-relay needs access to the tty (the `dialout` group on
most distributions). A set across several boards takes about 3-4 ms in total:
commands go to every board first and the replies are checked after.

### Emulating Relay Boards
You can run the software without hardware when started in emulation mode.  This is enabled by specifying the -e option on the command line with the number of boards you wish to emulate (valid range 1-4).  The number of relay boards will be virtually enumerated and allowed to be virtually set and read.

### Using Websocketd
I discovered websocketd and used this instead of a more complex scheme I had initially contrived using UDP sockets and php.  Websocketd allows you to 'websocket' enable a stdio program.   Since I had built the interactive mode into this software, this mapped nicely to websockets.  You can enable this software with the following command:

```
/path/to/websocketd --port=8080 /path/to/usb-relay
```
or emulating 3 relay boards
```
/path/to/websocketd --port=8080 /path/to/usb-relay -e 3
```
or run in background with logging to /tmp
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
* https://github.com/lowerpower/sainsmart-relay16 - relay16 firmware for SainSmart's serial 16-channel boards.
* https://blog.mycal.net/finally-a-replacement-for-the-relay-boards-we-cant-buy-anymore/ - the story behind the relay16 firmware.
* https://github.com/lowerpower/fire-controller-web - Web UI that can drive this software.
* https://www.facebook.com/rearedinsteel/ - Reared In Steel facebook.
* https://www.facebook.com/rearedinsteel/videos/713271008877496/ - project in action on flower tower
* [Video Demo](https://www.youtube.com/watch?v=d_1EEWdWekI) of this project integrated into a solution.
* [websocketd](https://github.com/joewalnes/websocketd) - allows websocket control of a stdio program.

## Acknowledgements
* https://github.com/mvines/relay :
I've borrowed the [required hardware](https://github.com/lowerpower/SainSmartUsbRelay/new/master?readme=1#required-hardware) section of this document from this site.  I also used this software here to help me figure out
the USB protocol for these relay boards, this is great software but I needed something a bit different and was more comfortable
doing this in C.  I highly recommend you look at this software to see if it fits your purpose better than the software here, and for general knowledge.
