# Project 5 - Fault Tolerance in Distributed Systems: complete project record

Course: Algorithmique Distribuee (Prof. M. R. Senouci, EMP). Team: **Derkaoui** (implementation and fault scenarios), **Mankour** (evaluation and metrics), **Benhalima** (report and presentation).
Tool: OMNeT++ 6.4.0. Repo: `github.com/RSD-009/distributed-fault-tolerance`.

## 0. How to use this document

- **Mankour's assistant:** read sections 1 to 12 to understand what is being measured, then section 15 (your task).
- **Benhalima's report and slides:** sections 2, 3, 5, 6, 8, 11, 13 and 16 are written so they can be adapted into the report.
- **Everyone:** section 12 says what was verified by real runs and what was **not**. Do not claim in the report anything listed as unverified until it has been run.

Status at the time of writing: the code is finished and working on Derkaoui's machine (see section 12 for the exact checks). The branch `fix/random-faults` is committed locally; the first push to GitHub failed with an HTTP 408 timeout and had to be retried. Check section 14 for the repo state before starting.

---

## 1. The assignment

Project 5 from the course list ("Exploring Fault Tolerance Mechanisms in Distributed Systems"):

> Investigate fault tolerance by selecting a distributed algorithm and simulating a system with 5-10 nodes. Introduce fault scenarios such as crash failures, network partitions, or message loss, and implement mechanisms like replication, checkpointing, or timeout-based recovery. Evaluate performance and reliability under these conditions, measuring metrics such as recovery time, message overhead, and availability. Deliverables: a working implementation, a short report (3-5 pages) describing the algorithm, fault types, mitigation mechanisms and evaluation, a presentation (15 minutes + 5 minutes of questions), and a demo (live or recorded).

General rules from the course sheet that apply to every project:

- Start by (a) defining the problem clearly with concrete examples, (b) presenting a brief state of the art with qualitative comparisons (assumptions, complexities), (c) selecting one solution, justifying the choice, and presenting it in detail with a simplified implementation demo.
- Students must not reuse presentations from previous years.
- **Grading:** problem definition and context 10%, state of the art 10%, implementation 25%, performance evaluation 10%, presentation and demo 25%, report quality 20%. Academic integrity is mandatory.
- **LLM rules:** allowed for brainstorming, clarifying concepts, improving language, debugging hints, alternative formulations that students critically evaluate. Not allowed: submitting LLM content as original work, using an LLM to design or implement the full project, relying on it for analysis without understanding. **Any use must be disclosed in the report (methods or appendix), otherwise the grade is 0.**

## 2. What the professor said about this project (translated from the team's notes)

- The course does not treat failures. In this project we do consider failures: either **node failures** or **link failures**. So we must define the different failures.
- Some faults are ones where we do not wait for the node, which later recovers and returns. Some faults are **transient** (temporary). So the report must discuss the **types of faults**.
- The most important part is the different **fault tolerance mechanisms**: replication, timeout-based recovery, checkpointing. There are solutions and there are hypotheses.
- A distributed system based on asynchronous message exchange is, in reality, **semi-synchronous**: we ping a node, and if a certain time passes (example: **50 ms**) without an answer, we assume there is a problem and trigger a fault-tolerance mechanism.

How we used it: timeout = 50 ms is the default everywhere; the "semi-synchronous" model is the explicit assumption behind our failure detector (section 6.1).

## 3. Course background we build on

The course (Chapter 1 and 2) models a distributed algorithm as entities that react to **events** (receiving a message, a timer expiring, a spontaneous event SP) with **actions**, written as rules `state x event -> action`. The course explicitly assumes **total reliability** ("Fiabilite totale (oubliez les pannes)", Chapter 2 broadcasting slides) and finite communication delays only "in the absence of failures" (Axiom 2). This project removes that assumption.

The problem used as the base algorithm is **broadcasting** (diffusion of an information I from one initiator to all nodes). The course presents successive versions:

| Version | Idea | Message cost (course) |
|---|---|---|
| diffuse-v0 | send to all neighbours whenever informed | does not terminate |
| diffuse-v1 | add state `done` | 2m |
| diffuse-v2 | do not send back to the sender | 2m - n + 1 |
| **diffuse-v3** | carry control knowledge Z (nodes already informed or about to be) in the message and send only to Y = N(x) \ Z | fewer messages (needs unique IDs and knowledge of neighbours) |

