:: usage: ./render.sh [BARE FILENAME (no ext)] [START (sec)] [LENGTH (sec)] [WORD] [COUNT] [TOTAL]

set FILENAME=%1
set START=%2
set LENGTH=%3
set WORD=%4
set COUNT=%5
set TOTAL=%6

ffmpeg -i %FILENAME.mkv -ss %START -t %LENGTH \
  -vf drawtext="fontfile='/home/jonah/.fonts/iosevka.ttf':text='%WORD':x=100:y=100" \
  data/clips/%WORD_%COUNT.mkv -y