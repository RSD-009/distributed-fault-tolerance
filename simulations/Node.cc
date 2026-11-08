#include "Node.h"
#include <sstream>
#include <algorithm>

Define_Module(Node);

// ---- small helpers for serializing the Z set onto a message par ----
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
    timeoutValue = par("timeout");
    maxRetries = par("maxRetries");
    msgLossProb = par("msgLossProb");
    crashTime = par("crashTime");

    buildNeighborTable();

    if (isInitiator) {
        informed = true;
        std::set<int> z(neighborIds.begin(), neighborIds.end());
        z.insert(nodeId);
        for (int n : neighborIds)
            sendInformTo(n, z);
        EV_INFO << "Node " << nodeId << ": INITIATOR, diffusion started.\n";
    }
}

void Node::maybeCrashNow()
{
    if (!crashed && crashTime >= SIMTIME_ZERO && simTime() >= crashTime) {
        crashed = true;
        EV_INFO << "Node " << nodeId << ": CRASHED at t=" << simTime() << "\n";
        for (auto &kv : retxTimer)
            cancelEvent(kv.second);
    }
}

void Node::sendInformTo(int neighborId, const std::set<int> &z)
{
    pendingZ[neighborId] = z;
    retriesSoFar[neighborId] = 0;

    cMessage *msg = new cMessage("INFORM", INFORM);
    msg->addPar("senderId") = nodeId;
    msg->addPar("zset") = setToStr(z).c_str();

    if (uniform(0, 1) < msgLossProb) {
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
        if (uniform(0, 1) < msgLossProb) {
            messagesDropped++;
            delete retry;
        } else {
            send(retry, "out", gateForNeighbor[neighborId]);
            messagesSent++;
        }
        scheduleAt(simTime() + timeoutValue, msg);
    } else {
        unreachableNeighbors.insert(neighborId);
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
        if (uniform(0, 1) >= msgLossProb) {
            send(ack, "out", gateForNeighbor[senderId]);
            messagesSent++;
        } else {
            messagesDropped++;
            delete ack;
        }
    } else {
        delete ack;
    }

    if (!informed) {
        informed = true;
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
    }
}

void Node::onAckReceived(cMessage *msg)
{
    int senderId = (int)msg->par("senderId").longValue();
    ackedNeighbors.insert(senderId);
    if (retxTimer.count(senderId))
        cancelEvent(retxTimer[senderId]);
}

void Node::handleMessage(cMessage *msg)
{
    maybeCrashNow();
    if (crashed) {
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
    recordScalar("messagesSent", messagesSent);
    recordScalar("messagesDropped", messagesDropped);
    recordScalar("unreachableNeighbors", (long)unreachableNeighbors.size());
    recordScalar("crashed", crashed ? 1 : 0);

    for (auto &kv : retxTimer)
        cancelAndDelete(kv.second);
}
