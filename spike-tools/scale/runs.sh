#!/bin/bash
# Scale spike: the flagship shot (r2.sh recipe: X-wing parked at -900 250 2200, yaw -22, pitch 6,
# 2/3 throttle from 1 s, chase view, 16 s) with the effects stacked one by one.
# Lossless stills at 4 and 12 s (<name>-s4.png, <name>-s12.png).
# Usage: runs.sh [run-name ...]   (no names = all runs)
set -uo pipefail
S="$HOME/fso/tc/scale/tools/scale.sh"; P="-900 250 2200 0"
export YAW=-22 PITCH=6 TAIL_SECS=3 KEY=bracketright SNAPS="4 12"
cd "$HOME/fso/tc/scale" || exit 1
HAZE="FSO_HAZE=${HAZE:-0.30 0.34 0.42 1000 1500}"
AO=(FSO_SSAO_RADIUS=60)
AMB=(-ambient "${AMB:-2.5}")
DOF=(FSO_DOF_SPLIT=1 FSO_DOF_SNAP=1 FSO_DOF_MAXBLUR=16 FSO_DOF_SAMPLES=96)
run() {  # run <name> <secs> [VAR=value ...] -- extra fso args
  local name=$1 secs=$2; shift 2
  if [ ${#only[@]} -gt 0 ] && [[ " ${only[*]} " != *" $name "* ]]; then return; fi
  local envs=()
  while [ $# -gt 0 ] && [ "$1" != -- ]; do envs+=("$1"); shift; done
  [ $# -gt 0 ] && shift
  env "${envs[@]}" "$S" "$name" "$P" "$secs" "$@"; echo "$name $?"
}
only=("$@")
run s1-off 16 --
run s2-haze 16 "$HAZE" --
run s3-hao 16 "$HAZE" "${AO[@]}" -- -ssao_spike "${AMB[@]}"
run s4-far-a 16 "$HAZE" "${AO[@]}" "${DOF[@]}" "FSO_DOF_FAR=0 1" -- -ssao_spike "${AMB[@]}" -dof_spike
run s4-far-b 16 "$HAZE" "${AO[@]}" "${DOF[@]}" "FSO_DOF_FAR=0.333 1" -- -ssao_spike "${AMB[@]}" -dof_spike
run s4-far-c 16 "$HAZE" "${AO[@]}" "${DOF[@]}" "FSO_DOF_FAR=0 0.5" -- -ssao_spike "${AMB[@]}" -dof_spike
run s4-far-d 16 "$HAZE" "${AO[@]}" "${DOF[@]}" "FSO_DOF_FAR=0.333 0.5" -- -ssao_spike "${AMB[@]}" -dof_spike
run s5-dust 16 "$HAZE" "${AO[@]}" "${DOF[@]}" "FSO_DOF_FAR=${FAR:-0.333 1}" FSO_DUST=SpikeDust -- -ssao_spike "${AMB[@]}" -dof_spike
# At the FSO default size the debris was hard to find (test: 1 sky pixel changed by >20 levels
# between two frames); 4x size for comparison.
run s5-dust4 16 "$HAZE" "${AO[@]}" "${DOF[@]}" "FSO_DOF_FAR=${FAR:-0.333 1}" FSO_DUST=SpikeDust FSO_DUST_SIZE=4 -- -ssao_spike "${AMB[@]}" -dof_spike
# Single effects on their own, for the cost table and to see each one alone.
run e-ao 16 "${AO[@]}" -- -ssao_spike
run e-amb 16 -- "${AMB[@]}"
run e-aoamb 16 "${AO[@]}" -- -ssao_spike "${AMB[@]}"
run e-far 16 "${DOF[@]}" "FSO_DOF_FAR=${FAR:-0.333 1}" -- -dof_spike
run e-dust 16 FSO_DUST=SpikeDust --
run e-haze-dark 16 "FSO_HAZE=0.02 0.025 0.035 1000 1500" --
