
#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "src/lte/model/lte-enb-rrc.h"
#include "src/core/model/callback.h"
#include "src/lte/model/lte-enb-net-device.h"
#include "handover-udp.h"
#include "src/network/helper/application-container.h"
#include <functional>
//#include "ns3/gtk-config-store.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("EpcFirstExample");

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */


void printsomething(uint64_t imsi, uint16_t cellId, uint16_t rnti){
std::cout << "hiiiii " << imsi << "  "<< cellId << std::endl;
}



int
main (int argc, char *argv[])
{

  uint16_t numberOfNodes = 2;
  double simTime = 5;
  double distance = 60.0;
  double interPacketInterval = 100;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("numberOfNodes", "Number of eNodeBs + UE pairs", numberOfNodes);
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("distance", "Distance between eNBs [m]", distance);
  cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
  cmd.Parse(argc, argv);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

   // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(numberOfNodes);
  ueNodes.Create(numberOfNodes);

  // Install Mobility Model
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numberOfNodes; i++)
    {
      positionAlloc->Add (Vector(distance * i, 0, 0));
    }
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install(enbNodes);
  mobility.Install(ueNodes);
  internet.Install (ueNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  // Install the IP stack on the UEs
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach one UE per eNodeB
  for (uint16_t i = 0; i < numberOfNodes; i++)
      {
        lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(i));
        // side effect: the default EPS bearer will be activated
      }


  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;
  uint16_t otherPort = 3000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  uint32_t u = 0;

      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
      PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
      PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort));
      serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
      serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
      serverApps.Add (packetSinkHelper.Install (ueNodes.Get(u)));
      
      
      Ptr<HandoverUdpClient> HandoverUdpClient_A = CreateObject<HandoverUdpClient> ();
      Ptr<HandoverUdpClient> HandoverUdpClient_B = CreateObject<HandoverUdpClient> ();

      
      ueNodes.Get(u)->AddApplication(HandoverUdpClient_A);
      ueNodes.Get(u+1)->AddApplication(HandoverUdpClient_B);
      clientApps.Add(HandoverUdpClient_A);
      clientApps.Add(HandoverUdpClient_B);
      HandoverUdpClient_A->SetRemote(remoteHostAddr,ulPort);      
      HandoverUdpClient_B->SetRemote(remoteHostAddr,dlPort);      

      
      
      
//      HandoverUdpClient dlClient (ueIpIface.GetAddress (u), dlPort);
//
//
//      HandoverUdpClient ulClient (remoteHostAddr, ulPort);
//
//
//      HandoverUdpClient client (ueIpIface.GetAddress (u), otherPort);
//
//
//      clientApps.Add (dlClient.Install (remoteHost));
//      clientApps.Add (ulClient.Install (ueNodes.Get(u)));
//      if (u+1 < ueNodes.GetN ())
//        {
//          clientApps.Add (client.Install (ueNodes.Get(u+1)));
//        }
//      else
//        {
//          clientApps.Add (client.Install (ueNodes.Get(0)));
//        }
    
  serverApps.Start (Seconds (0.01));
  clientApps.Start (Seconds (0.01));
  lteHelper->EnableTraces ();
  Ptr<LteEnbRrc> LteEnbRrc_test1 = enbLteDevs.Get(0)->GetObject<LteEnbNetDevice>()->GetRrc();
  LteEnbRrc_test1->TraceConnectWithoutContext("HandoverEndOk", MakeCallback(&HandoverUdpClient::Send,HandoverUdpClient_A));
  LteEnbRrc_test1->TraceConnectWithoutContext("RecvMeasurementReport", MakeCallback(&HandoverUdpClient::Sendm,HandoverUdpClient_A));

  Ptr<LteEnbRrc> LteEnbRrc_test2 = enbLteDevs.Get(1)->GetObject<LteEnbNetDevice>()->GetRrc();
  LteEnbRrc_test2->TraceConnectWithoutContext("HandoverEndOk", MakeCallback(&HandoverUdpClient::Send,HandoverUdpClient_B));
  LteEnbRrc_test2->TraceConnectWithoutContext("RecvMeasurementReport", MakeCallback(&HandoverUdpClient::Sendm,HandoverUdpClient_B));
 // Add X2 inteface
  lteHelper->AddX2Interface (enbNodes);

  // X2-based Handover
  lteHelper->HandoverRequest (Seconds (2), ueLteDevs.Get (0), enbLteDevs.Get (0), enbLteDevs.Get (1));

  
  // Uncomment to enable PCAP tracing
  p2ph.EnablePcapAll("handovertrigger");

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  /*GtkConfigStore config;
  config.ConfigureAttributes();*/

  Simulator::Destroy();
  return 0;

}

