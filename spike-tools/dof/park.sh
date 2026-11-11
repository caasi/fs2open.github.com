#!/bin/bash
# DoF spike: the parked far-ISD shot (player at rest, chase view), DoF off and on
# with each focus source. Stills at 1 and 4 s compare pixel for pixel.
set -uo pipefail
S="$HOME/fso/tc/dof/tools/dof.sh"; P="-900 250 2200 0"
export YAW=-22 PITCH=6 TAIL_SECS=3 FSO_DOF_RANGE=15 FSO_DOF_APERTURE=8 FSO_DOF_MAXBLUR=12
cd "$HOME/fso/tc/dof" || exit 1
$S k-off "$P" 7; echo "k-off $?"
FSO_DOF_FOCUS=player $S k-player "$P" 7 -dof_spike; echo "k-player $?"
FSO_DOF_FOCUS=target KEYS="2:t" $S k-target "$P" 7 -dof_spike; echo "k-target $?"
FSO_DOF_FOCUS_UV="0.47 0.51" $S k-uv "$P" 7 -dof_spike; echo "k-uv $?"
FSO_DOF_FOCUS=player FSO_DOF_SKY=0 $S k-player-sky0 "$P" 7 -dof_spike; echo "k-player-sky0 $?"
FSO_DOF_FOCUS=player HUD=on $S k-player-hud "$P" 7 -dof_spike; echo "k-player-hud $?"
