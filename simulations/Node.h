#ifndef NODE_H
#define NODE_H

#include <omnetpp.h>
#include <map>
#include <set>
#include <vector>

using namespace omnetpp;

enum MessageKind {
    INFORM = 0,
    ACK = 1,
    RETX_TIMER = 2,
    CRASH_EVENT = 3,
    RECOVER_EVENT = 4
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
    bool informed = false;
    double informedAt = -1;
    cMessage *crashEvent = nullptr;

    simtime_t recoverTime;
    bool checkpointing;
    cMessage *recoverEvent = nullptr;

    int partitionPeer;
    simtime_t partitionStart;
    simtime_t partitionEnd;

    simtime_t silentStart;
    simtime_t silentEnd;

    std::vector<int> neighborIds;
    std::map<int, int> gateForNeighbor;

    std::map<int, std::set<int>> pendingZ;
    std::map<int, int> retriesSoFar;
    std::map<int, cMessage *> retxTimer;
    std::set<int> ackedNeighbors;
    std::set<int> unreachableNeighbors;

    bool ckInformed = false;
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

    void crashNow();
    void recoverNow();
    void saveCheckpoint();
    bool linkIsCut(int neighborId);
    bool isSilent();
    void buildNeighborTable();
    void sendInformTo(int neighborId, const std::set<int> &z);
    void onInformReceived(cMessage *msg);
    void onAckReceived(cMessage *msg);
    void onRetxTimer(cMessage *msg);

    static std::string setToStr(const std::set<int> &s);
    static std::set<int> strToSet(const std::string &s);
};

#endif
