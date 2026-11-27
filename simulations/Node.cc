#include "Node.h"
#include <algorithm>

Define_Module(Node);

static const int INFORMATION = 2026;   // the information I broadcast by the initiator

// =====================================================================
//  PART 1 - diffuse-v3 (course Chapter 2, same rules as the lab Node3)
//  States: initiator / idle / done        Events: SP, Receiving(I,Z)
//   1) initiator x SP           -> send(I, N(x)+x) to N(x); become done
//   2) idle x Receiving(I,Z)    -> Process(I); become done;
//                                  if Y = N(x)\Z is not empty: send(I, Y U Z + x) to Y
//   3) initiator x Receiving    -> nil
//   4) idle x SP                -> nil
//   5) done x Receiving         -> nil
//   6) done x SP                -> nil
// =====================================================================

void Node::buildNeighborTable()
{
    // N(x): read off the wiring, so it works on any topology
    for (int i = 0; i < gateSize("out"); i++) {
        cGate *remote = gate("out", i)->getNextGate();
        int neighborId = remote->getOwnerModule()->par("nodeId");
        N.push_back(neighborId);
        gateForNeighbor[neighborId] = i;
    }
}

// build a diffuse-v3 message (I, Z) from a set Z
DiffuseMessage *Node::makeDiffuse(const std::set<int> &z)
{
    DiffuseMessage *msg = new DiffuseMessage("diffusion v3", INFORM);
    msg->setInformation(information);
    msg->setZArraySize(z.size());
    size_t k = 0;
    for (int v : z)
        msg->setZ(k++, v);
    msg->addPar("senderId") = nodeId;   // only used by the ACK layer (ours)
    return msg;
}

// read the content of Z from a message
std::set<int> Node::zOf(DiffuseMessage *msg)
{
    std::set<int> z;
    for (size_t j = 0; j < msg->getZArraySize(); j++)
        z.insert(msg->getZ(j));
    return z;
}

void Node::initialize()
{
    nodeId = par("nodeId");
    isInitiator = par("isInitiator");
    isCrashTarget = par("isCrashTarget");
    timeoutValue = par("timeout");
    maxRetries = par("maxRetries");
    msgLossProb = par("msgLossProb");
    crashTime = par("crashTime");

    partitionPeer = par("partitionPeer");
    partitionStart = par("partitionStart");
    partitionEnd = par("partitionEnd");

    silentStart = par("silentStart");
    silentEnd = par("silentEnd");

    recoverTime = par("recoverTime");
    checkpointing = par("checkpointing");
    useZ = par("useZ");

    buildNeighborTable();

    if (isCrashTarget && crashTime >= SIMTIME_ZERO) {
        crashTimer = new cMessage("crash timer", CRASH_TIMER);
        scheduleAt(crashTime, crashTimer);
    }
    if (isCrashTarget && crashTime >= SIMTIME_ZERO && recoverTime > crashTime) {
        recoverTimer = new cMessage("reboot timer", RECOVER_TIMER);
        scheduleAt(recoverTime, recoverTimer);
    }

    if (isInitiator) {
        // rule 1: initiator x SP -> send(I, N(x)+x) to N(x); become done
        information = INFORMATION;
        DiffuseMessage *msg = new DiffuseMessage("diffusion v3", INFORM);
        msg->setInformation(information);
        msg->setZArraySize(N.size());
        for (size_t i = 0; i < N.size(); i++)
            msg->setZ(i, N[i]);              // Z = N(x)
        msg->appendZ(nodeId);                // ... + x
        std::set<int> z = zOf(msg);
        delete msg;

        done = true;
        informedAt = simTime().dbl();
        getDisplayString().setTagArg("i", 1, "green");
        getDisplayString().setTagArg("i", 2, "60");
        EV_INFO << "Node " << nodeId << ": [rule 1] initiator x SP -> send(I, N(x)+x) to N(x); become done\n";
        for (int n : N)
            sendInformTo(n, z);
        saveCheckpoint();
        EV_INFO << "Node " << nodeId << ": INITIATOR, diffusion started.\n";
    }
}

