#!/bin/sh
# usage: ./render.sh [BARE FILENAME (no ext)] [START (sec)] [LENGTH (sec)] [WORD] [COUNT] [TOTAL] [SEASON] [EPISODE]

FILENAME=$1
START=$2
LENGTH=$3
WORD=$4
COUNT=$5
TOTAL=$6
SEASON=$7
EPISODE=$8

ffmpeg -i $FILENAME.mkv -ss $START -t $LENGTH \
  -vf "drawtext=fontfile='/home/jonah/.fonts/iosevka.ttf':fontsize='75':fontcolor='#ffffff':text='$WORD':x=(w-tw)/2:y='h-th-20':box=1:boxcolor='#0000007f':boxborderw='5', drawtext=fontfile='/home/jonah/.fonts/iosevka.ttf':fontsize='30':fontcolor='#ffffff':text='Times said\: $COUNT':x='20':y='h-th-20':box='1':boxcolor='#0000007f':boxborderw='5', drawtext=fontfile='/home/jonah/.fonts/iosevka.ttf':fontsize='30':fontcolor='#ffffff':text='Total words\: $TOTAL':x='20':y='20':box='1':boxcolor='#0000007f':boxborderw='5', drawtext=fontfile='/home/jonah/.fonts/iosevka.ttf':fontsize='30':fontcolor='#ffffff':text='S$SEASON E$EPISODE':x='w-tw-20':y='20':box='1':boxcolor='#0000007f':boxborderw='5', drawtext=fontfile='/home/jonah/.fonts/iosevka.ttf':fontsize='30':fontcolor='#ffffff':text='inothernews1':x='w-tw-20':y='h-th-20':box='1':boxcolor='#0000007f':boxborderw='5'" \
  data/clips/${WORD}_$COUNT.mkv -y