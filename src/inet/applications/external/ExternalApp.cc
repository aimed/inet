//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include <iostream>
#include <string>

#include "inet/applications/external/ExternalApp.h"

#include "../macapp/SimplePayload_m.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/applications/external/ExternalAppPayload_m.h"
#include "inet/applications/external/ApplicationAdapter.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/linklayer/common/SimpleLinkLayerControlInfo.h"


namespace inet {

using std::cout;

Define_Module(ExternalApp);

simsignal_t ExternalApp::rttSignal = registerSignal("rtt");
simsignal_t ExternalApp::numLostSignal = registerSignal("numLost");
simsignal_t ExternalApp::numOutOfOrderArrivalsSignal = registerSignal("numOutOfOrderArrivals");
simsignal_t ExternalApp::pingTxSeqSignal = registerSignal("pingTxSeq");
simsignal_t ExternalApp::pingRxSeqSignal = registerSignal("pingRxSeq");

#define TIMER_KIND 999


ExternalApp::ExternalApp()
{
    timer = nullptr;
}

ExternalApp::~ExternalApp()
{
    cancelAndDelete(timer);
}

void ExternalApp::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        // read params
        // addresses will be assigned later by ApplicationAdapter)
        packetSize = 0;
        printPing = par("printPing").boolValue();

        // state
        lastStart = -1;
        sendSeqNo = expectedReplySeqNo = 0;
        WATCH(sendSeqNo);
        WATCH(expectedReplySeqNo);

        // statistics
        rttStat.setName("pingRTT");
        sentCount = lossCount = outOfOrderArrivalCount = numPongs = 0;
        WATCH(lossCount);
        WATCH(outOfOrderArrivalCount);
        WATCH(numPongs);

        timer = new cMessage("timer");
        timer->setKind(TIMER_KIND);

        cModule* module = getParentModule()->getParentModule()->getSubmodule("adapter");
        adapter = check_and_cast<ApplicationAdapter*>(module);
        module = this->getParentModule()->getSubmodule("interfaceTable");
        interfaceTable = check_and_cast<IInterfaceTable*>(module);
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        // generate nodeId from MAC Adress
        InterfaceEntry* e = interfaceTable->getInterfaceByName("nic");
        srcAddr = e->getMacAddress();
        if (srcAddr.isUnspecified() || srcAddr.isBroadcast())
            throw cRuntimeError("Invalid source address!");
        nodeId = (unsigned long)srcAddr.getInt();

        // visualize MAC Address
        cDisplayString& dispStr = this->getParentModule()->getDisplayString();
        dispStr.setTagArg("tt", 0, srcAddr.str().c_str());
        dispStr.setTagArg("t", 0, srcAddr.str().c_str());
        // startup
        nodeStatus = dynamic_cast<NodeStatus *>(findContainingNode(this)->getSubmodule("status"));
//        if (isEnabled() && isNodeUp())
//            startSendingPingRequests();
    }
}

void ExternalApp::handleMessage(cMessage *msg)
{
    if (!isNodeUp()) {
        if (msg->isSelfMessage())
            throw cRuntimeError("Self message '%s' received when %s is down", msg->getName(), getComponentType()->getName());
        else {
            EV_WARN << "App is down, dropping '" << msg->getName() << "' message\n";
            delete msg;
            return;
        }
    }
    if (msg->isSelfMessage()) {
        if (msg->getKind() == TIMER_KIND)
            adapter->call_timerNotify(nodeId);
        else
            throw cRuntimeError("Unexpeced self message");
    }
    else {
        //SimplePayload* payload = check_and_cast<SimplePayload *>(msg);
        ExternalAppPayload* receivedMsg = check_and_cast<ExternalAppPayload*>(msg);
        processMsg(receivedMsg);
//        if(payload->getIsReply())
//            // process ping response
//            processPingResponse(payload);
//        else
//            processPingRequest(payload);
    }
}

void ExternalApp::processMsg(ExternalAppPayload* msg)
{
    adapter->call_receptionNotify(nodeId, msg->getOriginatorId());
    delete msg;
}

//void ExternalApp::refreshDisplay() const
//{
//    char buf[40];
//    sprintf(buf, "sent: %ld pks\nrcvd: %ld pks", sentCount, numPongs);
//    getDisplayString().setTagArg("t", 0, buf);
//}

