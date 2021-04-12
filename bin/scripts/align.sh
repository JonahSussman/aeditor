#!/bin/sh

# usage: ./align.sh [BARE FILENAME (no ext)] [START (sec)] [DURATION (sec)]

# Your password to linux should go here
PASSWORD="Cardgamesftw123"

ffmpeg -hide_banner -loglevel panic -ss $2 -i $1.mkv -t $3 -ar 16000 data/mfa_input/align.wav -y
# echo $4 > data/mfa_input/$1.txt

MFADIR=../montreal-forced-aligner
ALIGN=$MFADIR/bin/mfa_align
DICT=$MFADIR/pretrained_models/phineas-lexicon.txt
MODEL=$MFADIR/pretrained_models/english.zip
INPUT=data/mfa_input
OUTPUT=data/mfa_output

echo $PASSWORD | sudo -S $ALIGN --clean $INPUT $DICT $MODEL $OUTPUT --verbose -j 12 -n

python3 scripts/textgrid.py $OUTPUT/mfa_input/align.TextGrid -o $OUTPUT/align.csv
