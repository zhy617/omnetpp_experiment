simple Node
{
    parameters:
        double x;
        double y;
        int dataNodeNum;
        int csma_type;
        double p;

        @display("p=$x,$y;i=device/accesspoint,gold");

    gates:
        input in[dataNodeNum];
        output out[dataNodeNum];
}