bool ExternalApp::handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback)
{
    Enter_Method_Silent();
    if (dynamic_cast<NodeStartOperation *>(operation)) {
        if ((NodeStartOperation::Stage)stage == NodeStartOperation::STAGE_APPLICATION_LAYER && isEnabled())
            //startSendingPingRequests();
            throw cRuntimeError("implement handleOperationSatge in ExternalApp");
    }
    else if (dynamic_cast<NodeShutdownOperation *>(operation)) {
        if ((NodeShutdownOperation::Stage)stage == NodeShutdownOperation::STAGE_APPLICATION_LAYER)
            //stopSendingPingRequests();
            throw cRuntimeError("implement handleOperationSatge in ExternalApp");
    }
    else if (dynamic_cast<NodeCrashOperation *>(operation)) {
        if ((NodeCrashOperation::Stage)stage == NodeCrashOperation::STAGE_CRASH)
            //stopSendingPingRequests();
            throw cRuntimeError("implement handleOperationSatge in ExternalApp");
    }
    else
        throw cRuntimeError("Unsupported lifecycle operation '%s'", operation->getClassName());
    return true;
}

void ExternalApp::startSendingPingRequests()
{
    ASSERT(!timer->isScheduled());
    lastStart = simTime();
    sentCount = 0;
    sendSeqNo = 0;
}

void ExternalApp::stopSendingPingRequests()
{
    lastStart = -1;
    sendSeqNo = expectedReplySeqNo = 0;
    srcAddr = destAddr = MACAddress();
    destAddresses.clear();
    destAddrIdx = -1;
}

bool ExternalApp::isNodeUp()
{
    return !nodeStatus || nodeStatus->getState() == NodeStatus::UP;
}

bool ExternalApp::isEnabled()
{
    //return par("destAddr").stringValue()[0] && (count == -1 || sentCount < count);
    return true;
}

void ExternalApp::processPingResponse(SimplePayload *msg)
{
    if (msg->getOriginatorId() != nodeId) {
        EV_WARN << "Received response was not sent by this application, dropping packet\n";
        delete msg;
        return;
    }

    // get src, hopCount etc from packet, and print them
    SimpleLinkLayerControlInfo *ctrl = check_and_cast<SimpleLinkLayerControlInfo *>(msg->getControlInfo());
    MACAddress src = ctrl->getSourceAddress();
    //L3Address dest = ctrl->getDestinationAddress();

    // calculate the RTT time by looking up the the send time of the packet
    // if the send time is no longer available (i.e. the packet is very old and the
    // sendTime was overwritten in the circular buffer) then we just return a 0
    // to signal that this value should not be used during the RTT statistics)
    simtime_t rtt = sendSeqNo - msg->getSeqNo() > PING_HISTORY_SIZE ?
        0 : simTime() - sendTimeHistory[msg->getSeqNo() % PING_HISTORY_SIZE];

    if (printPing) {
        cout << getFullPath() << ": reply of " << std::dec << msg->getByteLength()
             << " bytes from " << src
             << " icmp_seq=" << msg->getSeqNo()
             << " time=" << (rtt * 1000) << " msec"
             << " (" << msg->getName() << ")" << endl;
    }

    // update statistics
    countPingResponse(msg->getByteLength(), msg->getSeqNo(), rtt);
    delete msg;
}

void ExternalApp::processPingRequest(SimplePayload *msg)
{
    // change addresses and send out reply
    SimpleLinkLayerControlInfo *ctrl = check_and_cast<SimpleLinkLayerControlInfo *>(msg->getControlInfo());
    //MACAddress src = ctrl->getSourceAddress();
    //MACAddress dest = ctrl->getDestinationAddress();
    ctrl->setDestinationAddress(MACAddress::BROADCAST_ADDRESS);
    ctrl->setSourceAddress(MACAddress::UNSPECIFIED_ADDRESS);
    msg->setName((std::string(msg->getName()) + "-reply").c_str());
    msg->setIsReply(true);

    send(msg, "appOut");
}

