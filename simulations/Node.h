#ifndef NODE_H
#define NODE_H

#include <omnetpp.h>
#include <map>
#include <set>
#include <vector>
#include "DiffuseMessage_m.h"      // generated from DiffuseMessage.msg

using namespace omnetpp;

enum MessageKind {
    INFORM = 0,          // a DiffuseMessage (diffuse-v3)
    ACK = 1,             // added by us (fault tolerance)
    RETX_TIMER = 2,      // added by us (fault tolerance)
    CRASH_TIMER = 3,
    RECOVER_TIMER = 4
};

class Node : public cSimpleModule
{
  private:
    int nodeId;
    bool isInitiator;
    bool isCrashTarget;
    simtime_t timeoutValue;
    int maxRetries;
    double msgLossProb;
    simtime_t crashTime;
    bool crashed = false;
    double informedAt = -1;
    cMessage *crashTimer = nullptr;

    simtime_t recoverTime;
    bool checkpointing;
    cMessage *recoverTimer = nullptr;

    int partitionPeer;
    simtime_t partitionStart;
    simtime_t partitionEnd;

    simtime_t silentStart;
    simtime_t silentEnd;

    // ---- diffuse-v3 (course Chapter 2) ----
    bool done = false;                 // course state "done" (was: informed)
    int information = 0;               // I, the information received
    std::vector<int> N;                // N(x): the neighbours of this node
    std::map<int, int> gateForNeighbor;

    // ---- fault tolerance (ours, unchanged) ----
    std::map<int, std::set<int>> pendingZ;
    std::map<int, int> retriesSoFar;
    std::map<int, cMessage *> retxTimer;
    std::set<int> ackedNeighbors;
    std::set<int> unreachableNeighbors;

    bool ckDone = false;
    int ckInformation = 0;
    std::map<int, std::set<int>> ckPendingZ;
    std::set<int> ckAcked;
    std::set<int> ckUnreachable;
    long checkpointsSaved = 0;

    long messagesSent = 0;
    long messagesDropped = 0;
    long messagesCut = 0;
    long messagesIgnored = 0;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    // diffuse-v3
    void onInformReceived(DiffuseMessage *msg);
    DiffuseMessage *makeDiffuse(const std::set<int> &z);
    std::set<int> zOf(DiffuseMessage *msg);

    // fault tolerance
    void crashNow();
    void recoverNow();
    void saveCheckpoint();
    bool linkIsCut(int neighborId);
    bool isSilent();
    void buildNeighborTable();
    void sendInformTo(int neighborId, const std::set<int> &z);
    void sendAck(int senderId);
    void onAckReceived(cMessage *msg);
    void onRetxTimer(cMessage *msg);
};

#endif
