#!/bin/bash

FIFO="fifo$RANDOM"
mkfifo $FIFO
vlc $FIFO 2>/dev/null&
MYIP="$1"
MYPORT="$2"
IP="$3"
PORT="$4"
FILE="$5"
sleep 2
echo "$MYIP:$MYPORT/$FILE" | nc $IP $PORT&
nc -l -p $MYPORT > $FIFO
rm -f $FIFO
