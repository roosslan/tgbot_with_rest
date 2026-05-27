#!/bin/bash
# cron's script for relaunch every midnight 
pkill -f listchecker
echo "listchecker killed"
pkill -f chanserv
echo "chanserv pkilled"

cd ~/chanserv/bin
sleep 10
echo "Just add '|| true' after the command where you want to ignore the error..."
./chanserv || true
echo "Chanserv exited"
exit 0

