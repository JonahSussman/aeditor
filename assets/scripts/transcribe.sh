#!/bin/sh

# usage: ./transcribe.sh [BARE FILENAME (no ext)] [START (sec)] [DURATION (sec)]
# requires: uv tool install openai-whisper

WHISPER_MODEL="${WHISPER_MODEL:-base}"

if ! command -v whisper >/dev/null 2>&1; then
  echo "Error: whisper not found. Install with: uv tool install openai-whisper" >&2
  exit 1
fi

mkdir -p data
rm -f data/transcribe_input.wav data/transcribe_input.txt data/transcribe_output.txt

echo "[1/3] Extracting audio segment..."
ffmpeg -hide_banner -loglevel panic -ss $2 -i $1.mkv -t $3 -ar 16000 data/transcribe_input.wav -y

echo "[2/3] Transcribing with Whisper (model: $WHISPER_MODEL)..."
whisper data/transcribe_input.wav --model "$WHISPER_MODEL" --output_format txt --output_dir data/ --language en \
  --initial_prompt "Um, uh, like, you know"

if [ -f data/transcribe_input.txt ]; then
  echo "[3/3] Cleaning up output..."
  sed "s/[^a-zA-Z0-9 '\\n]//g" data/transcribe_input.txt | tr '[:upper:]' '[:lower:]' > data/transcribe_output.txt
else
  echo "Error: whisper produced no output" >&2
  exit 1
fi

echo "Done."
