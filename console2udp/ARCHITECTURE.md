# console2udp - Architecture

## Overview

**console2udp** (`udptelnet`) is a cross-platform UDP client for communicating with the usb-relay daemon. It provides interactive and command-line modes for sending relay control commands over UDP, plus an MTU discovery tool.

## Component Diagram

```
┌─────────────────────────────────────────────────────┐
│                   console2udp                       │
│                                                     │
│  ┌──────────────┐    ┌───────────┐    ┌──────────┐  │
│  │ telnet_udp.c │───▶│   net.c   │───▶│ webio.c  │  │
│  │  (main app)  │    │ (sockets, │    │(send/recv│  │
│  │              │    │  DNS,     │    │ +optional │  │
│  │ - arg parse  │    │  HTTP)    │    │  SSL)     │  │
│  │ - event loop │    └───────────┘    └──────────┘  │
│  │ - MTU test   │                                   │
│  └──────┬───────┘                                   │
│         │                                           │
│  ┌──────┴───────┐    ┌───────────┐                  │
│  │  yselect.c   │    │  arch.c   │                  │
│  │  (I/O mux)   │    │ (platform │                  │
│  │              │    │  abstraction)                 │
│  └──────────────┘    └───────────┘                  │
│                                                     │
│  ┌──────────────────────────────────────────────┐   │
│  │  config.h / mytypes.h / debug.h              │   │
│  │  (platform detection, types, debug macros)   │   │
│  └──────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────┘
         │
         │ UDP (port 1024 default)
         ▼
┌─────────────────┐
│  usb-relay      │
│  daemon :1026   │
│  (../src/)      │
└─────────────────┘
```

## Operating Modes

### 1. Command-Line Mode (single shot)
Send a message from argv and exit immediately.
```
$ udptelnet -h 192.168.1.10 -p 1026 set FFFF
```

### 2. Broadcast Mode
Send to 255.255.255.255 with `SO_BROADCAST`.
```
$ udptelnet -b -p 1026 ping
```

### 3. Interactive Mode
Bidirectional telnet-like session. Multiplexes stdin and UDP socket via `Y_Select()`.
```
$ udptelnet -h 192.168.1.10 -p 1026
get
FFFF
set 0000
0000
```

### 4. MTU Discovery Mode
Binary search for maximum UDP payload without fragmentation.
```
$ udptelnet -m -h 192.168.1.10 -p 1026
```

## Data Flow

```
User Input (stdin)
    │
    ▼
_kbhit() ──▶ readln_from_a_file()
                    │
                    ▼
              sendto(sd, cmd, ...)  ──▶  UDP packet  ──▶  usb-relay daemon
                                                              │
                                                         process_command()
                                                              │
              recvfrom(sd, msg, ...) ◀──  UDP response  ◀────┘
                    │
                    ▼
              printf("%s", msg)
                    │
                    ▼
              User Output (stdout)
```

## File Reference

| File | Lines | Purpose |
|------|-------|---------|
| `telnet_udp.c` | ~550 | Main application: arg parsing, event loop, MTU test |
| `net.c` | ~1650 | Socket utilities, DNS, HTTP client, URL encoding |
| `net.h` | ~135 | Network types (`HTTP_RESP`, `HTTP_HEADER`) and prototypes |
| `webio.c` | ~565 | Send/recv abstraction with optional SSL/TLS layer |
| `webio.h` | ~90 | WebIO prototypes and error codes (`YOICS_SO_*`) |
| `arch.c` | ~655 | Platform abstraction: time, sleep, strings, file ops |
| `arch.h` | ~65 | Arch prototypes |
| `yselect.c` | ~173 | `select()` wrapper for I/O multiplexing |
| `yselect.h` | ~25 | Select prototypes |
| `config.h` | ~150 | Platform detection, conditional includes, compat macros |
| `mytypes.h` | ~120 | Cross-platform types: `U8`, `U16`, `U32`, `IPADDR`, `SOCKET` |
| `debug.h` | ~50 | Debug macros (levels 0-9), compile-time controlled |

## Key Data Structures

### IPADDR (mytypes.h)
```c
typedef struct _IPADDR {
    union {
        U32  ip32;                       // Full 32-bit address
        struct { U8 ipb1,ipb2,ipb3,ipb4; };  // Individual octets
        struct { U16 ipw1, ipw2; };      // 16-bit words
    };
} IPADDR;
```

