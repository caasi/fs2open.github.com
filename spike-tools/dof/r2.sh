#!/bin/bash
# DoF spike round 2: the flagship shot (focus on the player X-wing, ISD blurred), round 1 against
# round 2 (near/far split, Karis weight), plus the dot-pattern alternatives. Lossless stills at
# 4 and 10 s (<name>-s4.png, <name>-s10.png) for pixel comparisons.
# Usage: r2.sh [run-name ...]   (no names = all runs)
set -uo pipefail
S="$HOME/fso/tc/dof/tools/dof.sh"; P="-900 250 2200 0"
export YAW=-22 PITCH=6 TAIL_SECS=3 KEY=bracketright SNAPS="4 10"
export FSO_DOF_FOCUS=player FSO_DOF_RANGE=15 FSO_DOF_APERTURE=16 FSO_DOF_MAXBLUR=24
cd "$HOME/fso/tc/dof" || exit 1
run() {  # run <name> <secs> [VAR=value ...] -- extra fso args
  local name=$1 secs=$2; shift 2
  if [ ${#only[@]} -gt 0 ] && [[ " ${only[*]} " != *" $name "* ]]; then return; fi
  local envs=()
  while [ $# -gt 0 ] && [ "$1" != -- ]; do envs+=("$1"); shift; done
  [ $# -gt 0 ] && shift
  env "${envs[@]}" "$S" "$name" "$P" "$secs" "$@"; echo "$name $?"
}
only=("$@")
run v2-off 16 --
run v2-r1 16 -- -dof_spike
run v2-fix 16 FSO_DOF_SPLIT=1 FSO_DOF_SNAP=1 -- -dof_spike
run v2-fix-sky0 16 FSO_DOF_SPLIT=1 FSO_DOF_SNAP=1 FSO_DOF_SKY=0 -- -dof_spike
run v2-split 12 FSO_DOF_SPLIT=1 -- -dof_spike
run v2-r1-karis 12 FSO_DOF_KARIS=1 -- -dof_spike
run v2-r1-s256 12 FSO_DOF_SAMPLES=256 -- -dof_spike
run v2-r1-lod1 12 FSO_DOF_LOD=1 -- -dof_spike
run v2-split-s256 12 FSO_DOF_SPLIT=1 FSO_DOF_SAMPLES=256 -- -dof_spike
run v2-split-lod1 12 FSO_DOF_SPLIT=1 FSO_DOF_LOD=1 -- -dof_spike
run v2-split-dbg2 12 FSO_DOF_SPLIT=1 FSO_DOF_DEBUG=2 -- -dof_spike
run v2-dbg1 12 FSO_DOF_DEBUG=1 -- -dof_spike
run v2-r1-snap 12 FSO_DOF_SNAP=1 -- -dof_spike
run v2-split-snap 12 FSO_DOF_SPLIT=1 FSO_DOF_SNAP=1 -- -dof_spike
run v2-fix-s128 12 FSO_DOF_SPLIT=1 FSO_DOF_SNAP=1 FSO_DOF_SAMPLES=128 -- -dof_spike
run v2-fix-karis 12 FSO_DOF_SPLIT=1 FSO_DOF_SNAP=1 FSO_DOF_KARIS=1 -- -dof_spike
# Focus on the ISD (target), the X-wing near and blurred: does the near blur spread over the sharp ISD?
run v2-t-r1 12 FSO_DOF_FOCUS=target KEYS=2:t -- -dof_spike
run v2-t-fix 12 FSO_DOF_FOCUS=target KEYS=2:t FSO_DOF_SPLIT=1 FSO_DOF_SNAP=1 -- -dof_spike
run v2-t-fix-karis 12 FSO_DOF_FOCUS=target KEYS=2:t FSO_DOF_SPLIT=1 FSO_DOF_SNAP=1 FSO_DOF_KARIS=1 -- -dof_spike
run v2-t-fix-dbg2 12 FSO_DOF_FOCUS=target KEYS=2:t FSO_DOF_SPLIT=1 FSO_DOF_SNAP=1 FSO_DOF_DEBUG=2 -- -dof_spike
run v2-t-bbox 12 FSO_DOF_FOCUS=target KEYS=2:t FSO_DOF_SPLIT=1 FSO_DOF_SNAP=1 FSO_DOF_BBOX=1 -- -dof_spike
# Q3: lasers (FotG: left mouse button = fire primary) and a missile (Tab = fire secondary), ISD targeted.
fire="2:t 3:click 4:click 5:Tab 6:click 7:click 8:click 9:click 10:click"
run v2-laser-off 12 KEYS="$fire" SNAPS="4 7 9" --
run v2-laser-fix 12 KEYS="$fire" SNAPS="4 7 9" FSO_DOF_SPLIT=1 FSO_DOF_SNAP=1 -- -dof_spike
run v2-laser-dbg1 12 KEYS="$fire" SNAPS="4 7 9" FSO_DOF_DEBUG=1 -- -dof_spike
