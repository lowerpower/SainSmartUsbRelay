/*
 * relay16.c - SainSmart 101-70-208 serial (CH340) relay boards running the
 * relay16 firmware: https://github.com/lowerpower/sainsmart-relay16
 *
 * Copyright (c) 2026 mycal
 *
 * The board speaks plain ASCII at 115200 8N1, one command per line, and
 * answers every command with one line:
 *
 *      h 00F0      set all 16 relays       -> "00F0" (the state now on the board)
 *      r           read the state          -> "00F0"
 *      t 1000      host-silence failsafe   -> "1000" (ms, 0 = off)
 *      v           version and board ID    -> "relay16 1L id 3"
 *
 * The CH340 has no serial number, so the order of /dev/ttyUSB* says nothing
 * about which board is which. Each board stores its own ID in EEPROM instead,
 * and every board is placed in the set bitmask by that ID:
 *
 *  - default: board ID N is position N, the Nth group of 4 hex digits from
 *    the right. "set 00010000" turns on relay 1 of board ID 2.
 *  - -b 11,12,13: position 1 is board ID 11, position 2 is board ID 12, ...
 *
 * A board never moves to another position. A missing board leaves its
 * position empty (its relays are skipped) and the others stay put, so a
 * command meant for a small poofer can never land on the board next to it.
 * Two boards with the same ID is fatal at startup.
 *
 * Writes go to every changed board first and the replies are collected after,
 * so a set across several boards costs about one round trip (~3 ms), not one
 * per board. Every reply is checked against what was sent.
 *
 * Safety:
 *  - each board is set all-off when it is attached
 *  - the board's own failsafe (-f, default 1000 ms) turns its relays off if
 *    this program stops talking to it (crash, hang, USB unplugged); idle
 *    boards get an "r" keepalive every failsafe/4 ms
 *  - a board that disappears is reattached by ID when it comes back, and set
 *    all-off again
 *  - polling also catches a board that reset or tripped its failsafe; the
 *    reported state is then corrected to what the board actually has
 */

#include <ctype.h>
#include "usb-relay.h"

#if defined(LINUX)

#include <termios.h>
#include <syslog.h>
#include <stdarg.h>
#include <limits.h>

#define R16_REPLY_MS        50      // a reply normally takes ~3 ms
#define R16_IDLE_POLL_MS    1000    // health poll when the board failsafe is off
#define R16_MIN_POLL_MS     20
#define R16_RESCAN_MS       2000    // look for missing boards this often
#define R16_MAX_PORTS       32
#define R16_DEVLEN          128

//
// Warnings go to stderr and syslog, never stdout: under websocketd stdout is
// the websocket.
//
static void
r16_warn(const char *fmt, ...)
{
    char    msg[512];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    fprintf(stderr, "usb-relay: %s\n", msg);
    syslog(LOG_WARNING, "%s", msg);
}

//
// Open a serial port raw at 115200 8N1, non blocking, exclusive.
//
static int
r16_open(const char *dev)
{
    struct termios  tio;
    int             fd;

    fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
        return(-1);
    // keep other programs (ModemManager, a stray terminal) off the board
    ioctl(fd, TIOCEXCL);
    if (tcgetattr(fd, &tio) < 0)
    {
        close(fd);
        return(-1);
    }
    cfmakeraw(&tio);
    cfsetispeed(&tio, B115200);
    cfsetospeed(&tio, B115200);
    tio.c_cflag |= CLOCAL | CREAD;
    tio.c_cflag &= ~(CSTOPB | PARENB | CRTSCTS);
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 0;
    if (tcsetattr(fd, TCSANOW, &tio) < 0)
    {
        close(fd);
        return(-1);
    }
    return(fd);
}

//
// Send one command line. Anything left in the receive buffer is stale (a
// late reply to a command that timed out) and is dropped first.
//
static int
r16_send(RELAY *r, const char *line)
{
    int len = strlen(line);
    int ret;

    tcflush(r->fd, TCIFLUSH);
    r->rxlen = 0;
    ret = write(r->fd, line, len);
    r->last_tx = ms_count();
    return((ret == len) ? 0 : -1);
}

