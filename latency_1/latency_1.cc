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

//#include "ns3/gtk-config-store.h"
#include <time.h>

using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */

NS_LOG_COMPONENT_DEFINE ("latency");

struct result {
  uint64_t t1_packet = 0;
  double t1_latency =0;
  uint64_t t2_packet=0;
  double t2_latency=0;
} ;

result
runsim (uint32_t argc, int type)
{

    uint32_t numberOfNodes = argc;
    double simTime = 5;
//    double distance = 1000.0;
//    double interPacketInterval = 100;
    uint16_t at = type;


//    // Command line arguments
//    CommandLine cmd;
//    cmd.AddValue("numberOfNodes", "Number of eNodeBs + UE pairs", numberOfNodes);
//    cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
//    cmd.AddValue("distance", "Distance between eNBs [m]", distance);
//    cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
////    cmd.Parse(argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
    Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
    lteHelper->SetEpcHelper (epcHelper);
//    lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");

//    ConfigStore inputConfig;
//    inputConfig.ConfigureDefaults();

    // parse again so you can override default values from the command line
//    cmd.Parse(argc, argv);

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
    p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.0)));
    NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
    // interface 0 is localhost, 1 is the p2p device
//    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
    remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

    NodeContainer ueNodes;
    NodeContainer enbNodes;
    enbNodes.Create(6);
    ueNodes.Create(numberOfNodes);

    // Install Mobility Model
//    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
//    for (uint16_t i = 0; i < numberOfNodes; i++)
//      {
//        positionAlloc->Add (Vector(distance * i, 0, 0));
//      }
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
//    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                 "X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"),"Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
  
    mobility.Install(ueNodes);
    internet.Install (ueNodes);
    
    for (uint32_t u = 0; u < ueNodes.GetN (); ++u){
        uint32_t id = u;
        Ptr<MobilityModel> mob = ueNodes.Get(id)->GetObject<MobilityModel>();
//        Vector pos = mob->GetPosition ();	
//        std::cout <<"for node "<<id<<"  POS: x=" << pos.x << ", y=" << pos.y << std::endl;
    }

//    Ptr<ListPositionAllocator> positionAlloc1 = CreateObject<ListPositionAllocator> ();
//    positionAlloc1->Add (Vector(500, 500, 0));
    MobilityHelper mobility1;
    mobility1.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility1.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                 "X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"),"Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
      mobility1.Install(enbNodes);
    Ptr<MobilityModel> mob = enbNodes.Get(1)->GetObject<MobilityModel>();

//	Vector pos = mob->GetPosition ();	
//	std::cout <<"for node "<<0<<"  POS: x=" << pos.x << ", y=" << pos.y << std::endl;
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
    if(at == 0){
        lteHelper->Attach (ueLteDevs);
    }
    if(at == 1){
                    std::cout << "1"<< std::endl;

        uint32_t nb = enbNodes.GetN ();
        uint32_t nu = ueNodes.GetN ();
        uint32_t k = nu/nb;
                    std::cout << k<< std::endl;
        uint32_t counter  = 0;
        for (uint32_t b = 0; b < enbNodes.GetN (); ++b){
            for (uint32_t u = 0; u < k; ++u){
                            std::cout << counter<< std::endl;

                uint32_t index = b * nu/nb + u;
                lteHelper->Attach (ueLteDevs.Get(index), enbLteDevs.Get(b));
                counter ++;
            }
        }
            std::cout << "3"<< std::endl;

        if(counter < nu ){
                        std::cout << "2"<< std::endl;

            for (uint32_t u = counter; u < nu; ++u){
                lteHelper->Attach (ueLteDevs.Get(u), enbLteDevs.Get(u));
                
            }
        }


    }
    if(at == 2){
        NetDeviceContainer t1;
        for (uint32_t b = 0; b < 3; ++b){
            t1.Add(enbLteDevs.Get(b));
        }
        NetDeviceContainer t2;
        for (uint32_t b = 0; b < enbLteDevs.GetN() - 3; ++b){
            t2.Add(enbLteDevs.Get(b+3));
        }
        uint32_t u  = 0;
        for (; u < ueLteDevs.GetN()/2; ++u){
            lteHelper->AttachToClosestEnb(ueLteDevs.Get(u),t1);
        }
        for (; u < ueLteDevs.GetN(); ++u){
            lteHelper->AttachToClosestEnb(ueLteDevs.Get(u),enbLteDevs);
        }
    }

    uint16_t port = 3000;
    uint32_t np =10000;
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;
    std::vector< Ptr<UdpServerLatency> > servers;
    for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
      {
        Ptr<UdpClientLatency> c = CreateObject<UdpClientLatency> ();
        c->SetAttribute("MaxPackets",UintegerValue (np));
        c->SetAttribute("Interval",TimeValue(MilliSeconds (100)));
        Ipv4Address addr = ueIpIface.GetAddress(u,0);
        c->SetRemote(addr,port);
        c->Setimsi( ueNodes.Get(u)->GetId());

        remoteHost->AddApplication(c);
        clientApps.Add(c);
        
            

        Ptr<UdpServerLatency> s = CreateObject<UdpServerLatency> ();
        s->SetAttribute("Port",UintegerValue (port));
        s->Setimsi( ueNodes.Get(u)->GetId());

        serverApps.Add(s);
        ueNodes.Get(u)->AddApplication(s);   
        servers.push_back(s);
      }
    
    serverApps.Start (Seconds (1));
    clientApps.Start (Seconds (1.01));
    // Uncomment to enable PCAP tracing
