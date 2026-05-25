#!/bin/sh

# usage: ./align.sh [BARE FILENAME (no ext)] [START (sec)] [DURATION (sec)]
# requires: pip install whisperx (or uv tool install whisperx)

WHISPER_MODEL="${WHISPER_MODEL:-base}"

if ! command -v whisperx >/dev/null 2>&1; then
  echo "Error: whisperx not found. Install with: uv tool install whisperx" >&2
  exit 1
fi

mkdir -p data/mfa_input data/mfa_output

ffmpeg -hide_banner -loglevel panic -ss $2 -i $1.mkv -t $3 -ar 16000 data/mfa_input/align.wav -y

whisperx data/mfa_input/align.wav --model "$WHISPER_MODEL" --output_format json --output_dir data/mfa_output --language en 2>/dev/null

python3 scripts/whisperx_to_csv.py data/mfa_output/align.json -o data/mfa_output/align.csv