### HTTP_RESP (net.h)
```c
typedef struct http_resp {
    int   resp_code;      // HTTP status (200, 404, etc.)
    int   http_version;   // 0=HTTP/1.0, 1=HTTP/1.1
    int   data_length;    // Content-Length value
    char *response;       // Status text ("OK") — malloc'd
    char *header;         // Raw header block — malloc'd, 2KB max
    char *data;           // Response body — malloc'd
} HTTP_RESP;
```

### Select State (yselect.c)
```c
static fd_set fd_rx_master;   // Persistent read set
static fd_set fd_rx_list;     // Working copy for select()
static fd_set fd_tx_master;   // Persistent write set
static fd_set fd_tx_list;     // Working copy for select()
static unsigned int fd_max;   // Highest FD tracked
```

## Interactive Event Loop

```c
while (go) {
    rdy = Y_Select(1000);          // 1 second timeout
    if (rdy) {
        // Check for incoming UDP data
        ret = recvfrom(sd, message, CMD_MAX_SIZE, 0, ...);
        if (ret > 0) {
            message[ret] = 0;
            printf("%s", message);
        }
        // Check for keyboard input
        if (_kbhit()) {
            readln_from_a_file(stdin, command_buffer, CMD_MAX_SIZE - 1);
            sendto(sd, command_buffer, strlen(command_buffer), 0, ...);
        }
    }
}
```

## MTU Discovery Algorithm

1. Set `IP_DONTFRAGMENT` socket option
2. Start at 512 bytes, step size 500
3. Send test packet (marker `'t'` + padding)
4. Wait up to 1 second for echo response
5. On success: record `last_good_mtu`, increase by step
6. On `EMSGSIZE` or timeout: binary search downward
7. Converge when `last_good_mtu == current_mtu - 1` or step reaches 0
8. Report final MTU (add 28 for IP+UDP headers)

## Platform Support

| Feature | Linux | macOS | Windows |
|---------|-------|-------|---------|
| Non-blocking I/O | `fcntl()` | `fcntl()` | `ioctlsocket()` |
| Sleep (usec) | `usleep()` | `usleep()` | `Sleep(ms)` |
| Time | `ftime()` | `gettimeofday()` | `ftime()` |
| DF flag (MTU test) | `IP_MTU_DISCOVER` | unsupported | `IP_DONTFRAGMENT` |
| Error codes | `errno` | `errno` | `WSAGetLastError()` |
| Net init | none | none | `WSAStartup()` |
| Keyboard detect | `select()` on fd 0 | `select()` on fd 0 | `_kbhit()` (conio) |

## Build System

```bash
# Linux (dynamic)
make

# Linux (static, for deployment)
make -f makefile.linux      # produces udptelnet.s

# macOS (cross-compile from Linux)
make -f makefile.osx        # produces udptelnet.osx
```

All builds produce a single binary with zero external runtime dependencies.

## Integration with usb-relay

The daemon (`../src/usb-relay`) listens on UDP port 1026 (configurable with `-c`). console2udp defaults to port 1024 but is typically pointed at 1026:

```bash
# Start daemon
./usb-relay -c 1026

# Connect client
./udptelnet -h 127.0.0.1 -p 1026
```

Commands understood by the daemon: `get`, `set <mask> [hold]`, `ping`, `reset`, `quit`, `disconnect`. The daemon broadcasts responses to all tracked UDP clients.

## Error Codes (webio.h)

| Code | Constant | Meaning |
|------|----------|---------|
| 0 | `YOICS_SO_SUCCESS` | OK |
| -1 | `YOICS_SO_UNKNOWN_ERROR` | Unknown failure |
| -2 | `YOICS_SO_FAILED_TO_RESOLVE_HOST` | DNS failure |
| -3 | `YOICS_SO_CANNOT_CREATE_SOCKET` | Socket creation failed |
| -6 | `YOICS_SO_TIMEOUT` | Operation timed out |
| -21 | `YOICS_SO_SSL_TIMEOUT` | SSL handshake timeout |

## Design Notes

- **No authentication** — commands accepted from any UDP source. Use on trusted networks only.
- **No encryption** — plaintext UDP unless compiled with `-DWEB_SSL` (xyssl library).
- **Single-threaded** — all I/O multiplexed through `select()` with 1-second timeout.
- **Zero dependencies** — only libc and kernel socket APIs required.
- **Graceful degradation** — MTU test reports failure on unsupported platforms rather than crashing.
