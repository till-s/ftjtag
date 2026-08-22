#!/usr/bin/env bash
#
# First run the original program; this modifies the
# design's top-level vhdl or verilog inserting the debug
# cores and the jtag adapter which we intende to change
# with this script.
# Unfortunately the name of the file being modified is not
# passed on the command line. We have to rely on the
# user to set an environment variable.
#
# NOTES: it is STRONGLY recommended that you set 
#        KEEP_REVEAL_TEMP in the environment. This (natively supported)
#        variable leaves the auto-generated files in place which
#        can be very helpful. Note that even with this variable set
#        the critical top-level file is recreated *every time* synthesis
#        is run; so you can't edit that file manually. This is where
#        this script is usefule...
#
#        export KEEP_REVEAL_TEMP=1
#
#        For this script to work
#          1. the basereveal.patch must be applied to the radiant tool
#          2. the 'istflow.sh' and 'rvl_edit.py' must be installed
#             (with executable permissions) in the directory where
#             'istflow' is found (usually bin/lin64)
#
istflow $*

# Set some variables; user may override from environment
if [ -z ""${LOGF}"" ] ; then
  LOGF="/tmp/rvl_edit.log"
fi
if [ -z "${REVEAL_WORKDIR}" ] ; then
  # NOTE: needs trailing '/'
  REVEAL_WORKDIR=reveal_workspace/tmpreveal/
fi

# must supply $* from top when calling!
print_header () {
  echo "istflow command line args:"  >  "${LOGF}"
  echo $*                            >> "${LOGF}"
  echo "istflow environment:"        >> "${LOGF}"
  printenv                           >> "${LOGF}"
  echo "istflow current dir:"        >> "${LOGF}"
  pwd                                >> "${LOGF}"
}

# a development hack: if we find any '.hck' file then
# we use that (manual editing)
shopt -s nullglob

# check if there would be work

# if a 'skip_rvl_edit' file is found then we are done.
# User may create this file to skip the editing step
# (with radiant still running, REVEAL_TOP set).
if [ -f "${REVEAL_WORKDIR}skip_rvl_edit" ] ; then
  print_header $*
  echo "${REVEAL_WORKDIR}skip_rvl_edit found; SKIP EDITING" >> "${LOGF}"
  exit 0
fi
	
# a development hack: if we find any '.hck' file then
# we use that (manual editing)
HACK="NO"
for f in ${REVEAL_WORKDIR}*.hck; do
  HACK="YES"
done

if [ "${HACK}" = "YES" ]; then
  print_header $*
  for f in ${REVEAL_WORKDIR}*.hck; do
    echo "USING HACK $f" >> "${LOGF}"
    cp "$f" `dirname "$f"`/`basename "$f" .hck` >> "${LOGF}" 2>&1
  done
  exit 0
fi

# if the REVEAL_TOP variable (defines the file
# we are supposed to operate on) is not set then
# we are done.
if [ -z "${REVEAL_TOP}" ]; then
  if [ -e "/tmp/rvl_edit_force_log" ] ; then
    # may force messages for debugging
    print_header $*
    echo "${REVEAL_TOP} not set; skip editing" >> "${LOGF}"
  fi
  # go silently
  exit 0
fi

print_header $*

# use a short variable name
TGT="${REVEAL_WORKDIR}${REVEAL_TOP}"
if [ ! -f "${TGT}" ] ; then
  echo "ERROR: ${TGT} not found"   >> "${LOGF}"
  exit 1
fi

# make a copy
cp -f "${TGT}" "${TGT}.orig"       >> "${LOGF}" 2>&1

# and finally run the editor
if [ -z "${REVEAL_EDITOR}" ] ; then
  REVEAL_EDITOR="$(dirname "${BASH_SOURCE[0]}")/rvl_edit.py"
fi
if [ ! -x "${REVEAL_EDITOR}" ] ; then
  echo "ERROR: ${REVEAL_EDITOR} not found!" >> "${LOGF}"
  exit 1
fi
"${REVEAL_EDITOR}" "${TGT}"  >> "${LOGF}" 2>&1
