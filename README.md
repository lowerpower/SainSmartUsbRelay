# SainSmartUsbRelay
C program to enumerate and control SainSmart 16 channel relay boards.  I created this software for the Reared In Steel Flower Tower 
Fire Effects for Burning Man.   I needed to gang multiple boards and control them as one unit.   This software enumerates all 
SainSmart USB boards and lets you controll up to 4 boards with one command.

## To Build

## Using the Software

### Emulating Relay Boards


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
* hello

## Acknolegements
https://github.com/mvines/relay :
I've borrowed the required hardware (https://github.com/lowerpower/SainSmartUsbRelay/new/master?readme=1#required-hardware) section of this document from this site.  I also used this software here to help me figure out 
the USB protocol for these reley boards, this is great software but I needed somehting a bit different and was more cofortable 
doing this in C.  I highly recommend you look at this software to see if it fits your purpose betther than the software here, and 
for general knowlege
