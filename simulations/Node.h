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
    CRASH_EVENT = 3     // NEW: tells the node "you crash now"
};

class Node : public cSimpleModule
{
  private:
    int nodeId;
    bool isInitiator;
    simtime_t timeoutValue;
    int maxRetries;
    double msgLossProb;
    simtime_t crashTime;
    bool crashed = false;
    bool informed = false;
    cMessage *crashEvent = nullptr;   // NEW: the scheduled crash

    std::vector<int> neighborIds;
    std::map<int, int> gateForNeighbor;

    std::map<int, std::set<int>> pendingZ;
    std::map<int, int> retriesSoFar;
    std::map<int, cMessage *> retxTimer;
    std::set<int> ackedNeighbors;
    std::set<int> unreachableNeighbors;

    long messagesSent = 0;
    long messagesDropped = 0;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    void crashNow();                  // CHANGED: was maybeCrashNow()
    void buildNeighborTable();
    void sendInformTo(int neighborId, const std::set<int> &z);
    void onInformReceived(cMessage *msg);
    void onAckReceived(cMessage *msg);
    void onRetxTimer(cMessage *msg);

    static std::string setToStr(const std::set<int> &s);
    static std::set<int> strToSet(const std::string &s);
};

#endif
