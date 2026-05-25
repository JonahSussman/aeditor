#!/bin/sh
# usage: ./render_all.sh [MANIFEST_CSV] [OUTPUT_FILE]
# manifest format: FILENAME,START,LENGTH,WORD,COUNT,TOTAL,SEASON,EPISODE

MANIFEST=$1
OUTPUT=$2

if [ ! -f "$MANIFEST" ]; then
  echo "Error: manifest file '$MANIFEST' not found" >&2
  exit 1
fi

TOTAL_LINES=$(wc -l < "$MANIFEST")

rm -f data/clips/*.mkv
rm -f "$OUTPUT"

CURRENT=0
while IFS=',' read -r FILENAME START LENGTH WORD COUNT TOTAL SEASON EPISODE; do
  CURRENT=$((CURRENT + 1))
  echo "[$CURRENT/$TOTAL_LINES] Rendering: ${WORD}_${COUNT}"
  scripts/render.sh "$FILENAME" "$START" "$LENGTH" "$WORD" "$COUNT" "$TOTAL" "$SEASON" "$EPISODE" < /dev/null
done < "$MANIFEST"

echo "Building concat list..."
CONCAT_FILE="data/clips/_concat.txt"
> "$CONCAT_FILE"
while IFS=',' read -r FILENAME START LENGTH WORD COUNT TOTAL SEASON EPISODE; do
  SAFE_WORD=$(echo "$WORD" | tr -d "'")
  echo "file '${SAFE_WORD}_${COUNT}.mkv'" >> "$CONCAT_FILE"
done < "$MANIFEST"

echo "Concatenating $TOTAL_LINES clips..."
cd data/clips && ffmpeg -nostdin -hide_banner -f concat -safe 0 -i _concat.txt -c:v copy -c:a aac -ar 44100 -ac 1 "../../$OUTPUT" -y 2>&1
cd ../..

echo "Done. Output: $OUTPUT"
