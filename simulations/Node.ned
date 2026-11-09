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

        // NEW: network partition = one link is cut during a time window
        int partitionPeer = default(-1);               // neighbour at the other end of the cut link (-1 = none)
        double partitionStart @unit(s) = default(-1s); // when the cut starts
        double partitionEnd @unit(s) = default(-1s);   // when the link works again

        // NEW: transient fault = node is deaf during a window, then recovers
        double silentStart @unit(s) = default(-1s);
        double silentEnd @unit(s) = default(-1s);

        @display("i=device/server");
    gates:
        input in[];
        output out[];
}