Lower bound for broadcasting: time Theta(d), messages Theta(m) (m = links, n = nodes, d = diameter).

**diffuse-v3 rules (course):**

1. `initiator x SP -> send(I, N(x)+x) to N(x); become done`
2. `idle x Receiving(I,Z) -> Process(I); become done; if Y = N(x)\Z is not empty: send(I, Y U Z + x) to Y`
3. `initiator x Receiving(I,Z) -> nil`
4. `idle x SP -> nil`
5. `done x Receiving(I,Z) -> nil`
6. `done x SP -> nil`

Other course notions we use: the strategy "Use-a-tree" (broadcast on a spanning tree costs n-1 messages), and Chapter 4 global state / Chandy-Lamport snapshots (the course itself lists "fault tolerance: the global state is saved to serve as a recovery point" as a motivation), which is the conceptual basis of our checkpointing.

## 4. Design decisions and why

| Decision | Reason |
|---|---|
| **One algorithm (diffuse-v3) and a small set of mechanisms** | Every team member can keep the whole design in mind and explain it to the professor. |
| **7 nodes** (project asks 5-10) | Small enough to follow in the simulator, large enough for multi-hop relaying. A Mesh variant has 11 nodes (see 7.3). |
| **Binary tree as the main topology** | On a tree every non-root node has exactly one path to the root, so a crashed relay cuts off its whole subtree: a dramatic, measurable effect (availability). On a full mesh every node is one hop from the initiator and diffuse-v3's savings and the effect of crashes would be invisible. |
| **Ring and Star as extra topologies** | Famous in distributed algorithms (the course's own election project uses ring/star/mesh). They show that the same code works on other topologies, because each node reads its neighbours from the wiring. |
| **Link delay 5 ms** | Fixed on every link. |
| **Timeout 50 ms, 3 retries** | 50 ms is the professor's example. |
| **Everything random, drawn once per run** | Each run number is a different random "story" (initiator, crash victim, times), so results are statistics, not a scripted demo. |
| **Mechanisms are switches, faults are scenarios** | A fault and its mitigation are separate parameters, so each fault can be run with and without its mechanism and compared. |

## 5. Fault model

All faults are injected in the simulation (nothing is real). Assumptions: **crash-stop / fail-silent** nodes (a crashed node stops sending and ignores everything; no Byzantine behaviour), links lose or block messages but never corrupt them, only one fault scenario per run.

| Family | Fault | Duration | How it is injected | Config switch |
|---|---|---|---|---|
| **Node** | Crash | permanent | node schedules its own *crash timer*; when it fires the node stops, cancels its timers and ignores all messages | `crashTime` on the victim |
| **Node** | Crash + reboot | temporary | a second self-timer (*reboot timer*) at `recoverTime`: the node restarts either from its checkpoint or with empty memory | `crashTime`, `recoverTime`, `checkpointing` |
| **Node** | Transient | temporary | node is deaf (ignores incoming messages) between `silentStart` and `silentEnd`, then works again with memory intact | `silentStart`, `silentEnd` (victim only) |
| **Link** | Message loss | random, per message | each send (INFORM, retry, ACK) is dropped with probability `msgLossProb` | `msgLossProb` |
| **Link** | Link down, temporary | window | one link is blocked in both directions in `[partitionStart, partitionEnd)` | `partitionStart/End` |
| **Link** | Link down, permanent | whole run | same, window = whole simulation | `partitionStart = 0s`, `partitionEnd = 5s` |

Terminology: a link that is down for a window is a (simple) **network partition**: the two nodes stay alive but cannot talk.

Important remark for the report: **a timeout detector cannot tell a crash from a link failure.** A node that stays silent is declared "unreachable / presumed crashed" in both cases. This is a *false suspicion* when the node is actually alive (link down) and it is inherent to the semi-synchronous model.

## 6. Mitigation mechanisms

### 6.1 Timeout + retransmission (detection and recovery of loss)
- Every INFORM must be acknowledged by an ACK (the receiver always ACKs, even a duplicate).
- After each send a timer of `timeout` (50 ms) starts. No ACK: resend the same message, up to `maxRetries` (3). After that the neighbour is marked **unreachable** ("presumed crashed").
- **Detection time = (maxRetries + 1) x timeout = 200 ms** (first expiry at 50 ms, then three retries; seen in logs as `UNREACHABLE` at t = 0.2 s).
- Retransmission is safe because INFORM is idempotent: an already-informed node does nothing new.
- `maxRetries = 0` means *detection only* (no recovery); used in the `NoRetry` controls.
- Covers: message loss, temporary link down, transient node fault, and detects crashes. It does **not** recover a permanently crashed node.

### 6.2 Checkpointing (recovery of a rebooting node)
- After every state change the node copies its state (done, information, pending sends, acked neighbours, unreachable neighbours) into a checkpoint. In the simulation the copy stays in memory and is treated as stable storage (real systems would write to disk, at a cost).
- On reboot with `checkpointing = true`: reload the state and resend to every neighbour that never confirmed. Without checkpointing: cold restart, memory lost, `done = false`.
- Metric of its cost: `checkpointsSaved`.
- Covers: crash followed by a reboot. Needs the node to come back.

### 6.3 Replication (prevention, redundant paths)
- `replication = true` adds backup links in the tree: **2-3 and 2-4**, so nodes 3 and 4 have two parents (1 and 2). This is **path redundancy**, not state-machine replication; say so in the report.
- **Known caveat:** diffuse-v3 never sends to a node it believes already covered (Z). Example: if node 1 is the initiator, Z already contains 3 and 4, so node 2 will not use the backup links, and a cut link 1-3 leaves node 3 uninformed. If node 0 is the initiator, node 2 does send to 3 and 4 and the backup works. So replication helps in some runs and not in others. This is a real finding about combining diffuse-v3 with redundancy, not a bug.

### 6.4 Which mechanism answers which fault

| Fault | Mechanism | Expected effect (hypothesis to confirm with data) |
|---|---|---|
| Message loss | timeout + retransmission | availability stays near 100%, cost = extra messages |
| Link down, temporary | timeout + retransmission | recovered once the link is back |
| Link down, permanent | replication | helps only when the backup path is used (see caveat) |
| Transient node fault | timeout + retransmission | recovered after the node wakes up |
| Crash + reboot | checkpointing | informed state restored, resumed sending |
| Crash, permanent | detection only | subtree lost, crash detected after 200 ms |

## 7. Architecture of the code (`simulations/`)

### 7.1 Files

| File | Role |
|---|---|
| `DiffuseMessage.msg` | Message of diffuse-v3, same definition as the lab: `int Z[]; int information;` |
| `Node.ned` | Parameters of a node (see 7.2) |
| `Network.ned` | **Tree** topology and the random draws of the run |
| `Ring.ned`, `Star.ned`, `Mesh.ned` | Same random draws, other topologies |
| `Node.h`, `Node.cc` | The behaviour: part 1 diffuse-v3, part 2 fault tolerance |
| `omnetpp.ini` | Experiments (section 8) |
| `scripts/gate.sh` | Sanity checks before measuring |
| `scripts/analyze.py` | Turns `.sca` files into a table and charts |

(`Ring.ned`, `Star.ned` and `Mesh.ned` were seen only through grep lines; their exact wiring should be read in the files before describing them in the report.)

### 7.2 Parameters of a node (`Node.ned`)

`nodeId`, `isInitiator` and `isCrashTarget` (both set by the network, ONE random node each per run), `timeout` (default 50 ms), `maxRetries` (3), `msgLossProb` (0), `crashTime` (-1 = never; used only by the crash target), `recoverTime` (-1 = never), `checkpointing` (false), `partitionPeer` (set by the network for the two ends of the cut link, -1 = none), `partitionStart`, `partitionEnd`, `silentStart`, `silentEnd`.

### 7.3 Network files: the random scenario of a run

Drawn once per run (so every node agrees), each run number = a different seed:

- `initiatorId = intuniform(0, numNodes-1)`: **random initiator**.
- `crashNodeId = (initiatorId + intuniform(1, numNodes-1)) % numNodes`: **random victim, never the initiator** (a crashed initiator would change nothing, because it sends at t = 0).
- `partitionNodeId`, `partitionParentId`: **random link**. Tree: node 3 or 4 and its parent 1 (the two nodes that have a backup path). Ring: a random node and its predecessor. Star: a random leaf and the hub (node 0).
- The same random victim is used for crash, reboot **and** transient faults (the transient fault only affects the victim).
- Times are random in the ini: `crashTime = uniform(1ms, 20ms)`, `recoverTime = uniform(60ms, 150ms)`, `silentStart = uniform(1ms, 12ms)`, `silentEnd = uniform(40ms, 120ms)`. Every node draws a value, but only the victim uses it.
- Link-down windows are fixed in the ini (both ends of the link must share the same window).

Topologies: **Tree** (node i has children 2i+1 and 2i+2, 7 nodes, 6 links), **Ring** (7 nodes), **Star** (hub 0 and leaves), **Mesh** (11-node backbone). Link delay 5 ms everywhere.

### 7.4 `Node.cc`

**Part 1 - diffuse-v3 (written like the course rules and the lab's Node3):**
- `buildNeighborTable()` fills `N` = neighbours, read from the wiring.
- Rule 1 in `initialize()`: the initiator builds `Z = N(x) + x`, becomes done, sends to every neighbour.
- Rule 2 in `onInformReceived()`: compute `Y = N \ Z`, append Y and x to Z, send to Y, become done. Rules 3 and 5: an already-done node ignores the message (nil).
- Log lines `[rule 1]` and `[rule 2]` show which rule fires.
- The message is a `DiffuseMessage` (name "diffusion v3", kind INFORM); a `senderId` parameter is attached only for the ACK layer.

**Part 2 - fault tolerance (added by us):** `sendInformTo` (send + start timer), `sendAck`, `onRetxTimer` (retransmit or declare unreachable), `onAckReceived`, `saveCheckpoint`, `crashNow`, `recoverNow`, `linkIsCut`, `isSilent` (victim only), loss injection before every send.

**Message kinds:** INFORM (DiffuseMessage), ACK, and three self-timers: RETX_TIMER (retransmission), CRASH_TIMER, RECOVER_TIMER. The crash and reboot timers are **not protocol messages**: they are timers a node sets for itself (`scheduleAt`), the only way an OMNeT++ module can act at a chosen time without receiving anything. In the report call them "fault-injection timers local to the node". Qtenv shows them as `selfmsg`.

### 7.5 Reference timeline (Baseline, tree, initiator = node 0)
t = 0 node 0 sends INFORM to 1 and 2. t = 5 ms nodes 1 and 2 informed, they ACK and forward to 3,4,5,6. t = 10 ms nodes 3 to 6 informed and ACK. t = 15 ms last ACKs. Messages: 6 INFORM + 6 ACK = **12** (the 6 INFORM match the course formula 2m - n + 1 with m = 6, n = 7). With a random initiator the count stays 12 on the tree, but completion time depends on the initiator's position (from 10 ms for the root up to 20 ms for a leaf).

## 8. Experiments (`omnetpp.ini`)

`repeat = 10`: every config is run with seeds 0 to 9. 16 configs, so 160 result files. `sim-time-limit = 5s`, `numNodes = 7` (Mesh: 11).

| Config | Fault | Mechanism | Compare with |
|---|---|---|---|
| `Baseline` | none | none | reference: 7/7 informed, 12 messages |
| `Node_Crash_Timeout` | crash of a random node at a random time | detection only | Baseline |
| `Node_Reboot_Cold` | crash + reboot | none (memory lost) | `Node_Reboot_Checkpoint` |
| `Node_Reboot_Checkpoint` | crash + reboot | checkpointing | `Node_Reboot_Cold` |
| `Node_Transient_Retransmit` | one node deaf for a while | retransmission | `Node_Transient_NoRetry` |
| `Node_Transient_NoRetry` | same | none (`maxRetries = 0`) | `Node_Transient_Retransmit` |
| `Link_MsgLoss_Retransmit` | 30% message loss | retransmission | Baseline |
| `Link_Down_Temporary` | random link down 4 to 70 ms | retransmission | `Link_Down_Retransmit` |
| `Link_Down_Retransmit` | random link down for the whole run | retransmission only | `Link_Down_Replication` |
| `Link_Down_Replication` | same | + backup links | `Link_Down_Retransmit` |
| `Ring_Baseline`, `Ring_Node_Crash_Timeout` | none / crash | - | tree results |
| `Star_Baseline`, `Star_Node_Crash_Timeout` | none / crash | - | tree results |
| `Mesh_Baseline`, `Mesh_Node_Crash_Timeout` | none / crash (11 nodes) | - | tree results |

Missing controls worth knowing (not implemented): there is no `Link_MsgLoss_NoRetry` and no no-fault replication config, so the benefit of retransmission under loss and the pure cost of the backup links cannot be measured directly. They can be added in the ini in a few lines if the team wants them.

## 9. Metrics

Written by `finish()` of every node into the `.sca` files (one value per node):

`informed` (1 if the node is done), `informedAt` (time it became informed, -1 if never), `messagesSent`, `messagesDropped`, `messagesCut` (blocked by a down link), `messagesIgnored` (ignored while deaf), `checkpointsSaved`, `unreachableNeighbors`, `crashed`, `wasInitiator`, `wasCrashTarget`.

Derived (by `analyze.py`): **availability** = informed nodes / number of nodes, in %; **message overhead** = messages sent compared with the Baseline of the same topology; **completion time** = latest `informedAt`; **recovery time** = completion time minus the baseline completion time (only meaningful where nodes are informed late: transient, link down, loss); **detection time** = (maxRetries + 1) x timeout = 200 ms (by formula; visible in the log, not a recorded scalar); **cost of checkpointing** = `checkpointsSaved`.

Note: message counts only count messages actually sent (dropped ones are in `messagesDropped`).

## 10. How to run

**IDE:** open the project in the OMNeT++ IDE, F5 (refresh), Ctrl+B (build). Run `simulations/omnetpp.ini` with Qtenv, pick the config and the **Run #** (0 to 9) in the dropdown. Nodes turn green when informed, red when crashed, gold after a cold reboot.

**Terminal** (from `simulations/`; the program is in the repo root):

```bash
../distributed-fault-tolerance -u Cmdenv -n .. -c Node_Crash_Timeout -r 0 --cmdenv-express-mode=false omnetpp.ini | grep -E "rule|CRASHED|RETRANSMIT|INFORMED|UNREACHABLE"
```

Without `-r`, all 10 runs are played. Add `--cmdenv-log-level=debug` to see the "nil" rules.

**Sanity gate** (once, before measuring): `bash scripts/gate.sh`. Checks: Baseline informs 7 nodes with exactly 12 messages in every run; exactly one crash per run and never the initiator; at most one deaf node in the transient config; crash times differ between runs. Must print `ALL CHECKS PASSED`.

**Measurement campaign:**

```bash
cd ~/distributed-fault-tolerance/simulations
rm -f ../results/*.sca ../results/*.vec ../results/*.vci
R=$(grep -m1 '^repeat' omnetpp.ini | sed 's/[^0-9]//g')
for c in $(grep -o '^\[Config [A-Za-z0-9_]*' omnetpp.ini | sed 's/\[Config //'); do
  echo "running $c"
  ../distributed-fault-tolerance -u Cmdenv -n .. -c $c -r 0..$((R-1)) --result-dir=../results --cmdenv-express-mode=true omnetpp.ini > /dev/null
done
ls ../results/*.sca | wc -l          # must be (number of configs) x 10 = 160
cd .. && python3 scripts/analyze.py   # needs: pip install pandas matplotlib
```

Outputs: `results/summary.csv`, `results/runs.csv`, three charts in `report/figures/` (availability, messages, completion time). The `.sca` files are ignored by git on purpose. The OMNeT++ IDE Analysis tool (double-click a `.sca`, Browse Data, Scalars) can also be used to look at values and plot by hand.

## 11. History: what was found and fixed (useful for the report's "implementation" and "lessons")

1. The initial guides said node 3 is an internal relay. In the 7-node binary tree **node 3 is a leaf**, so crashing it stranded nothing.
2. The crash was checked only when a message arrived, so a node receiving nothing never crashed. Fix: the node schedules its own crash timer.
3. **Timeout unit bug:** `timeout` was declared `@unit(ms)`, so `50ms` reached C++ as `50` and was read as 50 seconds; retransmission never fired and no node was ever declared unreachable. Fix: `@unit(s)`.
4. MsgLoss looked identical to Baseline because the ini defined a single run (one fixed seed). Fix: `repeat = 10`. (The loss code itself was proven correct with a forced 100% loss test.)
5. Lost retransmissions and lost ACKs were silent; now every loss is logged.
6. Added link down (partition), the transient fault, checkpointing and replication.
7. Reorganised experiments into a **fault x mechanism matrix** (names `Family_Fault_Mechanism`), so mechanisms are switches and not scenarios; removed the meaningless "crash + replication" combination.
8. Randomised the scenario: initiator, crash victim and cut link per run; added Ring, Star, Mesh.
9. Rewrote the diffuse part in the **course style** (DiffuseMessage with Z and information, rules 1 to 6, `[rule n]` logs) without changing the fault logic.
10. Renamed `CRASH_EVENT` to `CRASH_TIMER` (it is a self-timer, not a message from the network).
11. Transient fault restricted to the single random victim (before, every node went deaf, which is an outage, not a node fault).
12. Crash victim can no longer be the initiator; crash, reboot and silence times are random.

## 12. Validation status (be honest in the report)

**Verified by real runs (output seen):** Baseline informs all nodes with 12 messages (before the random-scenario and diffuse changes); the crash is detected (retransmissions at 50, 100, 150 ms, `UNREACHABLE` at 200 ms); the loss code works (forced 100% loss); an earlier fixed-link partition behaved as predicted (short cut recovered by retry 2/3, long cut ended in `UNREACHABLE`); `isSilent()` and the crash-victim fix are in the source.

**Not verified at the time of writing (check before claiming):**
- the diffuse rewrite was meant to be validated by comparing results before/after on identical seeds; that comparison was not reported;
- `scripts/gate.sh` result on the real build;
- results of the reboot/checkpoint, replication, link-down, Ring, Star and Mesh configs;
- the 16-config campaign and `analyze.py` on real `.sca` files (the script was tested only on synthetic data);
- the real build after the `CRASH_TIMER` rename and the transient fix (a stale binary was in use during one check).

## 13. Limitations and threats to validity (state them in the report)

- A timeout detector cannot distinguish a crash from a link failure; **false suspicions** are possible (a healthy node declared "presumed crashed").
- The model is semi-synchronous: correctness of detection assumes delays below the timeout.
- Checkpoints are an in-memory copy treated as stable storage.
- "Replication" is path redundancy over backup links and only exists in the tree topology; interaction with Z-suppression can leave the backup unused.
- Crash-stop only; no Byzantine faults; one fault scenario per run; one initiator.
- Only 10 runs per config, so differences between close results are not significant; report mean and standard deviation.
- Deterministic configs (Baseline without randomness) give identical runs; only the random choices vary.
- During a transient fault the deaf node's own retry timers keep running (only incoming messages are ignored).

## 14. Team, tasks, git state

| Person | Task |
|---|---|
| Derkaoui | Implementation and fault scenarios; explains `Node.ned`, `Node.h`, `Node.cc` in the demo |
| Mankour | Run the campaign, tables and charts, write "Performance Evaluation" |
| Benhalima | Report (problem definition, state of the art, assembly) and the presentation, demo script, LLM disclosure |

Git rules: never commit to `main`; each person works on a feature branch and merges with a Pull Request. Derkaoui's work is on branch **`fix/random-faults`** (the first push failed with HTTP 408; verify with `git ls-remote --heads origin` that the branch exists on GitHub). `origin/main` had a newer commit than the branch at the time of the last fetch, so check `git log origin/main` and merge main into the branch if needed. Deadline: Thursday; all deliverables must be submitted before PC access is lost.

## 15. Mankour: what to do

1. Pull the branch, rebuild in the IDE (F5, Ctrl+B).
2. Run the measurement campaign (section 10) and check the file count is 160.
3. `python3 scripts/analyze.py`; check `summary.csv` has 16 rows.
4. Compare each fault with and without its mechanism: `Node_Reboot_Cold` vs `Node_Reboot_Checkpoint`; `Node_Transient_NoRetry` vs `Node_Transient_Retransmit`; `Link_Down_Retransmit` vs `Link_Down_Replication`; Baseline vs each fault; tree vs Ring/Star/Mesh for Baseline and crash.
5. Use the **mean and standard deviation over 10 runs**; each run is a different random story.
6. Write only what the numbers show. If a mechanism does not help in some runs, say so and explain (see the Z-suppression caveat in 6.3).
7. Deliver to Benhalima: the 3 charts, `summary.csv`, and `report/evaluation.md` (about one page: setup, table, figures, one paragraph per fault family, trade-offs, limits).
8. Push to your own branch `feature/evaluation` and open a Pull Request.
9. Disclose any LLM help honestly to Benhalima for the report.

## 16. Benhalima: report skeleton (3-5 pages) mapped to the grading

| Part | Grade weight | Content | Source in this document |
|---|---|---|---|
| Problem definition and context | 10% | total-reliability assumption of the course; node vs link faults; permanent vs transient; concrete examples; semi-synchronous model | sections 2, 3, 5 |
| State of the art | 10% | compare replication, checkpointing, timeout-based recovery (assumptions, cost, faults covered, limits); justify the choice | sections 6, 13; add real references after reading them (e.g. failure detectors, Chandy-Lamport from the course, primary-backup replication) and **verify every citation** |
| Algorithm and mechanisms | part of report quality | diffuse-v3 rules, the ACK/timeout layer, checkpoint, backup links | sections 3, 6, 7 |
| Performance evaluation | 10% | Mankour's section | section 15 |
| Conclusion and limits | report quality | what each mechanism fixes and where it fails; false suspicions | sections 6.4, 13 |
| LLM disclosure | mandatory | see below | section 17 |

Presentation (15 min + 5 min questions): problem and assumptions, state of the art comparison, why diffuse-v3, the rule table, the fault matrix, **live or recorded demo** (a good sequence: `Baseline`, then `Node_Crash_Timeout` showing the random victim turning red, the retransmissions at 50/100/150 ms and `UNREACHABLE` at 200 ms, then `Node_Reboot_Cold` vs `Node_Reboot_Checkpoint`, then Ring and Star as proof of generality), the charts, limits, conclusion. Record the demo as a backup.

## 17. LLM usage (must be disclosed)

Claude (Anthropic) was used during this project for brainstorming the design, finding bugs (crash on a leaf, timeout unit, single run), writing and restructuring parts of the code, scripts and documentation, and explanations. The course rules say LLM-generated work must not be submitted as original and the team must understand everything. Therefore: every member must be able to explain every file; Derkaoui is preparing a study programme to learn the code line by line; the report's methods or appendix must contain an **honest** disclosure written by the team describing what was actually used and for what. Do not copy generated text into the report without rewriting and understanding it.

## 18. Questions the professor may ask (short answers)

- *Why a tree?* One path per node makes the impact of a relay crash visible; a full mesh would hide it. Ring and Star show the code is topology-independent.
- *Why is retransmission safe?* INFORM is idempotent: an already-informed node does nothing new and only re-ACKs.
- *Can your detector tell a crash from a partition?* No. After 200 ms a silent neighbour is only "presumed crashed". This is the price of the semi-synchronous model.
- *What does checkpointing cost and when does it not help?* One state copy per state change (`checkpointsSaved`); useless if the node never reboots.
- *Why does replication not always help?* diffuse-v3 suppresses sends to nodes already in Z, so the backup path may not be used.
- *Why do the crash victim and times change every run?* Random draws once per run with different seeds; results are averaged over 10 runs.
- *Why can the crash not hit the initiator?* The initiator sends at t = 0, so crashing it later changes nothing.
- *What is the 12 in the Baseline?* 6 INFORM (course formula 2m - n + 1) + 6 ACK.
- *Where does the crash come from?* A timer the victim sets for itself; the simulator has no other way for a module to act at a chosen time.

## 19. Glossary

N(x) neighbours of x. Z control knowledge carried by the message (nodes informed or about to be). Y = N(x) \ Z neighbours still to inform. m links, n nodes, d diameter. Initiator: the node that starts the broadcast. Victim: the node chosen for a node fault. Semi-synchronous: asynchronous messages, but a bound (here 50 ms) is used to suspect failures. `.sca`: OMNeT++ scalar result file. Run: one execution with one seed (run number).
