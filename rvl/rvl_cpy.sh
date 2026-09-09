#!/usr/bin/env bash
#
# Simple "editor" which just swaps the target file
# for a hacked version.
#  - remove xx.hack
#  - start synthesis
#  - istflow will generate xx.vhd
#  - abort synthesis or let finish
#  - copy xx.vhd to xx.vhd.hack
#  - edit xx.vhd.hack
#  - force restart of synthesis
if [ -n "$1" -a -f "$1.hack" ] ; then
  cp -f "$1.hack" "$1"
fi
