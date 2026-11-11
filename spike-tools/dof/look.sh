#!/bin/bash
# DoF spike: which setting makes the ISD read as most massive (parked stills),
# then 15 s flight videos with the X-wing at 2/3 throttle toward the ISD.
set -uo pipefail
S="$HOME/fso/tc/dof/tools/dof.sh"; P="-900 250 2200 0"
export YAW=-22 PITCH=6 TAIL_SECS=3 FSO_DOF_APERTURE=8 FSO_DOF_MAXBLUR=12
cd "$HOME/fso/tc/dof" || exit 1
gpu() { echo "compute apps: $(nvidia-smi --query-compute-apps=pid,name --format=csv,noheader | tr '\n' ' ')"; }
gpu; FSO_DOF_FOCUS=target FSO_DOF_RANGE=2000 KEYS="2:t" $S k-target-r2000 "$P" 7 -dof_spike; echo "k-target-r2000 $?"
gpu; FSO_DOF_FOCUS=player FSO_DOF_RANGE=15 FSO_DOF_APERTURE=3 $S k-player-a3 "$P" 7 -dof_spike; echo "k-player-a3 $?"
export KEY=bracketright
gpu; $S v-off "$P" 16; echo "v-off $?"
gpu; FSO_DOF_FOCUS=player FSO_DOF_RANGE=15 $S v-player "$P" 16 -dof_spike; echo "v-player $?"
gpu; FSO_DOF_FOCUS=target FSO_DOF_RANGE=2000 KEYS="2:t" $S v-target-r2000 "$P" 16 -dof_spike; echo "v-target-r2000 $?"
gpu; FSO_DOF_FOCUS=target FSO_DOF_RANGE=0 KEYS="2:t" $S v-target "$P" 16 -dof_spike; echo "v-target $?"
gpu
