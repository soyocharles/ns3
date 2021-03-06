/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Jaume Nin <jaume.nin@cttc.cat>
 */

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
#include "src/csma/helper/csma-helper.h"
//#include "ns3/gtk-config-store.h"

using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */

NS_LOG_COMPONENT_DEFINE("EpcFirstExample");




int
main(int argc, char *argv[]) {

    uint16_t numberOfNodes = 1;
    double simTime = 1.1;
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
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
    lteHelper->SetEpcHelper(epcHelper);

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    // parse again so you can override default values from the command line
    cmd.Parse(argc, argv);

    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // Create a single RemoteHost
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);
    // Create the Internet
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    // interface 0 is localhost, 1 is the p2p device
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4> ());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    NodeContainer ueNodes;
    NodeContainer enbNodes;
    enbNodes.Create(numberOfNodes);
    ueNodes.Create(numberOfNodes);

    // Install Mobility Model
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    for (uint16_t i = 0; i < numberOfNodes; i++) {
        positionAlloc->Add(Vector(distance * i, 10, 0));
    }
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(enbNodes);
    mobility.Install(ueNodes);



    Ptr<ListPositionAllocator> positionAllocENB = CreateObject<ListPositionAllocator> ();
    for (uint16_t i = 0; i < numberOfNodes; i++) {
        positionAllocENB->Add(Vector(distance * i, -10, 0));
    }
    MobilityHelper mobilityENB;
    mobilityENB.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilityENB.SetPositionAllocator(positionAllocENB);
    mobilityENB.Install(enbNodes);

    // Install LTE Devices to the nodes
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

    // Install the IP stack on the UEs
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));

    // Assign IP address to UEs, and install applications
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u) {
        Ptr<Node> ueNode = ueNodes.Get(u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4> ());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    // Attach one UE per eNodeB
    for (uint16_t i = 0; i < numberOfNodes; i++) {
        lteHelper->Attach(ueLteDevs.Get(i), enbLteDevs.Get(i));
        // side effect: the default EPS bearer will be activated
    }

    NodeContainer csmac;
    csmac.Create(1);
    Ptr<Node> localhost = csmac.Get(0);


    NS_LOG_INFO("Create csma");
    PointToPointHelper p2p2;
    p2p2.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2p2.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2p2.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    csmac.Add(ueNodes.Get(0));
    NetDeviceContainer csmadevs = p2p2.Install(csmac);

    internet.Install(localhost);

    Ipv4AddressHelper localhost_to_ue;
    localhost_to_ue.SetBase("2.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer localhost_to_ue_interface = localhost_to_ue.Assign(csmadevs);
    Ipv4Address localhost_addr = localhost_to_ue_interface.GetAddress(0);



    Ptr<Ipv4StaticRouting> local_static_routing = ipv4RoutingHelper.GetStaticRouting(localhost->GetObject<Ipv4> ());
    local_static_routing->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);
    local_static_routing->AddNetworkRouteTo(Ipv4Address("1.0.0.0"), Ipv4Mask("255.0.0.0"), 1);






    // Install and start applications on UEs and remote host
    uint16_t dlPort = 1234;
    uint16_t ulPort = 2000;
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;

    PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
    PacketSinkHelper ulPacketSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), ulPort));
    serverApps.Add(dlPacketSinkHelper.Install(localhost));
    serverApps.Add(ulPacketSinkHelper.Install(remoteHost));

    UdpClientHelper dlClient(localhost_addr, dlPort);
    dlClient.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
    dlClient.SetAttribute("MaxPackets", UintegerValue(1000000));

    UdpClientHelper ulClient(remoteHostAddr, ulPort);
    ulClient.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
    ulClient.SetAttribute("MaxPackets", UintegerValue(1000000));


    clientApps.Add(dlClient.Install(remoteHost));
    clientApps.Add(ulClient.Install(localhost));

    serverApps.Start(Seconds(0.01));
    clientApps.Start(Seconds(0.01));
    lteHelper->EnableTraces();
    // Uncomment to enable PCAP tracing
    p2p2.EnablePcapAll("lena-epc-first");

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    /*GtkConfigStore config;
    config.ConfigureAttributes();*/

    Simulator::Destroy();
    return 0;

}