//    p2ph.EnablePcapAll("latency");

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    /*GtkConfigStore config;
    config.ConfigureAttributes();*/

    Simulator::Destroy();
//    std::cout << "End"<< std::endl;
    result res;
    uint32_t u = 0;
    for (; u < ueNodes.GetN ()/2; ++u){
        Ptr<UdpServerLatency> s = servers.at(u);
        double t = s->GetAverageLatency();
        uint64_t p = s->GetReceivedTaged();
        if(p != 0){
            res.t1_latency = res.t1_latency + t;
            res.t1_packet = res.t1_packet + p;
        }
    }
    for (; u < ueNodes.GetN (); ++u){
        Ptr<UdpServerLatency> s = servers.at(u);
        double t = s->GetAverageLatency();
        uint64_t p = s->GetReceivedTaged();
                if(p != 0){
        res.t2_latency = res.t2_latency + t;
        res.t2_packet = res.t2_packet + p;
                }
    }
    return res;

}

int
main(int argc, char* argv[]) {
    std::cout<< time(0) << std::endl;
    std::cout<< (uint32_t)time(0) << std::endl;
    uint32_t number_of_user_from = 10;
    uint32_t number_of_user_to = 50;
    uint32_t number_of_user_step = 1;
    uint32_t number_of_it = 1;
    for (uint32_t u = number_of_user_from; u < number_of_user_to; u = u + number_of_user_step){
        result res0;
        for (uint32_t i = 0; i < number_of_it; ++i){
            RngSeedManager::SetSeed(time(0));    
            result temp = runsim(u,0);
            if (temp.t1_packet != 0 ){
            res0.t1_latency = res0.t1_latency + temp.t1_latency;
            res0.t1_packet = res0.t1_packet + temp.t1_packet; 
            }
            if (temp.t2_packet != 0 ){
            res0.t2_latency = res0.t2_latency + temp.t2_latency;
            res0.t2_packet = res0.t2_packet + temp.t2_packet; 
            }
        }
        std::cout << "End "<< u <<" "<< res0.t1_latency <<" "<< res0.t1_packet <<" "<< res0.t2_latency<<" " << res0.t2_packet
                <<" " 
                << " latency_1 " << res0.t1_latency/res0.t1_packet
                << " latency_2 " << res0.t2_latency/res0.t2_packet
                << std::endl;
    }   
        
    for (uint32_t u = number_of_user_from; u < number_of_user_to;  u = u + number_of_user_step){
        result res2;
        for (uint32_t i = 0; i < number_of_it; ++i){
            RngSeedManager::SetSeed(time(0));    
            result temp = runsim(u,2);
            if (temp.t1_packet != 0 ){
            res2.t1_latency = res2.t1_latency + temp.t1_latency;
            res2.t1_packet = res2.t1_packet + temp.t1_packet; 
            }
            if (temp.t2_packet != 0 ){
            res2.t2_latency = res2.t2_latency + temp.t2_latency;
            res2.t2_packet = res2.t2_packet + temp.t2_packet; 
            }        
        }
        std::cout << "End "<< u <<" "<< res2.t1_latency <<" "<< res2.t1_packet <<" "<< res2.t2_latency<<" " << res2.t2_packet
                <<" " 
                << " latency_1 " << res2.t1_latency/res2.t1_packet
                << " latency_2 " << res2.t2_latency/res2.t2_packet
                << std::endl;

    }
    return 0;
}
