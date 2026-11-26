//
// Message of diffuse-v3 (same definition as the lab: Z[] and information).
//
message DiffuseMessage
{
    int Z[];           // control knowledge Z: nodes already informed or about to be
    int information;   // the information I being broadcast
}
