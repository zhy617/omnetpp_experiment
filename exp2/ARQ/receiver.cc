/*
 * receiver.cc
 *
 *  Created on: 2024Äê10ÔÂ30ÈÕ
 *      Author: 13183
 */


#include <string.h>
#include <omnetpp.h>
using namespace omnetpp;

class receiver:public cSimpleModule
{
    protected:
    virtual void handleMessage(cMessage *msg);
};

Define_Module(receiver);
void receiver :: handleMessage(cMessage *msg)
{

    if(uniform(0,1)<0.3)
    {
        EV << "\"Losing\"message" << msg << endl;
        bubble("message lost");
        delete msg;
    }
    else
    {
        EV << msg << "recv ,send ack. \n";
        delete msg;
        send(new cMessage("ack"), "out");
    }
}

