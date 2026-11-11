#!/bin/bash
# Scale spike: a labelled grid comparison video (MP4 + 960 px web copy) and a 1280 px JPEG sheet
# from the lossless stills at <sec> s. 2 or 4 inputs give 2 columns, 6 give 3 columns.
# Usage: sheet.sh <out-base> <sec> <name>:<label> ...   (labels: no colons or commas)
# WEB_FPS (default 30) lowers the frame rate of the web copy only, to keep it small.
set -euo pipefail
base=$1 sec=$2; shift 2
cd "$HOME/fso/tc/scale"
n=$#
cols=2; [ "$n" -eq 6 ] && cols=3
vin=() sin=() fc="" stack="" layout=""
font=/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
for ((i = 0; i < n; i++)); do
  name=${1%%:*} label=${1#*:}; shift
  vin+=(-i "$name.mp4"); sin+=(-i "$name-s$sec.png")
  fc+="[$i]drawtext=fontfile=$font:text='$label':fontcolor=white:fontsize=30:x=14:y=12:box=1:boxcolor=black@0.6[p$i];"
  stack+="[p$i]"
  layout+="$(( (i % cols) * 1280 ))_$(( (i / cols) * 720 ))|"
done
fc+="${stack}xstack=inputs=$n:layout=${layout%|}"
ffmpeg -loglevel error -y "${vin[@]}" -filter_complex "$fc,format=yuv420p[v]" -map "[v]" -shortest \
  -c:v libx264 -crf 18 -preset medium -movflags +faststart "$base.mp4"
ffmpeg -loglevel error -y -i "$base.mp4" -vf "fps=${WEB_FPS:-30},scale=960:-2" -c:v libx264 -crf 28 -preset slow \
  -pix_fmt yuv420p -movflags +faststart "$base-960.mp4"
ffmpeg -loglevel error -y "${sin[@]}" -filter_complex "$fc,scale=1280:-2" -q:v 4 "$base-sheet.jpg"
ls -la "$base.mp4" "$base-960.mp4" "$base-sheet.jpg"
