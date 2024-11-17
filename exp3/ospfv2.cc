/*
 * OspfRouter.cc
 *
 *  Created on: 2024年11月7日
 *      Author: 13183
 */
#include <map>
#include <memory.h>
#include <stdlib.h>
#include <string>

#include "inet/common/ModuleAccess.h"
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/routing/ospfv2/Ospfv2.h"
#include "inet/routing/ospfv2/Ospfv2ConfigReader.h"
#include "inet/routing/ospfv2/messagehandler/MessageHandler.h"

namespace inet
{
    namespace ospf
    {

        Define_Module(Ospf);

        Ospf::Ospf()
        {
        }

        Ospf::~Ospf()
        {
            cancelAndDelete(startupTimer);
            delete ospfRouter;
        }

        void Ospf::initialize(int stage)
        {
            RoutingProtocolBase::initialize(stage);

            if (stage == INITSTAGE_LOCAL)
            {
                host = getContainingNode(this); // 获取主机
                ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
                rt = getModuleFromPar<IIpv4RoutingTable>(par("routingTableModule"), this);
                startupTimer = new cMessage("OSPF-startup");
            }
            else if (stage == INITSTAGE_ROUTING_PROTOCOLS)
            { // interfaces and static routes are already initialized
                registerService(Protocol::ospf, nullptr, gate("ipIn"));
                registerProtocol(Protocol::ospf, gate("ipOut"), nullptr);
            }
        }

        void Ospf::handleMessageWhenUp(cMessage *msg)
        {
            if (msg == startupTimer)
            { // 收到startupTimer 创建路由器，订阅信号
                createOspfRouter();
                subscribe();
            }
            else
                ospfRouter->getMessageHandler()->messageReceived(msg); // 收到message
        }

        void Ospf::createOspfRouter() // 创建OspfRouter
        {
            ospfRouter = new Router(this, ift, rt);

            // read the OSPF AS configuration
            cXMLElement *ospfConfig = par("ospfConfig");
            OspfConfigReader configReader(this, ift);
            if (!configReader.loadConfigFromXML(ospfConfig, ospfRouter)) // load from xml file
                throw cRuntimeError("Error reading AS configuration from %s", ospfConfig->getSourceLocation());

            ospfRouter->addWatches();
        }

        void Ospf::subscribe() // 订阅信号，方便监控
        {
            host->subscribe(interfaceCreatedSignal, this);
            host->subscribe(interfaceDeletedSignal, this);
            host->subscribe(interfaceStateChangedSignal, this);
        }

        void Ospf::unsubscribe()
        {
            host->unsubscribe(interfaceCreatedSignal, this);
            host->unsubscribe(interfaceDeletedSignal, this);
            host->unsubscribe(interfaceStateChangedSignal, this);
        }

        /**
         * Listen on interface changes and update private data structures.
         */
        void Ospf::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
        {
            Enter_Method("Ospf::receiveSignal");

            const InterfaceEntry *ie;
            const InterfaceEntryChangeDetails *change;

            if (signalID == interfaceCreatedSignal)
            { // 接口创建
                // configure interface for RIP
                ie = check_and_cast<const InterfaceEntry *>(obj);
                if (ie->isMulticast() && !ie->isLoopback())
                {
                    // TODO
                }
            }
            else if (signalID == interfaceDeletedSignal)
            { // 接口删除
                ie = check_and_cast<const InterfaceEntry *>(obj);
                // TODO
            }
            else if (signalID == interfaceStateChangedSignal)
            {
                change = check_and_cast<const InterfaceEntryChangeDetails *>(obj);
                if (change->getFieldId() == InterfaceEntry::F_CARRIER || change->getFieldId() == InterfaceEntry::F_STATE)
                {
                    ie = change->getInterfaceEntry();
                    if (!ie->isUp())
                        handleInterfaceDown(ie);
                    else
                    {
                        // interface went back online. Do nothing!
                        // Wait for Hello messages to establish adjacency.
                    }
                }
            }
            else
                throw cRuntimeError("Unexpected signal: %s", getSignalName(signalID));
        }

