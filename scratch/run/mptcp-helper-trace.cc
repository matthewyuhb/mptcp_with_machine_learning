#include <sstream>

#include "mptcp-helper-system.h"
#include "mptcp-helper-router.h"
#include "mptcp-helper-trace.h"
#include "ns3/rl-data-interface.h"

#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/core-module.h"


#include "mptcp-helper-router.h"

#include "ns3/core-module.h"
#include "ns3/netanim-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"



namespace ns3{

void TraceMacRx(Ptr<OutputStreamWrapper> stream, Ptr<const Packet> packet)
{
  PppHeader pppHeader;
  Ipv4Header ipheader;
  TcpHeader tcpHeader;

  Ptr<Packet> copy = packet->Copy();
  copy->RemoveHeader(pppHeader);

  if (pppHeader.GetProtocol() == 0x0021)
  {
    copy->RemoveHeader(ipheader);

    if (ipheader.GetProtocol() == TcpL4Protocol::PROT_NUMBER)
    {
      copy->RemoveHeader(tcpHeader);

      MpTcpSubflowTag subflowTag;
      bool found = copy->PeekPacketTag(subflowTag);

      int subflowId = -1;
      if (found){
        subflowId =  subflowTag.GetSubflowId();
      }

      //Figure out if this is a FIN packet, or SYN packet
      bool isFin = tcpHeader.GetFlags() & TcpHeader::FIN;
      bool isSyn = tcpHeader.GetFlags() & TcpHeader::SYN;

      // this is the only difference with TraceMacTx   send connection
      (*stream->GetStream()) << Simulator::Now().GetNanoSeconds() << ",0,1,"
      << subflowId << ","
      << tcpHeader.GetSequenceNumber() << "," << tcpHeader.GetAckNumber()
      << "," << copy->GetSize() << "," << packet->GetSize()
      << "," << isSyn << "," << isFin << endl;
    }
  }
}

void TraceMacTx(Ptr<OutputStreamWrapper> stream, Ptr<const Packet> packet)
{
  PppHeader pppHeader;
  Ipv4Header ipheader;
  TcpHeader tcpHeader;

  Ptr<Packet> copy = packet->Copy();
  copy->RemoveHeader(pppHeader);

  if (pppHeader.GetProtocol() == 0x0021)
  {
    copy->RemoveHeader(ipheader);

    if (ipheader.GetProtocol() == TcpL4Protocol::PROT_NUMBER)
    {
      copy->RemoveHeader(tcpHeader);

      MpTcpSubflowTag subflowTag;
      bool found = copy->PeekPacketTag(subflowTag);

      int subflowId = -1;
      if (found){
        subflowId =  subflowTag.GetSubflowId();
      }

      //Figure out if this is a FIN packet, or SYN packet
      bool isFin = tcpHeader.GetFlags() & TcpHeader::FIN;
      bool isSyn = tcpHeader.GetFlags() & TcpHeader::SYN;

      // this is the only difference with TraceMacRx   send connection
      (*stream->GetStream()) << Simulator::Now().GetNanoSeconds() << ",1,1,"
      << subflowId << ","
      << tcpHeader.GetSequenceNumber() << "," << tcpHeader.GetAckNumber()
      << "," << copy->GetSize() << "," << packet->GetSize()
      << "," << isSyn << "," << isFin << endl;

    }
  }
}

void TraceQueueItemDrop(Ptr<OutputStreamWrapper> stream, Ptr<const QueueItem> item)
{
  Ptr<const Ipv4QueueDiscItem> qitem = StaticCast<const Ipv4QueueDiscItem>(item);
  Ipv4Header ipheader = qitem->GetHeader();
  TcpHeader tcpHeader;
  // std::cout << "1" << std::endl;
  if ((qitem->GetProtocol() == Ipv4L3Protocol::PROT_NUMBER)
      && (ipheader.GetProtocol() == TcpL4Protocol::PROT_NUMBER))
  {
    // std::cout << "2" << std::endl;
    Ptr<Packet> copy = item->GetPacket();
    copy->RemoveHeader(tcpHeader);
    // std::cout << "3" << std::endl;
    MpTcpSubflowTag subflowTag;
    bool found = copy->PeekPacketTag(subflowTag);

    int subflowId = -1;
    if (found){
      subflowId =  subflowTag.GetSubflowId();
    }

    //Figure out if this is a FIN packet, or SYN packet
    bool isFin = tcpHeader.GetFlags() & TcpHeader::FIN;
    bool isSyn = tcpHeader.GetFlags() & TcpHeader::SYN;
    // std::cout << "4" << std::endl;

    (*stream->GetStream()) << Simulator::Now().GetNanoSeconds()  << "," << subflowId << ","
    << tcpHeader.GetSequenceNumber() << "," << tcpHeader.GetAckNumber()
    << "," << isSyn << "," << isFin << endl;

  }
}

void ConfigureTracing (const string& outputDir, const NodeContainer& server,
                       const NodeContainer& client, const NodeContainer& middle,
                       const NodeContainer& other_servers, const NodeContainer& other_clients){
  // Create an output directory
  CheckAndCreateDirectory(outputDir);

  // Configure for clients
  stringstream devicePath;
  devicePath << "/NodeList/" << client.Get(0)->GetId() << "/DeviceList/*/$ns3::PointToPointNetDevice/"; // Hong Jiaming: Should change for Wi-Fi

  stringstream tfile;
  tfile << outputDir << "/mptcp_client";
  Ptr<OutputStreamWrapper> throughputFile = Create<OutputStreamWrapper>(tfile.str(), std::ios::out);
  *(throughputFile->GetStream()) << "timestamp,send,connection,subflow,seqno,ackno,size,psize,isSyn,isFin" << endl;

  Config::ConnectWithoutContext(devicePath.str() + "MacRx", MakeBoundCallback(TraceMacRx, throughputFile));
  Config::ConnectWithoutContext(devicePath.str() + "MacTx", MakeBoundCallback(TraceMacTx, throughputFile));

  // Hong Jiaming: move rtt recorder to python
  // stringstream clientRttFile;
  // clientRttFile << outputDir << "/mptcp_client_rtt";
  // Ptr<OutputStreamWrapper> clientRttFile = Create<OutputStreamWrapper>(clientRttFile.str(), std::ios::out);
  // *(clientRttFile->GetStream()) << "timestamp,subflow0RTT,subflow1RTT" << endl;
  // Config::ConnectWithoutContext(devicePath.str() + "MacTx", MakeBoundCallback(TraceRTT, clientRttFile, server.Get(0)));

  // configure for server
  uint32_t serverId = server.Get(0)->GetId();
  devicePath.str("");
  devicePath << "/NodeList/" << serverId << "/DeviceList/*/$ns3::PointToPointNetDevice/";

  stringstream sfile;
  sfile << outputDir << "/mptcp_server";
  Ptr<OutputStreamWrapper> serverFile = Create<OutputStreamWrapper>(sfile.str(), std::ios::out);
  *(serverFile->GetStream()) << "timestamp,send,connection,subflow,seqno,ackno,size,psize,isSyn,isFin" << endl;
  Config::ConnectWithoutContext(devicePath.str() + "MacTx", MakeBoundCallback(TraceMacTx, serverFile));
  Config::ConnectWithoutContext(devicePath.str() + "MacRx", MakeBoundCallback(TraceMacRx, serverFile));

  // Hong Jiaming: move rtt recorder to python
  // stringstream serverRttFile;
  // serverRttFile << outputDir << "/mptcp_server_rtt";
  // Ptr<OutputStreamWrapper> serverRttFile = Create<OutputStreamWrapper>(serverRttFile.str(), std::ios::out);
  // *(serverRttFile->GetStream()) << "timestamp,subflow0RTT,subflow1RTT" << endl;
  // Config::ConnectWithoutContext(devicePath.str() + "MacTx", MakeBoundCallback(TraceRTT, serverRttFile, server.Get(0)));

  // configure for droped packets
  stringstream dfile;
  dfile << outputDir << "/mptcp_drops";
  Ptr<OutputStreamWrapper> dropsFile = Create<OutputStreamWrapper>(dfile.str(), std::ios::out);
  *(dropsFile->GetStream()) << "timestamp,subflowId,seqno,ackno,isSyn,isFin" << endl;
  Config::ConnectWithoutContext("/NodeList/*/$ns3::TrafficControlLayer/RootQueueDiscList/*/Drop", MakeBoundCallback(TraceQueueItemDrop, dropsFile));
  // Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::QueueDisc/Drop", MakeBoundCallback(TraceQueueItemDrop, dropsFile));
  // Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::Queue/*/Drop",
  //                               MakeBoundCallback(TraceQueueItemDrop, dropsFile));

  uint32_t clientId = client.Get(0)->GetId();
  cout << "server node is: " << serverId << endl;
  cout << "client node is: " << clientId << endl;
  cout << "middle node are: ";
  for(int i = 0; i < middle.GetN(); ++i){
    cout << " " << middle.Get(i)->GetId();
  }
  cout << endl;
  cout << "other_servers node are: ";
  for(int i = 0; i < other_servers.GetN(); ++i){
    cout << " " << other_servers.Get(i)->GetId();
  }
  cout << endl;
  cout << "other_clients node are: ";
  for(int i = 0; i < other_clients.GetN(); ++i){
    cout << " " << other_clients.Get(i)->GetId();
  }
  cout << endl;

  // for(uint32_t i = 0; i < server.GetN(); ++i)
  // {
  //   PrintRoutingTable(server.Get(i), outputDir, "srv" + to_string(i));
  // }
  // for(uint32_t i = 0; i < client.GetN(); ++i)
  // {
  //   PrintRoutingTable(client.Get(i), outputDir, "cl" + to_string(i));
  // }
  // for (uint32_t  i = 0; i < middle.GetN(); ++i)
  // {
  //   PrintRoutingTable(middle.Get(i), outputDir, "middle" + to_string(i));
  // }
  // for (uint32_t  i = 0; i < other_servers.GetN(); ++i)
  // {
  //   PrintRoutingTable(other_servers.Get(i), outputDir, "other_servers" + to_string(i));
  // }
  // for (uint32_t  i = 0; i < other_clients.GetN(); ++i)
  // {
  //   PrintRoutingTable(other_clients.Get(i), outputDir, "other_clients" + to_string(i));
  // }
}

void TraceMonitorStates(const string& outputDir){
  //Create flow monitor
  static FlowMonitorHelper flowmon;
  static Ptr<FlowMonitor> monitor = flowmon.InstallAll();;
  static bool initialized = false;
  static Ptr<OutputStreamWrapper> logFile;

  if(!initialized){
    logFile = Create<OutputStreamWrapper>(outputDir + "/mptcp_monitor", std::ios::out);
    *(logFile->GetStream()) << "Timestamp,FlowId,From,To,TxPackets,TxBytes,RxPackets,RxBytes,DelaySum,JitterSum,LostPacketSum,TTL_expire,Bad_checksum" << endl;
    initialized = true;
  }

  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i){
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
    *(logFile->GetStream()) << Simulator::Now().GetNanoSeconds() << ","
                            << i->first << ","
                            << t.sourceAddress << ","
                            << t.destinationAddress << ","
                            << i->second.txPackets << ","
                            << i->second.txBytes << ","
                            << i->second.rxPackets << ","
                            << i->second.rxBytes << ","
                            << i->second.delaySum << ","
                            << i->second.jitterSum << ","
                            << i->second.lostPackets;
    if(i->second.packetsDropped.size() > Ipv4L3Protocol::DropReason::DROP_TTL_EXPIRED){
      *(logFile->GetStream()) << "," <<i->second.packetsDropped[Ipv4L3Protocol::DropReason::DROP_TTL_EXPIRED];
    }
    else{
      *(logFile->GetStream()) << ",0";
    }
    if(i->second.packetsDropped.size() > Ipv4L3Protocol::DropReason::DROP_BAD_CHECKSUM){
      *(logFile->GetStream()) << "," <<i->second.packetsDropped[Ipv4L3Protocol::DropReason::DROP_BAD_CHECKSUM];
    }
    else{
      *(logFile->GetStream()) << ",0";
    }
    *(logFile->GetStream()) << "\n";
    //std::cout << "Hong Jiaming: 3 " << i->first << endl;
  }
}