//
// Read what has arrived, waiting up to timeout_ms.
// Returns bytes read, 0 on timeout, -1 if the port is gone.
//
static int
r16_fill(RELAY *r, int timeout_ms)
{
    fd_set          fds;
    struct timeval  tv;
    int             ret;

    FD_ZERO(&fds);
    FD_SET(r->fd, &fds);
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    ret = select(r->fd + 1, &fds, NULL, NULL, &tv);
    if (ret < 0)
        return((errno == EINTR) ? 0 : -1);
    if (ret == 0)
        return(0);
    ret = read(r->fd, r->rxbuf + r->rxlen, sizeof(r->rxbuf) - r->rxlen);
    if (ret > 0)
    {
        r->rxlen += ret;
        return(ret);
    }
    if ((ret < 0) && ((errno == EAGAIN) || (errno == EINTR)))
        return(0);
    // readable but 0 bytes is a hangup: the board was unplugged
    return(-1);
}

//
// Collect one non-empty reply line, without CR/LF, by the deadline.
// Returns its length, 0 on timeout, -1 if the port is gone.
//
static int
r16_reply(RELAY *r, char *reply, int len, U32 deadline)
{
    for (;;)
    {
        char *eol = memchr(r->rxbuf, '\n', r->rxlen);

        if (eol)
        {
            int start = 0, end = eol - r->rxbuf, used = end + 1, n;

            while ((start < end) && isspace((unsigned char)r->rxbuf[start]))
                start++;
            while ((end > start) && isspace((unsigned char)r->rxbuf[end - 1]))
                end--;
            n = end - start;
            if (n > len - 1)
                n = len - 1;
            memcpy(reply, r->rxbuf + start, n);
            reply[n] = 0;
            memmove(r->rxbuf, r->rxbuf + used, r->rxlen - used);
            r->rxlen -= used;
            if (n > 0)
                return(n);
            continue;               // blank line, keep looking
        }
        if (r->rxlen >= (int)sizeof(r->rxbuf))
            r->rxlen = 0;           // line noise with no newline
        {
            int left = (int)(deadline - ms_count());

            if (left <= 0)
                return(0);
            if (r16_fill(r, left) < 0)
                return(-1);
        }
    }
}

static int
r16_command(RELAY *r, const char *line, char *reply, int len)
{
    if (r16_send(r, line) < 0)
        return(-1);
    return(r16_reply(r, reply, len, ms_count() + R16_REPLY_MS));
}

//
// A state reply is exactly 4 hex digits. Returns the state or -1.
//
static int
r16_parse_state(const char *s)
{
    int i;

    if (strlen(s) != 4)
        return(-1);
    for (i = 0; i < 4; i++)
        if (!isxdigit((unsigned char)s[i]))
            return(-1);
    return((int)strtol(s, NULL, 16));
}

static int
r16_interval(RELAY *r)
{
    int ms = r->failsafe ? r->failsafe / 4 : R16_IDLE_POLL_MS;

    return((ms < R16_MIN_POLL_MS) ? R16_MIN_POLL_MS : ms);
}

static void
r16_lost(RELAY *r, const char *why)
{
    r16_warn("relay16 board ID %d on %s lost (%s); it is reattached when it comes back",
             r->board_id, r->device ? r->device : "?", why);
    if (r->fd >= 0)
        close(r->fd);
    r->fd = -1;
    r->active = 0;
    r->estate = 0;
    r->written_valid = 1;           // nothing to confirm on a board that is not there
    r->rxlen = 0;
}

//
// Open a port and ask who it is. Returns the open fd (board ID and firmware
// version filled in), or -1 if it is not a relay16 board.
//
static int
r16_probe(const char *dev, int *id, char *fw, int fwlen)
{
    RELAY   tmp;
    char    reply[64], f[16];
    int     i;

    memset(&tmp, 0, sizeof(tmp));
    tmp.fd = r16_open(dev);
    if (tmp.fd < 0)
        return(-1);
    // end any partial line left by line noise or another program
    if (write(tmp.fd, "\n", 1) != 1)
    {
        close(tmp.fd);
        return(-1);
    }
    ysleep_usec(20000);
    if ((r16_command(&tmp, "v\n", reply, sizeof(reply)) > 0) &&
        (2 == sscanf(reply, "relay16 %15s id %d", f, &i)) && (i >= 0) && (i <= 255))
    {
        *id = i;
        snprintf(fw, fwlen, "%s", f);
        return(tmp.fd);
    }
    close(tmp.fd);
    return(-1);
}