void Node::onInformReceived(DiffuseMessage *msg)
{
    int senderId = (int)msg->par("senderId").longValue();

    // fault tolerance (ours): always ACK, even for a message already seen
    sendAck(senderId);

    // rules 3 and 5: initiator / done x Receiving(I,Z) -> nil
    if (done) {
        EV_DEBUG << "Node " << nodeId << ": [rule 3/5] done x Receiving(I,Z) -> nil\n";
        return;
    }

    // rule 2: idle x Receiving(I,Z) -> Process(I); become done; send(I, Y U Z + x) to Y
    done = true;
    informedAt = simTime().dbl();
    getDisplayString().setTagArg("i", 1, "green");
    getDisplayString().setTagArg("i", 2, "60");
    EV_INFO << "Node " << nodeId << ": [rule 2] idle x Receiving(I,Z) -> become done\n";
    EV_INFO << "Node " << nodeId << ": INFORMED (via " << senderId << ")\n";

    information = msg->getInformation();      // Process(I)

    // Y = N(x) \ Z : the neighbours that are not in Z
    std::vector<int> Y;
    for (size_t i = 0; i < N.size(); i++) {
        bool existe = false;
        for (size_t j = 0; j < msg->getZArraySize() && !existe; j++) {
            if (msg->getZ(j) == N[i])
                existe = true;
        }
        if (useZ ? !existe : N[i] != senderId)
            Y.push_back(N[i]);
    }

    // Z + Y
    for (size_t k = 0; k < Y.size(); k++)
        msg->appendZ(Y[k]);

    // + x
    bool xExiste = false;
    for (size_t j = 0; j < msg->getZArraySize(); j++) {
        if (msg->getZ(j) == nodeId) {
            xExiste = true;
            break;
        }
    }
    if (!xExiste)
        msg->appendZ(nodeId);

    // send(I, Y U Z + x) to Y
    if (!Y.empty()) {
        std::set<int> newZ = zOf(msg);
        for (size_t k = 0; k < Y.size(); k++)
            sendInformTo(Y[k], newZ);
    }
    saveCheckpoint();
}

// =====================================================================
//  PART 2 - FAULT TOLERANCE (added by us, not in the course)
//  ACK + timeout retransmission, crash, reboot, checkpoint,
//  link down (partition), transient fault, message loss.
//  Nothing below depends on how the diffuse rules are written.
// =====================================================================

void Node::saveCheckpoint()
{
    if (!checkpointing) return;
    ckDone = done;
    ckInformation = information;
    ckPendingZ = pendingZ;
    ckAcked = ackedNeighbors;
    ckUnreachable = unreachableNeighbors;
    checkpointsSaved++;
}

void Node::crashNow()
{
    crashed = true;
    crashedAt = simTime().dbl();
    getDisplayString().setTagArg("i", 1, "red");
    getDisplayString().setTagArg("i", 2, "80");
    bubble("CRASHED!");
    EV_INFO << "Node " << nodeId << ": CRASHED at t=" << simTime() << "\n";
    for (auto &kv : retxTimer)
        cancelEvent(kv.second);
}

void Node::recoverNow()
{
    crashed = false;
    recoveredAt = simTime().dbl();
    bubble("RECOVERED");
    EV_INFO << "Node " << nodeId << ": RECOVERED at t=" << simTime() << "\n";

    if (checkpointing) {
        done = ckDone;
        information = ckInformation;
        pendingZ = ckPendingZ;
        ackedNeighbors = ckAcked;
        unreachableNeighbors = ckUnreachable;
        EV_INFO << "Node " << nodeId << ": state RESTORED from checkpoint (informed="
                << done << ")\n";

        std::map<int, std::set<int>> toResend = pendingZ;
        for (auto &kv : toResend) {
            int n = kv.first;
            if (!ackedNeighbors.count(n) && !unreachableNeighbors.count(n)) {
                EV_INFO << "Node " << nodeId << ": resuming send to " << n << "\n";
                sendInformTo(n, kv.second);
            }
        }
    } else {
        done = false;
        information = 0;
        informedAt = -1;
        pendingZ.clear();
        retriesSoFar.clear();
        ackedNeighbors.clear();
        unreachableNeighbors.clear();
        for (auto &kv : retxTimer)
            cancelAndDelete(kv.second);
        retxTimer.clear();
        EV_INFO << "Node " << nodeId << ": cold restart, memory LOST (informed=false)\n";
    }

    getDisplayString().setTagArg("i", 1, done ? "green" : "gold");
    getDisplayString().setTagArg("i", 2, "60");
}

bool Node::linkIsCut(int neighborId)
{
    if (neighborId != partitionPeer) return false;
    if (partitionStart < SIMTIME_ZERO) return false;
    return simTime() >= partitionStart && simTime() < partitionEnd;
}

bool Node::isSilent()
{
    if (!isCrashTarget) return false;   // only the random victim of the run goes deaf
    if (silentStart < SIMTIME_ZERO) return false;
    return simTime() >= silentStart && simTime() < silentEnd;
}

void Node::sendInformTo(int neighborId, const std::set<int> &z)
{
    pendingZ[neighborId] = z;
    retriesSoFar[neighborId] = 0;

    DiffuseMessage *msg = makeDiffuse(z);

    if (linkIsCut(neighborId)) {
        messagesCut++;
        EV_WARN << "Node " << nodeId << ": PARTITION blocks message to " << neighborId << ".\n";
        delete msg;
    } else if (uniform(0, 1) < msgLossProb) {
        messagesDropped++;
        EV_WARN << "Node " << nodeId << ": message to " << neighborId << " LOST (simulated).\n";
        delete msg;
    } else {
        send(msg, "out", gateForNeighbor[neighborId]);
        messagesSent++;
    }

    cMessage *timer;
    if (retxTimer.count(neighborId)) {
        timer = retxTimer[neighborId];
    } else {
        timer = new cMessage("retx", RETX_TIMER);
        timer->addPar("neighborId") = neighborId;
        retxTimer[neighborId] = timer;
    }
    cancelEvent(timer);
    scheduleAt(simTime() + timeoutValue, timer);
}

