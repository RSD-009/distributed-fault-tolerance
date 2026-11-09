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

## Faults and mechanisms
Faults (what goes wrong):
- **Node faults:** permanent crash, crash + reboot, transient (deaf for a while).
- **Link faults:** message loss, partition (short or long).

Mechanisms (the answer to a fault), each one a switch in omnetpp.ini:
- **Timeout + retransmission** (`maxRetries`, 0 = detection only): loss, short partition, transient.
- **Checkpointing** (`checkpointing`): crash + reboot.
- **Replication** (`replication`, backup links 2-3 and 2-4): permanent crash, long partition.

## Experiments (simulations/omnetpp.ini, named Family_Fault_Mechanism)
| Config | Fault | Mechanism | Informed |
|---|---|---|---|
| Baseline | none | none | 7/7 (12 messages) |
| Node_Crash_Timeout | node 1 crashes at 2ms | detection only | 4/7 |
| Node_Crash_Replication | same | backup links | 6/7 |
| Node_Reboot_Cold | node 1 crashes at 7ms, reboots at 100ms | none | 6/7 |
| Node_Reboot_Checkpoint | same | checkpointing | 7/7 |
| Node_Transient_NoRetry | node 1 deaf 4-70ms | none | 4/7 |
| Node_Transient_Retransmit | same | retransmission | 7/7 |
| Link_Loss_NoRetry | 30% loss | none | below 7/7 |
| Link_Loss_Retransmit | same | retransmission | about 7/7 |
| Link_Partition_NoRetry | link 1-3 cut 4-70ms | none | 6/7 |
| Link_Partition_Retransmit | same | retransmission | 7/7 |
| Link_PartitionLong_Retransmit | link 1-3 cut 500ms | retransmission | 6/7 (false suspicion) |
| Link_PartitionLong_Replication | same | backup links | 7/7 |

Each config has 10 runs (`repeat = 10`). Only the Link_Loss configs are random.

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
