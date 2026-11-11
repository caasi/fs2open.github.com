#!/bin/bash
# DoF spike round 2: the main comparison as a labelled 2x2 video (MP4 + 960 px web copy) and a
# 1280 px JPEG sheet from the lossless stills at <sec> s.
# Usage: r2-sheet.sh <out-base> <sec> <name>:<label> x4   (labels: no colons or commas)
set -euo pipefail
base=$1 sec=$2; shift 2
cd "$HOME/fso/tc/dof"
vin=() sin=() fc=""
font=/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
for i in 0 1 2 3; do
  name=${1%%:*} label=${1#*:}; shift
  vin+=(-i "$name.mp4"); sin+=(-i "$name-s$sec.png")
  fc+="[$i]drawtext=fontfile=$font:text='$label':fontcolor=white:fontsize=28:x=14:y=12:box=1:boxcolor=black@0.5[p$i];"
done
fc+="[p0][p1]hstack[t];[p2][p3]hstack[b];[t][b]vstack"
ffmpeg -loglevel error -y "${vin[@]}" -filter_complex "$fc,format=yuv420p[v]" -map "[v]" \
  -c:v libx264 -crf 18 -preset medium -movflags +faststart "$base.mp4"
ffmpeg -loglevel error -y -i "$base.mp4" -vf scale=960:-2 -c:v libx264 -crf 28 -preset medium \
  -pix_fmt yuv420p -movflags +faststart "$base-960.mp4"
ffmpeg -loglevel error -y "${sin[@]}" -filter_complex "$fc,scale=1280:-2" -q:v 3 "$base-sheet.jpg"
ls -la "$base.mp4" "$base-960.mp4" "$base-sheet.jpg"
