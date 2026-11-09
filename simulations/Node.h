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
    CRASH_EVENT = 3     // tells the node "you crash now"
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
    cMessage *crashEvent = nullptr;   // the scheduled crash

    // NEW: partition settings (one cut link, one time window)
    int partitionPeer;
    simtime_t partitionStart;
    simtime_t partitionEnd;

    // NEW: transient fault window
    simtime_t silentStart;
    simtime_t silentEnd;

    std::vector<int> neighborIds;
    std::map<int, int> gateForNeighbor;

    std::map<int, std::set<int>> pendingZ;
    std::map<int, int> retriesSoFar;
    std::map<int, cMessage *> retxTimer;
    std::set<int> ackedNeighbors;
    std::set<int> unreachableNeighbors;

    long messagesSent = 0;
    long messagesDropped = 0;
    long messagesCut = 0;             // NEW: messages blocked by the partition
    long messagesIgnored = 0;         // NEW: messages ignored while silent

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    void crashNow();
    bool linkIsCut(int neighborId);   // NEW: is the link to this neighbour cut right now?
    bool isSilent();                  // NEW: is this node deaf right now?
    void buildNeighborTable();
    void sendInformTo(int neighborId, const std::set<int> &z);
    void onInformReceived(cMessage *msg);
    void onAckReceived(cMessage *msg);
    void onRetxTimer(cMessage *msg);

    static std::string setToStr(const std::set<int> &s);
    static std::set<int> strToSet(const std::string &s);
};

#endif
