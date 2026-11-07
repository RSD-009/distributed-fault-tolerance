# distributed-fault-tolerance

# Distributed Fault Tolerance — Project 5

Simulating fault tolerance mechanisms (heartbeat detection, leader
re-election, checkpointing) in a 5–10 node distributed system using
OMNeT++, for the Algorithmique Distribuée course.


## Setup
Requires OMNeT++ 6.4.0 installed locally (all teammates use the same version).
Clone the repo, then import it into the OMNeT++ IDE as an existing project.

## Running scenarios
| Config (`.ini`)          | Fault scenario              |
|---------------------------|------------------------------|
| `baseline.ini`             | No faults (sanity check)     |
| `crash.ini`                | Node crash + re-election     |
| `partition.ini`            | Network partition            |
| `msgloss.ini`              | Random message loss          |

## Team
- [Name] — topology & infrastructure
- [Name] — algorithm logic (heartbeat, election, checkpointing)
- [Name] — fault injection & metrics