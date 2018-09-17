#!/bin/sh

RUNCOUNT=0
ERRCOUNT=0
RUNLIMIT=$1

TEST='rm -rf ../build && ./test-helloalivtpnlb-ase.sh'

if [ -z "$1" ]; then
   echo "USAGE: $0 <number of iterations>"
   exit -1
fi

while [ $RUNCOUNT -lt $RUNLIMIT ]; do
   echo -n "---- Test run $RUNCOUNT/$RUNLIMIT ----> "
#   $TEST
   rm -rf ../build && ./test-helloalivtpnlb-ase.sh &> iteration.${RUNCOUNT}.log
   if [ $? -eq 0 ]; then
      echo "success."
   else
      echo "failure."
      echo "Killing simulation."
      sleep 2
      killall ase_simv
      ERRCOUNT=$(( $ERRCOUNT + 1 ))
   fi
   RUNCOUNT=$(( $RUNCOUNT + 1 ))
done

echo "===================================================================="
echo "Summary: $ERRCOUNT of $RUNCOUNT tests failed."

exit $ERRCOUNT

