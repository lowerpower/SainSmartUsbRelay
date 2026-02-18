#!/bin/bash
#
# Monitor Script for Fire-Midi Services
#
# Keeps usb-relay
#       websocketd
#              
#
# running
#
#



#files
LOGFILE="/tmp/monitor.txt"


RESTART="/tmp/restart"
RESTART_MIDI="/tmp/restart_midi"
RESTART_WEBSOCK="/tmp/restart_websock"

PID_DIR="/tmp"

BIN_DIR="/home/pi"

WEBSOCKETD="websocketd"
USBRELAY="usb-relay"
MIDI="midi2relay"


# kill $1
kill()
{
    local ret=1
    local PID="$PID_DIR/$1.pid"
    local pid

    if [ -e "$PID" ]; then
        pid2=$(cat "$PID")
        if [ -d "/proc/$pid2" ]; then
            echo "OK: $1 is running kill it" >> "$LOGFILE"
			command kill $pid2
			sleep 3
        else
            echo "FAIL: $1 is dead, cleanup pid file" >> "$LOGFILE"
            rm "$PID"
        fi
    fi

}


#check $1  
isRunning()
{
	local ret=1
	local PID="$PID_DIR/$1.pid"
    local pid

    if [ -e "$PID" ]; then
        pid2=$(cat "$PID")
        if [ -d "/proc/$pid2" ]; then
			ret=1
        else
            echo "FAIL: $1 is dead, cleanup pid file" >> "$LOGFILE"
			rm "$PID"
		fi
      fi
      if [ ! -e "$PID" ]; then
      	# start it up
        "$BIN_DIR/run-$1" &
        pid2=$!
        echo "$pid2" > "$PID"
        ret=0
        echo "OK: $1 has started" >> "$LOGFILE"
      fi
    return $ret
}


#
# Init here
logger "[FireMidi] monitor starting up"
echo "Starting Up" >> $LOGFILE

sleep 1

#
# Main Loop, this runs forever when device is running up
#
while [ 1 ]
do
	# check if restart files are written
    if [ -f "$RESTART" ]; then
        logger "[FireMidi] restart"
        echo "Restart All" >> "$LOGFILE"
        rm "$RESTART"
        killall "$WEBSOCKETD"
        killall "$USBRELAY"
        killall "$MIDI"
        sleep 2
    fi

    isRunning "$WEBSOCKETD"
    isRunning "$USBRELAY"
    isRunning "$MIDI"
    # check ever 15 seconds
    sleep 15
done






