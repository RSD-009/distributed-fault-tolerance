# Distributed Fault Tolerance - Project 5

Fault tolerance on the **diffuse-v3** broadcast algorithm (course Chapter 2),
simulated in OMNeT++ on a 7-node binary tree. The course assumes total
reliability; this project removes that assumption.

## Mechanism: ACK + timeout retransmission
- Every INFORM sent to a neighbour must be acknowledged (ACK).
- No ACK within 50ms (semi-synchronous model): resend the same message.
- After 3 failed retries: mark the neighbour UNREACHABLE (presumed crashed).
- One idea handles message loss, transient faults, and crash detection.

## Setup
Requires OMNeT++ 6.4.0. Import the repo in the OMNeT++ IDE (File > New >
OMNeT++ Project, default location), then press Ctrl+B to build.

## Scenarios (all in simulations/omnetpp.ini)
| Config | Fault | Expected result |
|---|---|---|
| Baseline | none | 7/7 informed, 12 messages (6 INFORM + 6 ACK) |
| Crash | relay node 1 crashes at 2ms (permanent) | 4/7 informed, node 0 reports node 1 UNREACHABLE |
| MsgLoss | each message lost with probability 0.3 | normally 7/7, extra retransmissions |
| CrashAndLoss | crash + 20% loss | at most 4/7 |
| Partition | link 1-3 cut from 4ms to 70ms, then heals | 7/7, node 3 informed late |
| PartitionLong | link 1-3 cut for 500ms | 6/7, node 1 wrongly suspects node 3 |
| Transient | node 1 deaf from 4ms to 70ms, then recovers | 7/7, node 1 informed at about 105ms |

Each config has 10 runs (`repeat = 10`), each with a different random seed.
Only MsgLoss and CrashAndLoss are random; the other configs give identical runs.

## Run from the terminal
    cd simulations
    ../distributed-fault-tolerance -u Cmdenv -n .. -c Crash -r 0 --cmdenv-express-mode=false omnetpp.ini

## Metrics (written by finish() in .sca files)
informed, messagesSent, messagesDropped, messagesCut, messagesIgnored,
unreachableNeighbors, crashed.

## Team
- Derkaoui - implementation and fault scenarios
- Mankour - evaluation and metrics
- Benhalima - report and presentation
