/*
 * sender.cc
 *
 *  Created on: 2024Äê10ÔÂ30ÈÕ
 *      Author: 13183
 */



#include <string.h>
#include <omnetpp.h>
using namespace omnetpp;
class sender:public cSimpleModule
{
   private:
    int seq;//sequence number
    cMessage *timeoutEvent;//timeout self-message
    simtime_t timeout; //timeout
    cMessage *message;

public:
    sender();
    virtual ~sender();

protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    virtual void sendCopyOf(cMessage *msg);
    virtual cMessage *generateNewMessage();
};

Define_Module(sender);

sender::sender()
{
    timeoutEvent=NULL;
    message = NULL;
}
sender::~sender()
{
    cancelAndDelete(timeoutEvent);
    delete message;
}


cMessage *sender::generateNewMessage()
{
    //generate a message
    char msgname[20];
    sprintf(msgname,"send-%d",++seq);
    cMessage *msg = new cMessage(msgname);
    return msg;
}

void sender::sendCopyOf(cMessage *msg)
{
    cMessage *copy = (cMessage *)msg->dup();
    send(copy,"out");
}

void sender::initialize()
{
    // Initialize variable
    seq=0;
    timeout= 1.0;
    timeoutEvent = new cMessage("timeoutEvent");

    //generate and send initial message
    EV << "Sending initial message\n";
    message = generateNewMessage();
    sendCopyOf (message);
    scheduleAt(simTime()+timeout,timeoutEvent);
}


void sender::handleMessage(cMessage *msg)
{
    if(msg == timeoutEvent)//timeout re-send
    {
        EV << "Timeout expired, re-sending message\n";
        sendCopyOf(message);
        scheduleAt(simTime()+timeout,timeoutEvent);
    }
    else //rcv ACK
    {
        EV << "Received:" << msg->getName() << "\n";
        delete msg;

        EV << "Timer cancelled. \n";

        cancelEvent(timeoutEvent);
        delete message;

        //send new message
        message = generateNewMessage();
        sendCopyOf(message);

        scheduleAt(simTime()+timeout, timeoutEvent);
    }
}

