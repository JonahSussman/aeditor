#!/bin/sh
# usage: ./render.sh [BARE FILENAME (no ext)] [START (sec)] [LENGTH (sec)] [WORD] [COUNT] [TOTAL] [SEASON] [EPISODE]

RENDER_FONT="${RENDER_FONT:-/home/jonah/.fonts/iosevka.ttf}"
RENDER_BRANDING="${RENDER_BRANDING:-inothernews1}"

FILENAME=$1
START=$2
LENGTH=$3
SAFE_WORD=$(echo "$4" | tr -d "'")
WORD=$SAFE_WORD
COUNT=$5
TOTAL=$6
SEASON=$7
EPISODE=$8

mkdir -p data/clips

ffmpeg -nostdin -hide_banner -loglevel panic -ss "$START" -i "$FILENAME.mkv" -t "$LENGTH" -avoid_negative_ts make_zero \
  -vf "drawtext=fontfile='$RENDER_FONT':fontsize='75':fontcolor='#ffffff':text='$WORD':x=(w-tw)/2:y='h-th-20':box=1:boxcolor='#0000007f':boxborderw='5', drawtext=fontfile='$RENDER_FONT':fontsize='30':fontcolor='#ffffff':text='Times said\: $COUNT':x='20':y='h-th-20':box='1':boxcolor='#0000007f':boxborderw='5', drawtext=fontfile='$RENDER_FONT':fontsize='30':fontcolor='#ffffff':text='Total words\: $TOTAL':x='20':y='20':box='1':boxcolor='#0000007f':boxborderw='5', drawtext=fontfile='$RENDER_FONT':fontsize='30':fontcolor='#ffffff':text='S${SEASON} E${EPISODE}':x='w-tw-20':y='20':box='1':boxcolor='#0000007f':boxborderw='5', drawtext=fontfile='$RENDER_FONT':fontsize='30':fontcolor='#ffffff':text='$RENDER_BRANDING':x='w-tw-20':y='h-th-20':box='1':boxcolor='#0000007f':boxborderw='5'" \
  -c:a aac -ar 44100 -ac 1 \
  "data/clips/${SAFE_WORD}_${COUNT}.mkv" -y

echo "Rendered: ${SAFE_WORD}_${COUNT}"