static int
r16_read_hex(const char *path)
{
    FILE    *f = fopen(path, "r");
    int     v = -1;

    if (f)
    {
        if (1 != fscanf(f, "%x", &v))
            v = -1;
        fclose(f);
    }
    return(v);
}

static int
r16_cmp(const void *a, const void *b)
{
    return(strcmp((const char *)a, (const char *)b));
}

//
// Ports to try: every -s device, then every tty whose USB device is a CH340.
//
static int
r16_candidates(RELAY_CONFIG *config, char devs[][R16_DEVLEN], int max)
{
    DIR             *dp;
    struct dirent   *ep;
    int             n = 0, i, first_auto;
    char            path[PATH_MAX], real[PATH_MAX], a[PATH_MAX], b[PATH_MAX];

    for (i = 0; (i < config->serial_dev_count) && (n < max); i++)
        snprintf(devs[n++], R16_DEVLEN, "%s", config->serial_dev[i]);
    first_auto = n;

    dp = opendir("/sys/class/tty");
    if (!dp)
        return(n);
    while ((ep = readdir(dp)) && (n < max))
    {
        int dup = 0;

        if (strncmp(ep->d_name, "ttyUSB", 6) && strncmp(ep->d_name, "ttyACM", 6))
            continue;
        // device -> the USB interface's tty node; ../.. is the USB device
        snprintf(path, sizeof(path), "/sys/class/tty/%s/device/../..", ep->d_name);
        if (!realpath(path, real))
            continue;
        if ((snprintf(path, sizeof(path), "%s/idVendor", real) >= (int)sizeof(path)) ||
            (r16_read_hex(path) != RELAY16_VID))
            continue;
        if ((snprintf(path, sizeof(path), "%s/idProduct", real) >= (int)sizeof(path)) ||
            (r16_read_hex(path) != RELAY16_PID))
            continue;
        if (snprintf(devs[n], R16_DEVLEN, "%s%s", config->dev_dir, ep->d_name) >= R16_DEVLEN)
            continue;
        // skip it if it was also given with -s (possibly by a by-id/by-path link)
        for (i = 0; i < first_auto; i++)
            if (realpath(devs[i], a) && realpath(devs[n], b) && !strcmp(a, b))
                dup = 1;
        if (!dup)
            n++;
    }
    closedir(dp);
    qsort(devs[first_auto], n - first_auto, R16_DEVLEN, r16_cmp);
    return(n);
}

static void
r16_slot_init(RELAY *r, int id)
{
    memset(r, 0, sizeof(*r));
    r->type = RELAY_TYPE_RELAY16;
    r->board_id = id;
    r->fd = -1;
    r->written_valid = 1;
}

//
// Put a probed board into its position: all relays off, failsafe set.
//
static int
r16_attach(RELAY_CONFIG *config, int slot, int fd, const char *dev, int id, const char *fw)
{
    RELAY   *r = &config->relays[slot];
    char    reply[32], cmd[32];

    if (r->device)
        free(r->device);
    r->device = strdup(dev);
    r->fd = fd;
    r->board_id = id;
    r->rxlen = 0;
    snprintf(r->fw, sizeof(r->fw), "%s", fw);

    if ((r16_command(r, "h 0000\n", reply, sizeof(reply)) <= 0) || (r16_parse_state(reply) != 0))
    {
        r16_warn("relay16 board ID %d on %s did not take all-off, not used", id, dev);
        close(fd);
        r->fd = -1;
        return(-1);
    }
    r->estate = 0;
    r->written_valid = 1;

    if (config->board_failsafe >= 0)
    {
        snprintf(cmd, sizeof(cmd), "t %d\n", config->board_failsafe);
        r16_command(r, cmd, reply, sizeof(reply));
    }
    r->failsafe = (r16_command(r, "t\n", reply, sizeof(reply)) > 0) ? atoi(reply) : 0;
    r->active = 1;

    if (config->verbose)
        fprintf(stderr, "usb-relay: relay16 board ID %d (firmware %s) on %s at position %d, failsafe %d ms\n",
                id, fw, dev, slot + 1, r->failsafe);
    return(0);
}

static int
r16_in_use(RELAY_CONFIG *config, const char *dev)
{
    int i;

    for (i = 0; i < config->board_count; i++)
        if (config->relays[i].active && config->relays[i].device &&
            !strcmp(config->relays[i].device, dev))
            return(1);
    return(0);
}

