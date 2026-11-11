#!/bin/bash
# Usage: scale.sh <name> <px py pz vel%> <seconds> [extra fso args...]
# Scale spike copy of dof/tools/dof.sh (xin.py stays in gtao/tools). Writes the player start into the ISD test mission, starts FotG, clicks through
# to the game, chase view + HUD off, records <seconds> of the game area to
# <name>.mp4 and stills at 3 s intervals, then stops the game.
set -euo pipefail
name="$1"; read -r px py pz pv <<<"$2"; secs="$3"; shift 3
out="$HOME/fso/tc/scale"; mkdir -p "$out"
tools="$HOME/fso/tc/gtao/tools"
log="$out/$name.log"
root="$HOME/fso/tc/fotg"
mis="$root/GTAO_Test/data/missions"
# Heading: yaw (deg, 0 = toward -z, the bridge) and pitch (deg, + = nose down).
orient=$(python3 -c "import math,sys
y,p=map(math.radians,map(float,sys.argv[1:3]))
f=(-math.sin(y)*math.cos(p),-math.sin(p),-math.cos(y)*math.cos(p))
u=(-math.sin(y)*math.sin(p),math.cos(p),-math.cos(y)*math.sin(p))
r=(u[1]*f[2]-u[2]*f[1],u[2]*f[0]-u[0]*f[2],u[0]*f[1]-u[1]*f[0])
print(',\\\\n\\\\t'.join(', '.join('%.6f'%c for c in v) for v in (r,u,f)))" "${YAW:-0}" "${PITCH:-0}")
sed -e "s/PX, PY, PZ/$px, $py, $pz/" -e "s/Initial Velocity: PV/Initial Velocity: $pv/" -e "s/ORIENT/$orient/" \
  "$mis/gtao_isd.fs2" >"$mis/scale_run.fs2"
export DISPLAY=:0 XAUTHORITY=/run/user/1000/gdm/Xauthority
cd "$root"
setsid /dev/shm/fs2open-scale/build/bin/fs2_open_26_3_0_x64_AVX2 \
  -mod GTAO_Test,FG_Core,FG_Effects,FG_Maps1,FG_Maps2,FG_Maps3,FG_Maps4,FG_Maps5,FG_Models,FG_Music,FG_SndVA,FG_UI \
  -pilot asdf -window -res 1280x720 -nomusic -nosound -stdout_log -soft_particles \
  -start_mission scale_run "$@" >"$log" 2>&1 </dev/null &
pid=$!
echo "game pid $pid"
trap 'pkill -P "$pid" -x zenity || true; kill -9 "$pid" 2>/dev/null || true' EXIT
wait_log() { timeout 120 bash -c "until grep -a -q '$1' '$log'; do sleep 1; done"; }
xin() { python3 "$tools/xin.py" "$@"; }
# FotG may skip the briefings (0 stages); click Commit only while one is shown.
until grep -a -q 'Got event GS_EVENT_ENTER_GAME' "$log"; do
  kill -0 "$pid" || { echo "game exited"; exit 1; }
  if pgrep -P "$pid" -x zenity >/dev/null; then echo "error dialog:"; grep -a -m3 "Error" "$log"; exit 1; fi
  if grep -a -q 'Got event GS_EVENT_START_BRIEFING\|Got event GS_EVENT_CMD_BRIEF' "$log"; then
    sleep 3; read -r x y < <(xin geom); xin click $((x + 1068)) $((y + 690)) || true; sleep 2
  else sleep 1; fi
done
sleep 2
read -r x y < <(xin geom)
[ "${VIEW:-chase}" = chase ] && xin key KP_Multiply     # chase view
[ "${HUD:-off}" = off ] && xin key O Shift_L       # HUD off (HUD=on keeps it)
sleep 1
ffmpeg -loglevel error -y -f x11grab -framerate 30 -video_size 1280x720 -i ":0+$x,$y" -t "$secs" \
  -c:v libx264 -crf 18 -preset veryfast -pix_fmt yuv420p "$out/$name.mp4" &
rec=$!
# SNAPS="<sec> ...": lossless PNG grabs of the game area at these seconds after recording starts.
snaps=()
for t in ${SNAPS:-}; do
  (sleep "$t"; ffmpeg -loglevel error -y -f x11grab -video_size 1280x720 -i ":0+$x,$y" -frames:v 1 "$out/$name-s$t.png") &
  snaps+=($!)
done
# With KEY (e.g. bracketright = 2/3 throttle) the ship waits at rest and starts 1 s into the recording.
if [ -n "${KEY:-}" ]; then sleep 1; xin key "$KEY"; fi
# KEYS="<sec>:<keysym> ...": more timed keys after recording starts (seconds from start, ascending).
now=1
for k in ${KEYS:-}; do
  at=${k%%:*}; sleep $((at - now)); now=$at
  # "click" = left mouse button at the window centre (FotG binds fire primary to it).
  if [ "${k#*:}" = click ]; then xin click $((x + 640)) $((y + 360)); else xin key "${k#*:}"; fi
done
wait "$rec"
for p in "${snaps[@]}"; do wait "$p"; done
for t in $(seq 1 3 $((secs - 1))); do
  ffmpeg -loglevel error -y -ss "$t" -i "$out/$name.mp4" -frames:v 1 "$out/$name-t$t.png"
done
sleep "${TAIL_SECS:-2}"
