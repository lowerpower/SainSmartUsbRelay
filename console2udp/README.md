# udpsend
udp packet sender command line client

This is a simple command line udp packet sender.   It allows the sending of UDP unicast and broadcast packets.

# Usage
```
usage: ./udpsend [-p port] [-h host] [-v(erbose)] [-b(roadcast)] [-?(this message)] message to send
        Defaults port=1024, host=127.0.0.1, verbose off, broadcast off
```

# Examples

send a unicast packet to port 1024 on localhost containing "hello local host"
```
./udpsend hello local host
```

send a unicast packet to host 192.168.2.10 port 1234  containing "this is a test"
```
./udpsend -h 192.168.2.10 -p 1234  this is a test
```

send a unicast packet to host 192.168.2.10 port 1234  containing "this is a test" with verbose on
```
./udpsend -v -h 192.168.2.10 -p 1234  this is a test
verbose on
sent to 192.168.2.10:1234 the message : this is a test
```

send a broadcast packet to host 192.168.2.10 port 1234 containing "this is a test" with verbose on
```
./udpsend -v -b -p 1234  this is a test
verbose on
Setting socket to broadcast.
sent brodcast to 255.255.255.255:1234 the message : this is a test
```

# Building
Using the makefile in the src directory should built on most linux/unix platforms.   There is a win32 project for building on windows consol app also.


#License
see license file at github
https://github.com/lowerpower/udpsend




