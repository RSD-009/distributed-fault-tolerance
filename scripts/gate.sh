#!/bin/bash
# gate.sh - sanity checks to run BEFORE the measurement campaign.
# Usage:  bash scripts/gate.sh        (from the repo root)
cd "$(dirname "$0")/../simulations" || exit 1
EXE=../distributed-fault-tolerance
R=$(grep -m1 '^repeat' omnetpp.ini | sed 's/[^0-9]//g')
R=${R:-1}
fail=0
echo "runs per config (repeat): $R"

run()  { rm -f /tmp/g.sca; $EXE -u Cmdenv -n .. -c "$1" -r "$2" --output-scalar-file=/tmp/g.sca omnetpp.ini > /dev/null 2>&1 \
           || { echo "FAIL: $1 run $2 did not run"; fail=1; }; }
sum()  { awk -v n="$1" '$1=="scalar" && $3==n {s+=$4} END{print s+0}' /tmp/g.sca; }
cnt()  { awk -v n="$1" '$1=="scalar" && $3==n && $4>0 {c++} END{print c+0}' /tmp/g.sca; }
both() { awk '$1=="scalar"{v[$2" "$3]=$4; m[$2]=1} END{b=0; for(k in m) if(v[k" wasInitiator"]==1 && v[k" wasCrashTarget"]==1) b++; print b+0}' /tmp/g.sca; }

for r in $(seq 0 $((R-1))); do
  # 1. Baseline (tree): every node informed, exactly 12 messages (6 INFORM + 6 ACK)
  run Baseline $r
  inf=$(sum informed); msg=$(sum messagesSent)
  if [ "$inf" != "7" ] || [ "$msg" != "12" ]; then echo "FAIL Baseline run $r: informed=$inf messages=$msg (expected 7 and 12)"; fail=1; fi

  # 2. Crash: exactly one node crashes, and it is never the initiator
  run Node_Crash_Timeout $r
  if [ "$(sum crashed)" != "1" ]; then echo "FAIL Node_Crash_Timeout run $r: crashed nodes = $(sum crashed) (expected 1)"; fail=1; fi
  if [ "$(both)" != "0" ]; then echo "FAIL Node_Crash_Timeout run $r: the initiator is also the crash target"; fail=1; fi

  # 3. Transient: at most ONE node ever ignores messages
  run Node_Transient_Retransmit $r
  if [ "$(cnt messagesIgnored)" -gt 1 ]; then echo "FAIL Node_Transient_Retransmit run $r: $(cnt messagesIgnored) nodes were deaf (expected at most 1)"; fail=1; fi
  if [ "$(both)" != "0" ]; then echo "FAIL Node_Transient_Retransmit run $r: the initiator is also the victim"; fail=1; fi
done

# 4. The crash time must differ from run to run
times=$(for r in $(seq 0 $((R-1))); do
  $EXE -u Cmdenv -n .. -c Node_Crash_Timeout -r $r --cmdenv-express-mode=false omnetpp.ini 2>/dev/null | grep "CRASHED at"
done | sed 's/.*t=//' | sort -u | wc -l)
echo "distinct crash times over $R runs: $times"
if [ "$R" -ge 3 ] && [ "$times" -lt 3 ]; then echo "FAIL: crash time is not random (only $times distinct values)"; fail=1; fi

echo
if [ $fail -eq 0 ]; then echo "ALL CHECKS PASSED - ready for the measurement campaign"; else echo "SOME CHECKS FAILED - do NOT start the measurements"; fi
exit $fail