static int
r16_missing(RELAY_CONFIG *config)
{
    int i;

    for (i = 0; i < config->board_count; i++)
        if ((RELAY_TYPE_RELAY16 == config->relays[i].type) && !config->relays[i].active)
            return(1);
    return(0);
}

//
// Find relay16 boards and give each its position. Runs before the HID scan,
// so HID boards (which have no ID) are placed after the relay16 positions.
//
int
relay16_find(RELAY_CONFIG *config)
{
    char    devs[R16_MAX_PORTS][R16_DEVLEN];
    char    fws[R16_MAX_PORTS][16];
    int     fds[R16_MAX_PORTS], ids[R16_MAX_PORTS];
    int     n, i, j, s, found = 0, attached = 0;

    n = r16_candidates(config, devs, R16_MAX_PORTS);
    for (i = 0; i < n; i++)
    {
        fds[found] = r16_probe(devs[i], &ids[found], fws[found], sizeof(fws[found]));
        if (fds[found] < 0)
        {
            if (i < config->serial_dev_count)
                r16_warn("%s is not answering as a relay16 board", devs[i]);
            continue;
        }
        if (found != i)
            memcpy(devs[found], devs[i], R16_DEVLEN);
        found++;
    }

    // Two boards with one ID would share a position: refuse to run.
    for (i = 0; i < found; i++)
        for (j = i + 1; j < found; j++)
            if (ids[i] && (ids[i] == ids[j]))
            {
                r16_warn("FATAL: two relay16 boards report ID %d (%s and %s). Give each board its own ID "
                         "(relay16.py -p PORT i ID, then w) and restart.", ids[i], devs[i], devs[j]);
                for (s = 0; s < found; s++)
                    close(fds[s]);
                exit(3);
            }

    // Lay out the positions.
    if (config->board_map_count)
    {
        for (s = 0; s < config->board_map_count; s++)
            r16_slot_init(&config->relays[s], config->board_map[s]);
        config->board_count = config->board_map_count;
    }
    else
    {
        int maxid = 0;

        for (i = 0; i < found; i++)
            if ((ids[i] <= MAX_RELAY_BOARDS) && (ids[i] > maxid))
                maxid = ids[i];
        for (s = 0; s < maxid; s++)
            r16_slot_init(&config->relays[s], s + 1);
        config->board_count = maxid;
    }

    for (i = 0; i < found; i++)
    {
        int slot = -1;

        if (config->board_map_count)
        {
            for (s = 0; s < config->board_map_count; s++)
                if (config->board_map[s] == ids[i])
                    slot = s;
        }
        else if ((ids[i] >= 1) && (ids[i] <= MAX_RELAY_BOARDS))
            slot = ids[i] - 1;

        if (slot < 0)
        {
            if (0 == ids[i])
                r16_warn("relay16 board on %s has no ID (0), not used", devs[i]);
            else if (config->board_map_count)
                r16_warn("relay16 board ID %d on %s is not in the -b list, not used", ids[i], devs[i]);
            else
                r16_warn("relay16 board ID %d on %s is above %d; list it with -b to use it",
                         ids[i], devs[i], MAX_RELAY_BOARDS);
            close(fds[i]);
            continue;
        }
        if (0 == r16_attach(config, slot, fds[i], devs[i], ids[i], fws[i]))
            attached++;
    }

    for (s = 0; s < config->board_count; s++)
        if (!config->relays[s].active)
            r16_warn("relay16 board ID %d (position %d) not found; its relays are skipped until it is plugged in",
                     config->relays[s].board_id, s + 1);

    config->last_rescan = ms_count();
    return(attached);
}

//
// Reattach missing boards by ID.
//
static void
r16_rescan(RELAY_CONFIG *config)
{
    char    devs[R16_MAX_PORTS][R16_DEVLEN];
    char    fw[16];
    int     n, i, s, fd, id;

    n = r16_candidates(config, devs, R16_MAX_PORTS);
    for (i = 0; i < n; i++)
    {
        int slot = -1;

        if (r16_in_use(config, devs[i]))
            continue;
        fd = r16_probe(devs[i], &id, fw, sizeof(fw));
        if (fd < 0)
            continue;
        for (s = 0; s < config->board_count; s++)
            if ((RELAY_TYPE_RELAY16 == config->relays[s].type) && !config->relays[s].active &&
                (config->relays[s].board_id == id) && id)
                slot = s;
        if (slot < 0)
        {
            close(fd);
            continue;
        }
        if (0 == r16_attach(config, slot, fd, devs[i], id, fw))
            r16_warn("relay16 board ID %d back on %s (position %d), relays off", id, devs[i], slot + 1);
    }
}

