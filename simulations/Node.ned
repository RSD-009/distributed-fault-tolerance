package simulations;

//
// A single process/node running diffuse-v3 (course Chapter 2: Broadcasting),
// plus a fault-tolerance layer on top (ACK + timeout-based retransmission).
// Owns one in/out gate pair per neighbour link (connected in Network.ned).
//
simple Node
{
    parameters:
        int nodeId;                       // 0..N-1, set from Network.ned
        bool isInitiator = default(false); // exactly one node starts diffuse-v3
        double timeout @unit(s) = default(50ms); // prof's example value
        int maxRetries = default(3);
        double msgLossProb = default(0);  // probability a given send is dropped
        double crashTime @unit(s) = default(-1s); // -1 = never crash
        @display("i=device/server");
    gates:
        input in[];
        output out[];
}
