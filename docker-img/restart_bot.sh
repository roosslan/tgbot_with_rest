#!/bin/bash
# cron's script for relaunch every midnight 
pkill -f listchecker
echo "listchecker killed"
pkill -f chanserv
echo "chanserv pkilled"

cd /home/rasa/chanserv/bin
sleep 10
./chanserv || true
echo "chanserv exited"
exit 0