//
// Start a set on one board. The reply is collected by relay16_collect().
//
int
relay16_send_state(RELAY *r, int state)
{
    char cmd[16];

    snprintf(cmd, sizeof(cmd), "h %04X\n", state & 0xffff);
    if (r16_send(r, cmd) < 0)
    {
        r16_lost(r, "write failed");
        return(-1);
    }
    return(0);
}

//
// Collect the replies of every board marked in pending[] (sent by
// relay16_send_state, or a poll). All the commands are already on the wire,
// so the boards answer in parallel and one deadline covers them all.
//
// is_poll = 0: a set; a wrong reply leaves the board unconfirmed so the next
//              set rewrites it.
// is_poll = 1: a keepalive read; a board whose state is not what we last
//              set changed by itself (reset, failsafe), so report the truth.
//
void
relay16_collect(RELAY_CONFIG *config, char *pending, int is_poll)
{
    U32     deadline = ms_count() + R16_REPLY_MS;
    char    reply[32];
    int     i, ret, state;

    for (i = 0; i < config->board_count; i++)
    {
        RELAY *r = &config->relays[i];

        if (!pending[i] || !r->active)
            continue;
        ret = r16_reply(r, reply, sizeof(reply), deadline);
        if (ret < 0)
        {
            r16_lost(r, "port closed");
            continue;
        }
        state = (ret > 0) ? r16_parse_state(reply) : -1;
        if (state == r->estate)
        {
            r->written_valid = 1;
            continue;
        }
        if (is_poll && (state >= 0))
        {
            r16_warn("relay16 board ID %d reports %04X, expected %04X (board reset or failsafe tripped)",
                     r->board_id, state, r->estate);
            r->estate = state;
            r->written_valid = 1;
        }
        else
        {
            if (config->verbose)
                r16_warn("relay16 board ID %d: %s (wanted %04X)", r->board_id,
                         ret ? reply : "no reply", r->estate);
            r->written_valid = 0;
        }
    }
}

//
// Call every pass of the main loop: keepalive reads to idle boards, and a
// periodic search for missing ones.
//
void
relay16_poll(RELAY_CONFIG *config)
{
    char    pending[MAX_RELAY_BOARDS];
    int     i, any = 0;
    U32     now = ms_count();

    memset(pending, 0, sizeof(pending));
    for (i = 0; i < config->board_count; i++)
    {
        RELAY *r = &config->relays[i];

        if ((RELAY_TYPE_RELAY16 != r->type) || !r->active)
            continue;
        if ((int)(now - r->last_tx) < r16_interval(r))
            continue;
        if (r16_send(r, "r\n") < 0)
        {
            r16_lost(r, "write failed");
            continue;
        }
        pending[i] = 1;
        any = 1;
    }
    if (any)
        relay16_collect(config, pending, 1);

    if (r16_missing(config) && ((int)(now - config->last_rescan) >= R16_RESCAN_MS))
    {
        config->last_rescan = now;
        r16_rescan(config);
    }
}

//
// How long the main loop may sleep before relay16_poll() has work: at most
// max_ms.
//
int
relay16_wait_ms(RELAY_CONFIG *config, int max_ms)
{
    int i, wait = max_ms;
    U32 now = ms_count();

    for (i = 0; i < config->board_count; i++)
    {
        RELAY *r = &config->relays[i];
        int left;

        if ((RELAY_TYPE_RELAY16 != r->type) || !r->active)
            continue;
        left = r16_interval(r) - (int)(now - r->last_tx);
        if (left < wait)
            wait = left;
    }
    if (r16_missing(config))
    {
        int left = R16_RESCAN_MS - (int)(now - config->last_rescan);

        if (left < wait)
            wait = left;
    }
    return((wait < 1) ? 1 : wait);
}

#else   // not LINUX: no serial support

int  relay16_find(RELAY_CONFIG *config) { return(0); }
int  relay16_send_state(RELAY *r, int state) { return(-1); }
void relay16_collect(RELAY_CONFIG *config, char *pending, int is_poll) { }
void relay16_poll(RELAY_CONFIG *config) { }
int  relay16_wait_ms(RELAY_CONFIG *config, int max_ms) { return(max_ms); }

#endif
