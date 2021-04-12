# Time Stats:
| Season    | Time            |
|-----------|-----------------|
| s1        |  `09h 50m 07s`  |
| s2        |  `14h 57m 31s`  |
| s3        |  `13h 03m 22s`  |
| s4        |  `13h 26m 00s`  |
| extras    |  `02h 02m 22s`  |
| **TOTAL** |  `53h 19m 22s`  |

https://talesfromthecodemines.wordpress.com/2016/10/09/how-to-use-libvlc-on-windows/

Use gentle and google auto-subtitle somehow

Hash map to store word count total

SPELL CHECK

Load up clip with subtitles and have option to split clip equally via spaces

use trie

ffmpeg -i episode.mkv -vf scale=-1:720 -c:v libx264 -crf 0 -preset veryslow -c:a copy episode_720.mkv
ffmpeg -i episode.mkv -filter:v scale=1280:-1 -c:a copy episode_720.mkv

Word
- id
- Begin timestamp
- End timestamp

PulseAudio hosted at 127.0.0.1
Display at port 0
export DISPLAY=:0
export PULSE_SERVER=tcp:127.0.0.1
export LIBGL_ALWAYS_INDIRECT=Yes

Labelling program
- get audio, video, and if present subtitle streams
- if subtitle streams, generate intervals with names
- Use Gentle to force align words
-- Cut out audio
-- Put in subtitle thing
-- delete old interval, import new intervals
-- adjust mistakes
- split evenly at spaces if gentle doesn't work 
- Label words based on audio samples, frames are too imprecise!!
- Generate csv or whatever
- OVERALL STATS GETS SAVED SEPARATELY (can update by re-tallying all csvs in saved directory)

Compiling program
- concat all csvs together
- seperate into different letters (A B C, etc)
- order alphabetically
- go into each clip, trim, atrim, etc
- apply neat-looking overlays (calculate stats using global and letter csv)
- Pray nothing goes wrong when concat

SFML and sfeMovie!!!!!!

use wasdqe and ijkluo as two "joysticks"
ctrl + g to perform gentle analysis