void Node::sendAck(int senderId)
{
    cMessage *ack = new cMessage("ACK", ACK);
    ack->addPar("senderId") = nodeId;
    if (gateForNeighbor.count(senderId)) {
        if (linkIsCut(senderId)) {
            messagesCut++;
            EV_WARN << "Node " << nodeId << ": PARTITION blocks ACK to " << senderId << ".\n";
            delete ack;
        } else if (uniform(0, 1) >= msgLossProb) {
            send(ack, "out", gateForNeighbor[senderId]);
            messagesSent++;
        } else {
            messagesDropped++;
            EV_WARN << "Node " << nodeId << ": ACK to " << senderId << " LOST (simulated).\n";
            delete ack;
        }
    } else {
        delete ack;
    }
}

void Node::onRetxTimer(cMessage *msg)
{
    int neighborId = (int)msg->par("neighborId").longValue();
    if (ackedNeighbors.count(neighborId)) return;

    if (retriesSoFar[neighborId] < maxRetries) {
        retriesSoFar[neighborId]++;
        EV_INFO << "Node " << nodeId << ": no ACK from " << neighborId
                << ", RETRANSMIT (" << retriesSoFar[neighborId] << "/" << maxRetries << ")\n";
        DiffuseMessage *retry = makeDiffuse(pendingZ[neighborId]);
        if (linkIsCut(neighborId)) {
            messagesCut++;
            EV_WARN << "Node " << nodeId << ": PARTITION blocks retry to " << neighborId << ".\n";
            delete retry;
        } else if (uniform(0, 1) < msgLossProb) {
            messagesDropped++;
            EV_WARN << "Node " << nodeId << ": RETRY to " << neighborId << " LOST (simulated).\n";
            delete retry;
        } else {
            send(retry, "out", gateForNeighbor[neighborId]);
            messagesSent++;
        }
        scheduleAt(simTime() + timeoutValue, msg);
    } else {
        unreachableNeighbors.insert(neighborId);
        if (unreachableAt < 0) unreachableAt = simTime().dbl();
        saveCheckpoint();
        EV_WARN << "Node " << nodeId << ": neighbour " << neighborId
                << " UNREACHABLE after " << maxRetries << " retries -- presumed crashed.\n";
    }
}

void Node::onAckReceived(cMessage *msg)
{
    int senderId = (int)msg->par("senderId").longValue();
    ackedNeighbors.insert(senderId);
    if (retxTimer.count(senderId))
        cancelEvent(retxTimer[senderId]);
    saveCheckpoint();
}

void Node::handleMessage(cMessage *msg)
{
    if (msg->getKind() == CRASH_TIMER) {
        crashTimer = nullptr;
        delete msg;
        crashNow();
        return;
    }
    if (msg->getKind() == RECOVER_TIMER) {
        recoverTimer = nullptr;
        delete msg;
        recoverNow();
        return;
    }
    if (crashed) {
        delete msg;
        return;
    }
    if (isSilent() && msg->getKind() != RETX_TIMER) {
        messagesIgnored++;
        EV_WARN << "Node " << nodeId << ": TRANSIENT fault, ignoring message at t=" << simTime() << "\n";
        delete msg;
        return;
    }

    switch (msg->getKind()) {
        case INFORM:
            onInformReceived(check_and_cast<DiffuseMessage *>(msg));
            delete msg;
            break;
        case ACK:
            onAckReceived(msg);
            delete msg;
            break;
        case RETX_TIMER:
            onRetxTimer(msg);
            break;
        default:
            EV_WARN << "Node " << nodeId << ": unknown message kind.\n";
            delete msg;
    }
}

void Node::finish()
{
    recordScalar("informed", done ? 1 : 0);
    recordScalar("informedAt", informedAt);
    recordScalar("messagesSent", messagesSent);
    recordScalar("messagesDropped", messagesDropped);
    recordScalar("messagesCut", messagesCut);
    recordScalar("messagesIgnored", messagesIgnored);
    recordScalar("checkpointsSaved", checkpointsSaved);
    recordScalar("unreachableNeighbors", (long)unreachableNeighbors.size());
    recordScalar("crashed", crashed ? 1 : 0);
    recordScalar("crashedAt", crashedAt);
    recordScalar("recoveredAt", recoveredAt);
    recordScalar("unreachableAt", unreachableAt);
    recordScalar("wasInitiator", isInitiator ? 1 : 0);
    recordScalar("wasCrashTarget", isCrashTarget ? 1 : 0);

    for (auto &kv : retxTimer)
        cancelAndDelete(kv.second);
    if (crashTimer)
        cancelAndDelete(crashTimer);
    if (recoverTimer)
        cancelAndDelete(recoverTimer);
}
