import sys
import os
import re
import string

import csv
import srt

def srt_to_csv(filename):
    with open(filename, "r") as subtitle_file:
        subs = list(srt.parse(subtitle_file.read()))
    
    pattern = re.compile("[^A-Za-z0-9 '-]+")

    with open(os.path.splitext(filename)[0] + ".csv", "w+") as csv_file:
        rowwriter = csv.writer(csv_file, delimiter=',')

        season  = int(input("Season? "))
        episode = int(input("Episode? "))
        # Row format: season, epsode, start, end, str
        for subtitle in subs:
            #TODO: Make format better
            content = pattern.sub(' ', subtitle.content.replace('\n', ' ')).lower()

            start = int(subtitle.start.total_seconds() * 1000)
            end   = int(subtitle.end.total_seconds() * 1000)

            rowwriter.writerow([season, episode, start, end, content])
    

if (__name__ == "__main__"):
    filename = ""
    if (len(sys.argv) < 2):
        filename = input('Filename? ')
    else:
        filename = sys.argv[1]

    srt_to_csv(filename)