void ExternalApp::countPingResponse(int bytes, long seqNo, simtime_t rtt)
{
    EV_INFO << "Ping reply #" << seqNo << " arrived, rtt=" << rtt << "\n";
    emit(pingRxSeqSignal, seqNo);

    numPongs++;

    // count only non 0 RTT values as 0s are invalid
    if (rtt > 0) {
        rttStat.collect(rtt);
        emit(rttSignal, rtt);
    }

    if (seqNo == expectedReplySeqNo) {
        // expected ping reply arrived; expect next sequence number
        expectedReplySeqNo++;
    }
    else if (seqNo > expectedReplySeqNo) {
        EV_DETAIL << "Jump in seq numbers, assuming pings since #" << expectedReplySeqNo << " got lost\n";

        // jump in the sequence: count pings in gap as lost for now
        // (if they arrive later, we'll decrement back the loss counter)
        long jump = seqNo - expectedReplySeqNo;
        lossCount += jump;
        emit(numLostSignal, lossCount);

        // expect sequence numbers to continue from here
        expectedReplySeqNo = seqNo + 1;
    }
    else {    // seqNo < expectedReplySeqNo
              // ping reply arrived too late: count as out-of-order arrival (not loss after all)
        EV_DETAIL << "Arrived out of order (too late)\n";
        outOfOrderArrivalCount++;
        lossCount--;
        emit(numOutOfOrderArrivalsSignal, outOfOrderArrivalCount);
        emit(numLostSignal, lossCount);
    }
}

void ExternalApp::finish()
{
    if (sendSeqNo == 0) {
        if (printPing)
            EV_DETAIL << getFullPath() << ": No pings sent, skipping recording statistics and printing results.\n";
        return;
    }

    lossCount += sendSeqNo - expectedReplySeqNo;
    // record statistics
    recordScalar("Pings sent", sendSeqNo);
    recordScalar("ping loss rate (%)", 100 * lossCount / (double)sendSeqNo);
    recordScalar("ping out-of-order rate (%)", 100 * outOfOrderArrivalCount / (double)sendSeqNo);

    // print it to stdout as well
    if (printPing) {
        cout << "--------------------------------------------------------" << endl;
        cout << "\t" << getFullPath() << endl;
        cout << "--------------------------------------------------------" << endl;

        cout << "sent: " << sendSeqNo << "   received: " << numPongs << "   loss rate (%): " << (100 * lossCount / (double)sendSeqNo) << endl;
        cout << "round-trip min/avg/max (ms): " << (rttStat.getMin() * 1000.0) << "/"
             << (rttStat.getMean() * 1000.0) << "/" << (rttStat.getMax() * 1000.0) << endl;
        cout << "stddev (ms): " << (rttStat.getStddev() * 1000.0) << "   variance:" << rttStat.getVariance() << endl;
        cout << "--------------------------------------------------------" << endl;
    }
}

unsigned long ExternalApp::getNodeId()
{
    return nodeId;
}

void ExternalApp::sendPing()
{
    Enter_Method("sendPing");

    char name[32];
    sprintf(name, "ping%ld", sendSeqNo);

    SimplePayload *msg = new SimplePayload(name);
    //ASSERT(pid != -1);
    msg->setOriginatorId(nodeId);
    msg->setSeqNo(sendSeqNo);
    msg->setByteLength(packetSize + 4);

    // store the sending time in a circular buffer so we can compute RTT when the packet returns
    sendTimeHistory[sendSeqNo % PING_HISTORY_SIZE] = simTime();

    emit(pingTxSeqSignal, sendSeqNo);
    sendSeqNo++;
    sentCount++;
    SimpleLinkLayerControlInfo* controlInfo = new SimpleLinkLayerControlInfo;
    controlInfo->setSourceAddress(srcAddr);
    controlInfo->setDestinationAddress(MACAddress::BROADCAST_ADDRESS);

    msg->setControlInfo(dynamic_cast<cObject *>(controlInfo));
    EV_INFO << "Sending ping request #" << msg->getSeqNo() << " to lower layer.\n";
    send(msg, "appOut");
}

void ExternalApp::sendMsg(unsigned long dest, int numBytes)
{
    Enter_Method("send");

    char name[32];
    sprintf(name, "msg%ld", sendSeqNo);

    ExternalAppPayload *msg = new ExternalAppPayload(name);
    //ASSERT(pid != -1);
    msg->setOriginatorId(nodeId);
    msg->setDestinationId(dest);
    msg->setByteLength(numBytes);

    sendSeqNo++;
    sentCount++;

    SimpleLinkLayerControlInfo* controlInfo = new SimpleLinkLayerControlInfo;
    controlInfo->setSourceAddress(srcAddr);
    controlInfo->setDestinationAddress(MACAddress(dest));

    msg->setControlInfo(dynamic_cast<cObject *>(controlInfo));
    EV_INFO << "Sending message #" << sendSeqNo-1 << " to lower layer.\n";
    send(msg, "appOut");
}

void ExternalApp::wait(simtime_t duration)
{
    Enter_Method("wait");
    scheduleAt(simTime()+duration, timer);
}

} //namespace
