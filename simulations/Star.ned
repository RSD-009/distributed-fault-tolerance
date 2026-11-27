package simulations;

network Star
{
    parameters:
        int numNodes = default(7);
        bool replication = default(false);
        int initiatorId = default(intuniform(0, numNodes-1));
        int crashNodeId = default((initiatorId + intuniform(1, numNodes-1)) % numNodes); // never the initiator
        int partitionNodeId = default(intuniform(1, numNodes-1));
        int partitionParentId = default(0);
    submodules:
        node[numNodes]: Node {
            nodeId = index;
            isInitiator = (index == parent.initiatorId);
            isCrashTarget = (index == parent.crashNodeId);
            partitionPeer = (index == parent.partitionNodeId) ? parent.partitionParentId
                          : (index == parent.partitionParentId) ? parent.partitionNodeId : -1;
        }
    connections allowunconnected:
        for i=1..numNodes-1 {
            node[0].out++ --> {delay = 5ms;} --> node[i].in++;
            node[i].out++ --> {delay = 5ms;} --> node[0].in++;
        }
}
