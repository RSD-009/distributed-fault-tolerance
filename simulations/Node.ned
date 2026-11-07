package simulations;

//
// A single process/node in the distributed system.
// Owns one in/out gate pair per neighbour link (connected in Network.ned).
// Student B: add leader-election + checkpointing logic in Node.cc.
// Student C: this module is also where fault injection hooks in
// (crash = stop scheduling; partition/loss = drop messages in the channel).
//
simple Node
{
    parameters:
        int nodeId;                      // 0..N-1, set from Network.ned
        double heartbeatInterval @unit(ms) = default(20ms);
        double timeout @unit(ms) = default(50ms); // from the prof's example
        @display("i=device/server");
    gates:
        input in[];
        output out[];
}
