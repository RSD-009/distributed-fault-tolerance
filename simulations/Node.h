#ifndef NODE_H
#define NODE_H

#include <omnetpp.h>

using namespace omnetpp;

// Message kinds exchanged between nodes.
// Student B: add ELECTION_MSG, COORDINATOR_MSG, CHECKPOINT_MSG here.
enum MessageKind {
    HEARTBEAT_PING = 0,
    HEARTBEAT_ACK = 1
};

class Node : public cSimpleModule
{
  private:
    int nodeId;
    simtime_t heartbeatInterval;
    simtime_t timeoutValue;
    cMessage *heartbeatTimer = nullptr;

    // Student C: add a per-neighbour "suspected" bool array here once
    // fault injection + timeout checking is wired up.

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    void sendHeartbeats();
    void handleHeartbeatPing(cMessage *msg);
    void handleHeartbeatAck(cMessage *msg);
};

#endif
