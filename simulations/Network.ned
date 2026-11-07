package simulations;

//
// Full-mesh network of N nodes. N is set per-run from omnetpp.ini
// (keep it between 5 and 10 as the project spec requires).
// Student A: this is the topology — change here if you want ring/star instead.
//
network Network
{
    parameters:
        int numNodes = default(6);
    submodules:
        node[numNodes]: Node {
            nodeId = index;
        }
    connections allowunconnected:
        // Wire every node to every other node (full mesh).
        // For a ring or star topology, replace this "for" loop.
        for i=0..numNodes-1, for j=0..numNodes-1 {
            node[i].out++ --> {delay = 5ms;} --> node[j].in++ if i != j;
        }
}