void PrintMonitorStates(void){
  //Create flow monitor
  static FlowMonitorHelper flowmon;
  static Ptr<FlowMonitor> monitor = flowmon.InstallAll();;

  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i){
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
    std::cout << "Timestamp: " << Simulator::Now().GetNanoSeconds() << ","
                    "FlowId: " << i->first << ","
                      "From: " << t.sourceAddress << ","
                        "To: " << t.destinationAddress << ","
                 "TxPackets: " << i->second.txPackets << ","
                   "TxBytes: " << i->second.txBytes << ","
                 "RxPackets: " << i->second.rxPackets << ","
                   "RxBytes: " << i->second.rxBytes << ","
                  "DelaySum: " << i->second.delaySum << ","
                 "JitterSum: " << i->second.jitterSum << ","
             "LostPacketSum: " << i->second.lostPackets;
    if(i->second.packetsDropped.size() > Ipv4L3Protocol::DropReason::DROP_TTL_EXPIRED){
      std::cout << ", TTL_expire: " <<i->second.packetsDropped[Ipv4L3Protocol::DropReason::DROP_TTL_EXPIRED];
    }
    if(i->second.packetsDropped.size() > Ipv4L3Protocol::DropReason::DROP_BAD_CHECKSUM){
      std::cout << ", Bad_checksum: " <<i->second.packetsDropped[Ipv4L3Protocol::DropReason::DROP_BAD_CHECKSUM];
    }
    std::cout << endl;
    //std::cout << "Hong Jiaming: 3 " << i->first << endl;
  }
  std::cout << endl;
}

