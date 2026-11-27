package simulations;

network Mesh
{
    parameters:
        int numNodes = default(11);
        bool replication = default(false);
        int initiatorId = default(intuniform(0, numNodes-1));
        int crashNodeId = default((initiatorId + intuniform(1, numNodes-1)) % numNodes); // never the initiator
    submodules:
        node[numNodes]: Node {
            nodeId = index;
            isInitiator = (index == parent.initiatorId);
            isCrashTarget = (index == parent.crashNodeId);
        }
    connections allowunconnected:
        node[0].out++ --> {delay=5ms;} --> node[6].in++;  node[6].out++ --> {delay=5ms;} --> node[0].in++;
        node[0].out++ --> {delay=5ms;} --> node[8].in++;  node[8].out++ --> {delay=5ms;} --> node[0].in++;
        node[0].out++ --> {delay=5ms;} --> node[9].in++;  node[9].out++ --> {delay=5ms;} --> node[0].in++;
        node[0].out++ --> {delay=5ms;} --> node[1].in++;  node[1].out++ --> {delay=5ms;} --> node[0].in++;

        node[8].out++ --> {delay=5ms;} --> node[6].in++;  node[6].out++ --> {delay=5ms;} --> node[8].in++;
        node[8].out++ --> {delay=5ms;} --> node[7].in++;  node[7].out++ --> {delay=5ms;} --> node[8].in++;
        node[8].out++ --> {delay=5ms;} --> node[9].in++;  node[9].out++ --> {delay=5ms;} --> node[8].in++;
        node[8].out++ --> {delay=5ms;} --> node[10].in++; node[10].out++ --> {delay=5ms;} --> node[8].in++;
        node[8].out++ --> {delay=5ms;} --> node[1].in++;  node[1].out++ --> {delay=5ms;} --> node[8].in++;
        node[8].out++ --> {delay=5ms;} --> node[5].in++;  node[5].out++ --> {delay=5ms;} --> node[8].in++;

        node[6].out++ --> {delay=5ms;} --> node[7].in++;  node[7].out++ --> {delay=5ms;} --> node[6].in++;
        node[6].out++ --> {delay=5ms;} --> node[9].in++;  node[9].out++ --> {delay=5ms;} --> node[6].in++;
        node[6].out++ --> {delay=5ms;} --> node[10].in++; node[10].out++ --> {delay=5ms;} --> node[6].in++;
        node[6].out++ --> {delay=5ms;} --> node[1].in++;  node[1].out++ --> {delay=5ms;} --> node[6].in++;

        node[7].out++ --> {delay=5ms;} --> node[9].in++;  node[9].out++ --> {delay=5ms;} --> node[7].in++;
        node[7].out++ --> {delay=5ms;} --> node[10].in++; node[10].out++ --> {delay=5ms;} --> node[7].in++;
        node[7].out++ --> {delay=5ms;} --> node[1].in++;  node[1].out++ --> {delay=5ms;} --> node[7].in++;
        node[7].out++ --> {delay=5ms;} --> node[5].in++;  node[5].out++ --> {delay=5ms;} --> node[7].in++;
        node[7].out++ --> {delay=5ms;} --> node[2].in++;  node[2].out++ --> {delay=5ms;} --> node[7].in++;
        node[7].out++ --> {delay=5ms;} --> node[4].in++;  node[4].out++ --> {delay=5ms;} --> node[7].in++;

        node[9].out++ --> {delay=5ms;} --> node[10].in++; node[10].out++ --> {delay=5ms;} --> node[9].in++;
        node[9].out++ --> {delay=5ms;} --> node[1].in++;  node[1].out++ --> {delay=5ms;} --> node[9].in++;

        node[10].out++ --> {delay=5ms;} --> node[1].in++; node[1].out++ --> {delay=5ms;} --> node[10].in++;

        node[1].out++ --> {delay=5ms;} --> node[2].in++;  node[2].out++ --> {delay=5ms;} --> node[1].in++;
        node[1].out++ --> {delay=5ms;} --> node[5].in++;  node[5].out++ --> {delay=5ms;} --> node[1].in++;
        node[1].out++ --> {delay=5ms;} --> node[4].in++;  node[4].out++ --> {delay=5ms;} --> node[1].in++;

        node[2].out++ --> {delay=5ms;} --> node[3].in++;  node[3].out++ --> {delay=5ms;} --> node[2].in++;
        node[2].out++ --> {delay=5ms;} --> node[4].in++;  node[4].out++ --> {delay=5ms;} --> node[2].in++;
        node[2].out++ --> {delay=5ms;} --> node[5].in++;  node[5].out++ --> {delay=5ms;} --> node[2].in++;

        node[3].out++ --> {delay=5ms;} --> node[4].in++;  node[4].out++ --> {delay=5ms;} --> node[3].in++;

        node[4].out++ --> {delay=5ms;} --> node[5].in++;  node[5].out++ --> {delay=5ms;} --> node[4].in++;
}
