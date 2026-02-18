# SainSmartUsbRelay Architecture

## Overview

A C-based control system for SainSmart 16-channel USB relay boards using the Linux HID raw interface. Designed for real-time relay automation with zero external dependencies. Originally built for fire effects control at Burning Man.

**Author:** Mike Johnson (lowerpower) | **License:** MIT (2017) | **Version:** 0.1

---

## Project Structure

```
SainSmartUsbRelay/
├── src/                        # Main relay control application
│   ├── usb-relay.c/h          # Core application & HID protocol
│   ├── arch.c/h               # Platform abstraction (time, string, I/O)
│   ├── net.c/h                # Cross-platform socket utilities
│   ├── webio.c/h              # Socket I/O with optional SSL
│   ├── yselect.c/h            # Select-based I/O multiplexing
│   ├── daemonize.c/h          # Background process support
│   ├── log.c/h                # Structured logging
│   ├── config.h               # Platform detection & compat headers
│   ├── debug.h                # Debug macros (9 levels)
│   ├── mytypes.h              # Cross-platform type definitions
│   ├── wingetopt.c/h          # Windows getopt compatibility
│   └── makefile               # GCC build (Linux/macOS)
│
├── console2udp/src/            # UDP telnet-like client tool
│   ├── telnet_udp.c           # UDP client for relay control
│   ├── net.c/h, webio.c/h    # Shared networking code
│   ├── arch.c/h, yselect.c/h # Shared platform code
│   └── makefile*              # Linux and macOS builds
│
├── win32/                      # Windows Visual Studio project
│   ├── SainSmartUsbRelay.sln
│   └── SainSmartUsbRelay.vcxproj
│
├── monitor.sh                  # Process monitoring script
├── README.md
└── LICENSE
```

## System Architecture

```
┌─────────────────────────────────────────────┐
│         SainSmart 16-CH Relay Board(s)      │
│         VID: 0x0416  PID: 0x5020            │
└──────────────────┬──────────────────────────┘
                   │  HID Raw (64-byte packets)
                   │
          /dev/hidraw* (Linux kernel)
                   │
                   │  ioctl / read / write
                   ▼
┌──────────────────────────────────────────────┐
│              usb-relay (daemon)               │
│                                              │
│  ┌────────────┐  ┌───────────┐  ┌────────┐  │
│  │ HID Proto  │  │ Command   │  │ Client │  │
│  │ Engine     │  │ Processor │  │ Mgmt   │  │
│  └────────────┘  └───────────┘  └────────┘  │
│  ┌────────────┐  ┌───────────┐  ┌────────┐  │
│  │ yselect    │  │ net/webio │  │ log    │  │
│  │ (I/O mux)  │  │ (sockets) │  │        │  │
│  └────────────┘  └───────────┘  └────────┘  │
└───────┬──────────────────┬───────────────────┘
        │                  │
   stdin/stdout        UDP :1026
        │                  │
        ▼                  ▼
   ┌──────────┐    ┌──────────────────┐
   │ Console  │    │   UDP Clients    │
   │ or       │    │  - udptelnet     │
   │websocketd│    │  - custom apps   │
   └──────────┘    │  - MIDI bridges  │
                   └──────────────────┘
```

## Hardware Protocol

### Device Identification
- **Interface:** Linux HID raw (`/dev/hidraw*`)
- **Vendor ID:** `0x0416` | **Product ID:** `0x5020`
- **Controller:** Nuvoton HID

### HID Packet Format (64 bytes)
```
Byte 0:     Command (WRITE=0xC3, READ=0xD2, ERASE=0x71)
Byte 1:     Length
Bytes 2-5:  Argument 1 (32-bit)
Bytes 6-9:  Argument 2 (32-bit)
Bytes 10-13: Signature (0x43444948 "CDIH")
Bytes 14-17: Checksum (additive sum of all preceding bytes)
Bytes 18-63: Padding
```

### State Representation
- Each 16-channel board = 4 hex digits (`0x0000`-`0xFFFF`)
- Up to 16 boards supported (256 channels max)
- Full state: 64 hex character string
- Bit-level mapping accounts for HID controller wiring order

## Communication

### UDP Protocol (Primary - Port 1026)
ASCII text commands over UDP datagrams (max 4096 bytes).

| Command | Description | Example |
|---------|-------------|---------|
| `get` | Read current relay state | `get` → `0000` |
| `set <mask> [hold]` | Set relay bitmask with optional hold time (100ms units) | `set FFFF 5` |
| `ping` | Heartbeat | `ping` → `pong` |
| `reset` | Reset device state | |
| `quit` | Shutdown daemon | |
| `disconnect` | Client disconnect | |

**Client management:** UDP endpoints tracked in a linked list with 6-minute expiry. Responses broadcast to all active clients.

### Interactive Mode (stdin/stdout)
Same command set via console. Can be wrapped by `websocketd` to expose as a WebSocket endpoint for browser-based control.

## Data Flow

1. **Inbound:** `yselect()` monitors UDP socket + stdin via `select()`
2. **Parse:** `process_command()` dispatches to appropriate handler
3. **Write:** `write_bitmask()` → `send_command(WRITE)` → HID packet → `/dev/hidraw*`
4. **Read:** `read_bitmask()` → `send_command(READ)` → parse response bytes 2-3
5. **Respond:** `send_status()` → reply to all tracked UDP clients

## Key Design Decisions

- **No external libraries.** Uses native Linux HID raw interface directly — no libusb, no libhidapi. Minimizes deployment complexity on embedded targets (Raspberry Pi).
- **UDP over TCP.** Eliminates connection overhead for real-time control. Client tracking is application-level with expiration.
- **Emulation mode** (`-e <boards>`). Enables development/testing without physical hardware.
- **Hold time mechanism.** Prevents rapid relay cycling that could damage hardware. Delays in 100ms increments.
- **Max on-time safety.** Auto-clears all relays after timeout (default 2s) to prevent stuck-on conditions — critical for fire effects.
- **Daemonization.** Runs as background service with PID file, syslog, and signal handling (SIGINT, SIGTERM, SIGSEGV).

## Build

### Linux
```bash
cd src && make
# Produces: usb-relay
# Requires: linux/hidraw.h, linux/input.h (kernel headers)
```

### Windows
Open `win32/SainSmartUsbRelay.sln` in Visual Studio.

### UDP Client
```bash
cd console2udp/src && make
# Produces: udptelnet
```

## Integration Points

| Integration | Method |
|-------------|--------|
| Web browser | `websocketd` wraps stdin/stdout → WebSocket |
| Web UI | [fire-controller-web](https://github.com/lowerpower/fire-controller-web) frontend |
| MIDI controllers | Custom UDP command generation |
| Custom apps | Direct UDP socket to port 1026 |
| Monitoring | `monitor.sh` for process supervision |

## Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| Linux (x86/ARM) | Primary | Full HID raw support |
| Raspberry Pi | Tested | Primary deployment target |
| macOS | Partial | Build support, limited HID |
| Windows | Project included | Visual Studio solution |
