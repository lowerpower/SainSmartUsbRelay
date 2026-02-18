# Changelog

## 2026-02-17 — Code Audit & Bug Fix Release

Full security and correctness audit of the entire codebase. 66 issues identified and resolved across `src/`, `console2udp/`, build scripts, and documentation.

### Critical Fixes

- **C1** `src/usb-relay.c` — `calculate_checksum()` used `=` instead of `+=`. Checksum was just the last byte, not an additive sum.
- **C2** `src/usb-relay.c` — `padd_set_string()` buffer underflow. Wrote before buffer start via `*--ret` when `mod==0`.
- **C3** `src/usb-relay.h` — `emulation_state_string[STATE_SIZE]` off-by-one. 16 boards x 4 hex + null = 65 bytes into 64-byte buffer.
- **C4** `src/usb-relay.h` — `STATE_SIZE` macro missing parentheses. `MAX_RELAY_BOARDS*4` changed to `(MAX_RELAY_BOARDS*4)`.
- **C5** `monitor.sh` — `kill()` shell function recursively called itself instead of system `kill`. Infinite recursion crash.
- **C6** `console2udp/src/telnet_udp.c` — `printf("%s", current_mtu)` where `current_mtu` is `int`. Segfault on format mismatch.
- **C7** `console2udp/src/net.c` — `read_sock_web_header` called with uninitialized `data_length` (0) as buffer size.

### High-Priority Fixes

- **H1** `src/usb-relay.c` — `read_current_state()` processed uninitialized buffer on read failure. `if(ret)` was true for -1.
- **H2** `src/usb-relay.c` — `read_data()` always called `read()` even when `select()` timed out.
- **H3** `src/usb-relay.c` — `find_relay_devices()` had no bounds check on `board_count` vs `MAX_RELAY_BOARDS`.
- **H4** `src/usb-relay.c` — `send_command()` return value ignored in `write_state`, `read_current_state`, `reset_board`.
- **H5** `src/log.c` — `vsprintf()` into fixed buffers with no length limit. Changed to `vsnprintf`.
- **H6** `src/usb-relay.c` — `open()` check used `<= 0` instead of `< 0`. fd 0 is valid.
- **H7** `src/usb-relay.c` — `strcpy`/`strcat` in `is_device_relay()` with no bounds check. Changed to `snprintf`.
- **H8** `src/makefile` — Added compiler hardening flags (`-fstack-protector-strong`, `-D_FORTIFY_SOURCE=2`, `-Wformat`, `-Wformat-security`).
- **H9** `console2udp/src/telnet_udp.c` — NULL dereference: `strlen(strtok_r(...))` without NULL check.
- **H10** `console2udp/src/net.c` — `sizeof(WEB_RESP_HEADER_LEN)` evaluated to `sizeof(int)` = 4. Only cleared 4 of 2048 bytes.
- **H11** `console2udp/src/net.c` — `memset` called before NULL check on `malloc` result.
- **H12** `console2udp/src/net.c` — `strcmp` logic inverted in HTTP response parsing. Changed to `strncmp(...)==0`.
- **H13** `console2udp/src/net.c` — Stack overflow in `curl_get` via unbounded `sprintf`/`strcat`. Changed to single `snprintf`.
- **H14** `console2udp/src/webio.c` — `SSL_VERIFY_NONE` documented as intentional for embedded targets without CA certs.

### Medium-Priority Fixes

- **M1** `src/usb-relay.c` — `sanity_test()` undefined behavior: `1 << i` when `i >= 32`. Added cast to `unsigned long long`.
- **M2** `src/usb-relay.c` — `clear_all()` ignored `process_command()` return. Safety shutoff now logs on failure.
- **M3** `src/usb-relay.c` — Signal handler called non-async-safe functions (`printf`, `syslog`, `backtrace`). Stripped to async-signal-safe only.
- **M4** `src/usb-relay.c` — `memset` sizeof mismatch: `sizeof(struct sockaddr)` vs `sizeof(struct sockaddr_in)`.
- **M5** `src/arch.c` — `trim()` accessed `p[-1]` on empty string. Added length guard.
- **M6** `src/arch.c` — `readln_from_a_buffer()` used `||` instead of `&&`. Loop only stopped at null, not CR/LF.
- **M7** `src/arch.c` — `assert` macros used `||` instead of `&&`. Allowed NULL through then dereferenced.
- **M8** `src/daemonize.c` — `freopen()` logic inverted. Missing `!` operators caused warning on success.
- **M9** `src/usb-relay.c` — Added `Yoics_Init_Select()` call in `main()` so fd_sets are formally initialized.
- **M10** `src/usb-relay.c` — Hold time arithmetic guarded against U32 underflow on `ms_count()` wraparound.
- **M11** `console2udp/src/telnet_udp.c` — `sizeof(CMD_MAX_SIZE)` evaluated to 4, not 4096. Changed to `CMD_MAX_SIZE`.
- **M12** `console2udp/src/net.c` — Operator precedence bug: `ret = (read_sock_line(...) > 0)` assigned boolean instead of return value.
- **M13** `console2udp/src/net.c` — Timeout loop logic inverted. `timeout>=breaker` changed to `breaker>=timeout`.
- **M14** `console2udp/src/webio.c` — Non-SSL `WebIOConnect` missing `break` on success. Fell through to next address.
- **M15** `monitor.sh` — Quoted all variable expansions throughout.
- **M16** `monitor.sh` — PID capture via `ps | grep` replaced with `$!` (direct background PID).
- **M17** `README.md` — Removed contradictory "completely untested" statement.
- **M18** `README.md` — Fixed documented flag `-3` to actual flag `-e`.
- **M19** `src/makefile` — Wired up `DEPENDALL` as prerequisite so header changes trigger rebuild.
- **M20** `src/makefile` — Removed undefined `$(MYLIB)` reference from clean target.

### Low-Priority Fixes

- **L1** `src/usb-relay.c` — Removed unreachable `break` after `return` in `bus_str()`.
- **L2** `src/usb-relay.c` — `lookup_client()` confirmed as dead code (never called).
- **L3** `src/usb-relay.c` — Renamed `remove_connection` parameter to `conn` to avoid shadowing function name.
- **L4** `src/arch.c` — `ysleep_usec` Windows path divided by 100 instead of 1000 (10x too long).
- **L5** `src/makefile` — Removed duplicate `$(INCLUDE)` from CFLAGS.
- **L6** `src/makefile` — Deduplicated `yselect.h` in DEPENDALL; added missing `config.h`, `arch.h`, `debug.h`, `log.h`.
- **L7** `.gitignore` — Confirmed build artifacts are not tracked. Non-issue.
- **L8** `README.md` — Fixed ~17 spelling errors throughout.

### Other Changes

- **`net.h`** — Added parentheses to `WEB_RESP_HEADER_LEN` macro: `1024*2` changed to `(1024*2)`.

### Documentation

- Added `ARCHITECTURE.md` — project-level architecture documentation covering system design, HID protocol, command processing, and data flow.
- Added `console2udp/ARCHITECTURE.md` — client architecture documentation covering operating modes, event loop, MTU discovery, platform support, and integration.
- Added `TODO.md` — full issue tracker (all items resolved).
