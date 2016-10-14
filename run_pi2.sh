#!/bin/sh

export QT_XKB_CONFIG_ROOT=/usr/share/X11/xkb

curr=`/bin/date +'%s'`
cd /home/pi/dev/rinf-gui
nohup /usr/bin/gpspipe -r -o logs/pipelog-$curr.gpslog &

while true ;do
 ./rinf-gui >/dev/null 2>/dev/null
 err=$?
 if [ $err -ne 0 ]; then
  /usr/bin/aplay program_crash.wav
 else
  /usr/bin/aplay program_stop.wav
  exit 0
 fi
 /usr/bin/aplay program_restart.wav
done
