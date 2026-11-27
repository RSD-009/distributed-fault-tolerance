package simulations;

simple Node
{
    parameters:
        int nodeId;
        bool isInitiator = default(false);  // set by the network, ONE random node per run
        bool isCrashTarget = default(false); // set by the network, ONE random node per run
        double timeout @unit(s) = default(50ms);
        int maxRetries = default(3);
        double msgLossProb = default(0);
        double crashTime @unit(s) = default(-1s); // applies ONLY if isCrashTarget is true

        int partitionPeer = default(-1);      // set by the network for the 2 chosen nodes
        double partitionStart @unit(s) = default(-1s);
        double partitionEnd @unit(s) = default(-1s);

        double silentStart @unit(s) = default(-1s);
        double silentEnd @unit(s) = default(-1s);

        double recoverTime @unit(s) = default(-1s);
        bool checkpointing = default(false);
        bool useZ = default(true);   // true: diffuse-v3 (promesse Z), false: style v2 (tous les voisins sauf l'expediteur)

        @display("i=device/server");
    gates:
        input in[];
        output out[];
}