        void Ospf::handleStartOperation(LifecycleOperation *operation) // 处理开始操作：创建router + subscribe
        {
            ASSERT(ospfRouter == nullptr);
            simtime_t startupTime = par("startupTime");
            if (startupTime <= simTime())
            {
                createOspfRouter();
                subscribe();
            }
            else
                scheduleAt(simTime() + startupTime, startupTimer);
        }

        void Ospf::handleStopOperation(LifecycleOperation *operation)
        {
            ASSERT(ospfRouter);
            delete ospfRouter;
            cancelEvent(startupTimer);
            ospfRouter = nullptr;
            unsubscribe();
        }

        void Ospf::handleCrashOperation(LifecycleOperation *operation)
        {
            ASSERT(ospfRouter);
            delete ospfRouter;
            cancelEvent(startupTimer);
            ospfRouter = nullptr;
            unsubscribe();
        }

        void Ospf::insertExternalRoute(int ifIndex, const Ipv4AddressRange &netAddr)
        {
            Enter_Method_Silent();
            OspfAsExternalLsaContents newExternalContents;
            newExternalContents.setRouteCost(OSPF_BGP_DEFAULT_COST);
            newExternalContents.setExternalRouteTag(OSPF_EXTERNAL_ROUTES_LEARNED_BY_BGP);
            const Ipv4Address netmask = netAddr.mask;
            newExternalContents.setNetworkMask(netmask);
            ospfRouter->updateExternalRoute(netAddr.address, newExternalContents, ifIndex);
        }

        int Ospf::checkExternalRoute(const Ipv4Address &route)
        {
            Enter_Method_Silent();
            for (uint32_t i = 0; i < ospfRouter->getASExternalLSACount(); i++)
            {
                AsExternalLsa *externalLSA = ospfRouter->getASExternalLSA(i);
                Ipv4Address externalAddr = externalLSA->getHeader().getLinkStateID();
                if (externalAddr == route)
                { // FIXME was this meant???
                    if (externalLSA->getContents().getE_ExternalMetricType())
                        return 2;
                    else
                        return 1;
                }
            }
            return 0;
        }

        void Ospf::handleInterfaceDown(const InterfaceEntry *ie)
        {
            EV_DEBUG << "interface " << ie->getInterfaceId() << " went down. \n";

            // Step 1: delete all direct-routes connected to this interface

            // ... from OSPF table
            for (uint32_t i = 0; i < ospfRouter->getRoutingTableEntryCount(); i++)
            {
                OspfRoutingTableEntry *ospfRoute = ospfRouter->getRoutingTableEntry(i); // one router
                if (ospfRoute && ospfRoute->getInterface() == ie && ospfRoute->getNextHopAsGeneric().isUnspecified())
                {
                    EV_DEBUG << "removing route from OSPF routing table: " << ospfRoute << "\n";
                    ospfRouter->deleteRoute(ospfRoute);
                    i--;
                }
            }
            // ... from Ipv4 table
            for (int32_t i = 0; i < rt->getNumRoutes(); i++)
            {
                Ipv4Route *route = rt->getRoute(i);
                if (route && route->getInterface() == ie && route->getNextHopAsGeneric().isUnspecified())
                {
                    EV_DEBUG << "removing route from Ipv4 routing table: " << route << "\n";
                    rt->deleteRoute(route);
                    i--;
                }
            }

            // Step 2: find the OspfInterface associated with the ie and take it down
            OspfInterface *foundIntf = nullptr;
            for (auto &areaId : ospfRouter->getAreaIds())
            {
                Area *area = ospfRouter->getAreaByID(areaId);
                if (area)
                {
                    for (auto &ifIndex : area->getInterfaceIndices())
                    {
                        OspfInterface *intf = area->getInterface(ifIndex);
                        if (intf && intf->getIfIndex() == ie->getInterfaceId())
                        {
                            foundIntf = intf;
                            break;
                        }
                    }
                    if (foundIntf)
                    {
                        foundIntf->processEvent(OspfInterface::INTERFACE_DOWN);
                        break;
                    }
                }
            }
        }

    } // namespace ospf
} // namespace inet
