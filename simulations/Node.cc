#include "Node.h"

Define_Module(Node);

void Node::initialize()
{
    nodeId = par("nodeId");
    heartbeatInterval = par("heartbeatInterval");
    timeoutValue = par("timeout");

    heartbeatTimer = new cMessage("heartbeatTimer");
    scheduleAt(simTime() + heartbeatInterval, heartbeatTimer);

    EV << "Node " << nodeId << " initialized.\n";
}

void Node::handleMessage(cMessage *msg)
{
    if (msg == heartbeatTimer) {
        sendHeartbeats();
        scheduleAt(simTime() + heartbeatInterval, heartbeatTimer);
        return;
    }

    switch (msg->getKind()) {
        case HEARTBEAT_PING:
            handleHeartbeatPing(msg);
            break;
        case HEARTBEAT_ACK:
            handleHeartbeatAck(msg);
            break;
        default:
            EV << "Node " << nodeId << ": unknown message kind.\n";
    }
    delete msg;
}

void Node::sendHeartbeats()
{
    // Student C: this is the natural hook point for fault injection —
    // e.g. "if (this node is crashed) return;" before sending,
    // or drop sends to a partitioned neighbour.
    for (int i = 0; i < gateSize("out"); i++) {
        cMessage *ping = new cMessage("ping", HEARTBEAT_PING);
        send(ping, "out", i);
    }
}

void Node::handleHeartbeatPing(cMessage *msg)
{
    int arrivalGateIndex = msg->getArrivalGate()->getIndex();
    cMessage *ack = new cMessage("ack", HEARTBEAT_ACK);
    send(ack, "out", arrivalGateIndex);
}

void Node::handleHeartbeatAck(cMessage *msg)
{
    // Student B: this is where you'd reset a per-neighbour "last seen" timestamp.
    // Student C: compare simTime() - lastSeen against timeoutValue to suspect a fault.
    EV << "Node " << nodeId << ": got heartbeat ack.\n";
}

void Node::finish()
{
    // Student C: recordScalar() your metrics here, e.g.:
    // recordScalar("messagesSent", messagesSentCount);
    cancelAndDelete(heartbeatTimer);
}
