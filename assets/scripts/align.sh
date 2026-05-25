#!/bin/sh

# usage: ./align.sh [BARE FILENAME (no ext)] [START (sec)] [DURATION (sec)]
# requires: conda create -n aligner -c conda-forge montreal-forced-aligner
#           conda run -n aligner mfa model download acoustic english_us_arpa
#           conda run -n aligner mfa model download dictionary english_us_arpa

MFA_ENV="${MFA_ENV:-aligner}"
MFA_DICT="${MFA_DICT:-english_us_arpa}"
MFA_MODEL="${MFA_MODEL:-english_us_arpa}"
INPUT=data/mfa_input
OUTPUT=data/mfa_output

echo "[1/4] Checking MFA installation..."
if ! conda run -n "$MFA_ENV" mfa version >/dev/null 2>&1; then
  echo "Error: MFA not found in conda env '$MFA_ENV'." >&2
  echo "Install with:" >&2
  echo "  conda create -n $MFA_ENV -c conda-forge montreal-forced-aligner" >&2
  echo "  conda run -n $MFA_ENV mfa model download acoustic $MFA_MODEL" >&2
  echo "  conda run -n $MFA_ENV mfa model download dictionary $MFA_DICT" >&2
  exit 1
fi

mkdir -p "$INPUT" "$OUTPUT"
rm -f "$INPUT/align.wav"
rm -f "$OUTPUT/align.TextGrid" "$OUTPUT/align.csv"

echo "[2/4] Extracting audio segment..."
ffmpeg -hide_banner -loglevel panic -ss $2 -i $1.mkv -t $3 -ar 16000 "$INPUT/align.wav" -y

echo "[3/4] Running forced alignment (this may take a moment)..."
conda run -n "$MFA_ENV" mfa align --clean "$INPUT" "$MFA_DICT" "$MFA_MODEL" "$OUTPUT" -j 12 --verbose 2>&1 || exit 1

echo "[4/4] Converting TextGrid to CSV..."
python3 scripts/textgrid.py "$OUTPUT/align.TextGrid" -o "$OUTPUT/align.csv"

echo "Done."
