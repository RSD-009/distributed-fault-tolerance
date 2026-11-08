package simulations;

//
// A binary-tree topology of N nodes (node 0 = root = diffuse-v3 initiator).
// Chosen over a full mesh deliberately: in a full mesh every node is directly
// reachable from the initiator in one hop, which makes diffuse-v3's whole
// point (avoiding redundant relays via the Z set) invisible. A tree forces
// real multi-hop relaying, and makes a crashed relay's impact obvious: its
// entire subtree becomes unreachable -- exactly what the "availability"
// metric in the project brief is meant to capture.
//
// Student A: for a denser/more redundant topology (so a single crash can't
// strand a subtree), add extra cross-links here -- the Node.cc logic doesn't
// care about topology shape, it reads N(x) straight from the wiring.
//
network Network
{
    parameters:
        int numNodes = default(7); // keep between 5 and 10 per the project spec
    submodules:
        node[numNodes]: Node {
            nodeId = index;
            isInitiator = (index == 0);
        }
    connections allowunconnected:
        for i=0..numNodes-1 {
            node[i].out++ --> {delay = 5ms;} --> node[2*i+1].in++ if 2*i+1 < numNodes;
            node[2*i+1].out++ --> {delay = 5ms;} --> node[i].in++ if 2*i+1 < numNodes;
            node[i].out++ --> {delay = 5ms;} --> node[2*i+2].in++ if 2*i+2 < numNodes;
            node[2*i+2].out++ --> {delay = 5ms;} --> node[i].in++ if 2*i+2 < numNodes;
        }
}