// Hong Jiaming: This function may be a potential breaking point (or problematic point). 
// This queue means the TxQueue of netDevice. And I In fact somehow more interested in QueueDisc Length (in case of pfifo)
// This means I should modify this future. 
void TraceQueueLength(const string& outputDir, const NetDeviceContainer devs){
  static bool initialized = false;
  static Ptr<OutputStreamWrapper> logFile;
  static Ptr<DropTailQueue> droptail;
  static Ptr<DropTailQueue> droptail1;

  if(!initialized){
    logFile = Create<OutputStreamWrapper>(outputDir + "/routers_queue_len", std::ios::out);
    *(logFile->GetStream()) << "Timestamp,Queue0,Queue1,Queue0Dropped,Queue1Dropped" << endl;
    initialized = true;
    PointerValue val;
    devs.Get(0)->GetAttribute ("TxQueue", val);
    droptail = val.Get<DropTailQueue> ();
    devs.Get(1)->GetAttribute ("TxQueue", val);
    droptail1 = val.Get<DropTailQueue> ();
  }

  *(logFile->GetStream()) << Simulator::Now().GetNanoSeconds() << ","
                          << droptail->GetNPackets() << "," << droptail1->GetNPackets() << ","
                          << droptail->GetTotalDroppedPackets() << "," << droptail1->GetTotalDroppedPackets() << "\n";

  // for(uint32_t index = 0; index < devs.GetN(); index++){
  //   PointerValue txQueue;
  //   devs.Get(index)->GetAttribute ("TxQueue", txQueue);
  //   *(logFile->GetStream()) << txQueue.Get<DropTailQueue> ()->GetNPackets() << ", ";
  // }
}

};
