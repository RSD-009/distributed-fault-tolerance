package simulations;

network Network
{
    parameters:
        int numNodes = default(7);
        bool replication = default(false);
        int initiatorId = default(intuniform(0, numNodes-1));
        int crashNodeId = default(intuniform(0, numNodes-1));
        int partitionNodeId = default(intuniform(1, numNodes-1));
        int partitionParentId = default(int(floor((partitionNodeId-1)/2.0)));
    submodules:
        node[numNodes]: Node {
            nodeId = index;
            isInitiator = (index == parent.initiatorId);
            isCrashTarget = (index == parent.crashNodeId);
            partitionPeer = (index == parent.partitionNodeId) ? parent.partitionParentId
                          : (index == parent.partitionParentId) ? parent.partitionNodeId : -1;
        }
    connections allowunconnected:
        for i=0..numNodes-1 {
            node[i].out++ --> {delay = 5ms;} --> node[2*i+1].in++ if 2*i+1 < numNodes;
            node[2*i+1].out++ --> {delay = 5ms;} --> node[i].in++ if 2*i+1 < numNodes;
            node[i].out++ --> {delay = 5ms;} --> node[2*i+2].in++ if 2*i+2 < numNodes;
            node[2*i+2].out++ --> {delay = 5ms;} --> node[i].in++ if 2*i+2 < numNodes;
        }
        node[2].out++ --> {delay = 5ms;} --> node[3].in++ if replication && numNodes > 4;
        node[3].out++ --> {delay = 5ms;} --> node[2].in++ if replication && numNodes > 4;
        node[2].out++ --> {delay = 5ms;} --> node[4].in++ if replication && numNodes > 4;
        node[4].out++ --> {delay = 5ms;} --> node[2].in++ if replication && numNodes > 4;
}
