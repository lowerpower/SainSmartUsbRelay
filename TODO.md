# SainSmartUsbRelay - Bug & Issue Tracker

## CRITICAL

- [x] **C1** `src/usb-relay.c:28` — `calculate_checksum()` uses `=` instead of `+=`. Checksum is just last byte, not additive sum.
- [x] **C2** `src/usb-relay.c:549-562` — `padd_set_string()` buffer underflow. Writes before buffer via `*--ret`. When `mod==0`, writes 4 bytes before pointer.
- [x] **C3** `src/usb-relay.h:122` — `emulation_state_string[STATE_SIZE]` off-by-one. 16 boards x 4 hex + null = 65 bytes into 64-byte buffer.
- [x] **C4** `src/usb-relay.h:74` — `STATE_SIZE` macro missing parentheses. `MAX_RELAY_BOARDS*4` should be `(MAX_RELAY_BOARDS*4)`.
- [x] **C5** `monitor.sh:33-51` — `kill()` function recursively calls itself instead of system `kill`. Infinite recursion crash.
- [x] **C6** `console2udp/src/telnet_udp.c:330` — `printf("%s", current_mtu)` where `current_mtu` is `int`. Segfault.
- [x] **C7** `console2udp/src/net.c:1364` — `read_sock_web_header` called with uninitialized `data_length` as buffer size.

## HIGH

- [x] **H1** `src/usb-relay.c:179` — `read_current_state()` processes uninitialized `buffer[128]` on read failure. `if(ret)` is true for -1.
- [x] **H2** `src/usb-relay.c:111-154` — `read_data()` always calls `read()` even when `select()` times out.
- [x] **H3** `src/usb-relay.c:288-378` — `find_relay_devices()` no bounds check on `board_count` vs `MAX_RELAY_BOARDS`.
- [x] **H4** `src/usb-relay.c:84,173,218` — `send_command()` return value ignored in `write_state`, `read_current_state`, `reset_board`.
- [x] **H5** `src/log.c:76,135` — `vsprintf()` into fixed buffers (1024, 256) with no length limit. Use `vsnprintf`.
- [x] **H6** `src/usb-relay.c:342` — `open()` check uses `<= 0` instead of `< 0`. fd 0 is valid.
- [x] **H7** `src/usb-relay.c:246-256` — `strcpy`/`strcat` in `is_device_relay()` with no bounds check on 256-byte buffer.
- [x] **H8** `src/makefile` — No compiler hardening flags (`-fstack-protector-strong`, `-D_FORTIFY_SOURCE=2`, etc.).
- [x] **H9** `console2udp/src/telnet_udp.c:161` — NULL deref: `strlen(strtok_r(...))` without NULL check.
- [x] **H10** `console2udp/src/net.c:1270` — `sizeof(WEB_RESP_HEADER_LEN)` = `sizeof(int)` = 4. Only clears 4 of 2048 bytes.
- [x] **H11** `console2udp/src/net.c:1263` — `memset` before NULL check on `malloc` result.
- [x] **H12** `console2udp/src/net.c:1324` — `strcmp` logic inverted in HTTP response parsing.
- [x] **H13** `console2udp/src/net.c:1425` — Stack overflow in `curl_get` via unbounded `sprintf`/`strcat`.
- [x] **H14** `console2udp/src/webio.c:117` — SSL verification disabled (`SSL_VERIFY_NONE`). Documented as intentional for embedded targets without CA certs.

## MEDIUM

- [x] **M1** `src/usb-relay.c:539` — `sanity_test()` undefined behavior: `1 << i` when `i >= 32`. Cast happens after shift.
- [x] **M2** `src/usb-relay.c:676-714` — `clear_all()` ignores `process_command()` return. Safety shutoff doesn't verify success.
- [x] **M3** `src/usb-relay.c:932-961` — Signal handler calls non-async-safe functions (`printf`, `syslog`, `backtrace`, `free`).
- [x] **M4** `src/usb-relay.c:855` — `memset` sizeof mismatch: `sizeof(struct sockaddr)` vs `sizeof(struct sockaddr_in)`.
- [x] **M5** `src/arch.c:631-639` — `trim()` accesses `p[-1]` on empty string (undefined behavior).
- [x] **M6** `src/arch.c:970-972` — `readln_from_a_buffer()` uses `||` instead of `&&`. Loop only stops at null, not CR/LF.
- [x] **M7** `src/arch.c:728,754,781,816` — `assert` uses `||` instead of `&&`. Allows NULL through then dereferences.
- [x] **M8** `src/daemonize.c:112-118` — `freopen()` logic inverted. `!` only on first call; warning fires on success.
- [x] **M9** `src/yselect.c:43` — `Yoics_Init_Select()` never called from `main()`. fd_sets not formally initialized.
- [x] **M10** `src/usb-relay.c:1233` — U32 timer wraparound. `ms_count()` wraps every ~49.7 days. Hold time arithmetic underflows.
- [x] **M11** `console2udp/src/telnet_udp.c:242` — `memset(message, '.', sizeof(CMD_MAX_SIZE))` clears 4 bytes not 4096.
- [x] **M12** `console2udp/src/net.c:1110` — Operator precedence bug: `ret = (read > 0)` instead of `(ret = read) > 0`.
- [x] **M13** `console2udp/src/net.c:1070` — Timeout loop logic inverted. Exits immediately instead of retrying.
- [x] **M14** `console2udp/src/webio.c:338` — Non-SSL `WebIOConnect` missing `break` on success.
- [x] **M15** `monitor.sh:40-63` — Unquoted variable expansions throughout.
- [x] **M16** `monitor.sh:73` — PID capture via `ps | grep` races and can match wrong process.
- [x] **M17** `README.md:4,10` — Contradicts itself: "fully tested" vs "completely untested".
- [x] **M18** `README.md:67` — Documents flag `-3` but actual flag is `-e`.
- [x] **M19** `src/makefile:28` — `DEPENDALL` defined but never used in rules. Header changes don't trigger rebuild.
- [x] **M20** `src/makefile:39` — `$(MYLIB)` referenced but never defined.

## LOW

- [x] **L1** `src/usb-relay.c:223-243` — `bus_str()` has unreachable `break` after `return`.
- [x] **L2** `src/usb-relay.c:763-767` — `lookup_client()` stub always returns NULL. Dead code, never called.
- [x] **L3** `src/usb-relay.c:770` — Parameter name `remove_connection` shadows function name.
- [x] **L4** `src/arch.c:335` — `ysleep_usec` divides by 100 instead of 1000 on Windows (10x too long).
- [x] **L5** `src/makefile:17` — `$(INCLUDE)` duplicated in CFLAGS. Fixed in prior commit.
- [x] **L6** `src/makefile:28` — `yselect.h` listed twice in DEPENDALL; missing `config.h`, `arch.h`, `debug.h`, `log.h`.
- [x] **L7** `.gitignore` — Build artifacts (`.o`, binaries) not actually tracked. Non-issue.
- [x] **L8** `README.md` — ~17 spelling errors throughout.
