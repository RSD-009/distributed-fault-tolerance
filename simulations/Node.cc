#include "Node.h"
#include <sstream>
#include <algorithm>

Define_Module(Node);

std::string Node::setToStr(const std::set<int> &s)
{
    std::ostringstream oss;
    for (auto it = s.begin(); it != s.end(); ++it) {
        if (it != s.begin()) oss << ",";
        oss << *it;
    }
    return oss.str();
}

std::set<int> Node::strToSet(const std::string &s)
{
    std::set<int> result;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (!item.empty()) result.insert(std::stoi(item));
    }
    return result;
}

void Node::buildNeighborTable()
{
    for (int i = 0; i < gateSize("out"); i++) {
        cGate *remote = gate("out", i)->getNextGate();
        int neighborId = remote->getOwnerModule()->par("nodeId");
        neighborIds.push_back(neighborId);
        gateForNeighbor[neighborId] = i;
    }
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

    buildNeighborTable();

    if (isCrashTarget && crashTime >= SIMTIME_ZERO) {
        crashEvent = new cMessage("crash", CRASH_EVENT);
        scheduleAt(crashTime, crashEvent);
    }
    if (isCrashTarget && crashTime >= SIMTIME_ZERO && recoverTime > crashTime) {
        recoverEvent = new cMessage("recover", RECOVER_EVENT);
        scheduleAt(recoverTime, recoverEvent);
    }

    if (isInitiator) {
        informed = true;
        informedAt = simTime().dbl();
        getDisplayString().setTagArg("i", 1, "green");
        getDisplayString().setTagArg("i", 2, "60");
        std::set<int> z(neighborIds.begin(), neighborIds.end());
        z.insert(nodeId);
        for (int n : neighborIds)
            sendInformTo(n, z);
        saveCheckpoint();
        EV_INFO << "Node " << nodeId << ": INITIATOR, diffusion started.\n";
    }
}

void Node::saveCheckpoint()
{
    if (!checkpointing) return;
    ckInformed = informed;
    ckPendingZ = pendingZ;
    ckAcked = ackedNeighbors;
    ckUnreachable = unreachableNeighbors;
    checkpointsSaved++;
}

void Node::crashNow()
{
    crashed = true;
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
    bubble("RECOVERED");
    EV_INFO << "Node " << nodeId << ": RECOVERED at t=" << simTime() << "\n";

    if (checkpointing) {
        informed = ckInformed;
        pendingZ = ckPendingZ;
        ackedNeighbors = ckAcked;
        unreachableNeighbors = ckUnreachable;
        EV_INFO << "Node " << nodeId << ": state RESTORED from checkpoint (informed="
                << informed << ")\n";

        std::map<int, std::set<int>> toResend = pendingZ;
        for (auto &kv : toResend) {
            int n = kv.first;
            if (!ackedNeighbors.count(n) && !unreachableNeighbors.count(n)) {
                EV_INFO << "Node " << nodeId << ": resuming send to " << n << "\n";
                sendInformTo(n, kv.second);
            }
        }
    } else {
        informed = false;
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

    getDisplayString().setTagArg("i", 1, informed ? "green" : "gold");
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
    if (silentStart < SIMTIME_ZERO) return false;
    return simTime() >= silentStart && simTime() < silentEnd;
}

void Node::sendInformTo(int neighborId, const std::set<int> &z)
{
    pendingZ[neighborId] = z;
    retriesSoFar[neighborId] = 0;

    cMessage *msg = new cMessage("INFORM", INFORM);
    msg->addPar("senderId") = nodeId;
    msg->addPar("zset") = setToStr(z).c_str();

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

void Node::onRetxTimer(cMessage *msg)
{
    int neighborId = (int)msg->par("neighborId").longValue();
    if (ackedNeighbors.count(neighborId)) return;

    if (retriesSoFar[neighborId] < maxRetries) {
        retriesSoFar[neighborId]++;
        EV_INFO << "Node " << nodeId << ": no ACK from " << neighborId
                << ", RETRANSMIT (" << retriesSoFar[neighborId] << "/" << maxRetries << ")\n";
        cMessage *retry = new cMessage("INFORM", INFORM);
        retry->addPar("senderId") = nodeId;
        retry->addPar("zset") = setToStr(pendingZ[neighborId]).c_str();
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
        saveCheckpoint();
        EV_WARN << "Node " << nodeId << ": neighbour " << neighborId
                << " UNREACHABLE after " << maxRetries << " retries -- presumed crashed.\n";
    }
}

void Node::onInformReceived(cMessage *msg)
{
    int senderId = (int)msg->par("senderId").longValue();
    std::set<int> z = strToSet(msg->par("zset").stringValue());

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

    if (!informed) {
        informed = true;
        informedAt = simTime().dbl();
        getDisplayString().setTagArg("i", 1, "green");
        getDisplayString().setTagArg("i", 2, "60");
        EV_INFO << "Node " << nodeId << ": INFORMED (via " << senderId << ")\n";

        std::vector<int> y;
        for (int n : neighborIds)
            if (!z.count(n)) y.push_back(n);

        if (!y.empty()) {
            std::set<int> newZ = z;
            for (int n : y) newZ.insert(n);
            newZ.insert(nodeId);
            for (int n : y)
                sendInformTo(n, newZ);
        }
        saveCheckpoint();
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
    if (msg->getKind() == CRASH_EVENT) {
        crashEvent = nullptr;
        delete msg;
        crashNow();
        return;
    }
    if (msg->getKind() == RECOVER_EVENT) {
        recoverEvent = nullptr;
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
            onInformReceived(msg);
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
    recordScalar("informed", informed ? 1 : 0);
    recordScalar("informedAt", informedAt);
    recordScalar("messagesSent", messagesSent);
    recordScalar("messagesDropped", messagesDropped);
    recordScalar("messagesCut", messagesCut);
    recordScalar("messagesIgnored", messagesIgnored);
    recordScalar("checkpointsSaved", checkpointsSaved);
    recordScalar("unreachableNeighbors", (long)unreachableNeighbors.size());
    recordScalar("crashed", crashed ? 1 : 0);
    recordScalar("wasInitiator", isInitiator ? 1 : 0);
    recordScalar("wasCrashTarget", isCrashTarget ? 1 : 0);

    for (auto &kv : retxTimer)
        cancelAndDelete(kv.second);
    if (crashEvent)
        cancelAndDelete(crashEvent);
    if (recoverEvent)
        cancelAndDelete(recoverEvent);
}
