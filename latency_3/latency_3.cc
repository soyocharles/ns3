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
static bool ho = true;
/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */

NS_LOG_COMPONENT_DEFINE ("latency");

struct result{
  uint64_t  t1_packet   = 0;
  double    t1_latency  = 0;
  uint64_t  t2_packet   = 0;
  double    t2_latency  = 0;
};

void printsomething(uint64_t imsi, uint16_t cellId, uint16_t rnti){
    ho = true;
}
        
void 
handover(Ptr<NetDevice> ueDevice, NetDeviceContainer enbDevices, Ptr<NetDevice> nowit, Ptr<LteHelper>  lteHelper){
    Vector uepos = ueDevice->GetNode ()->GetObject<MobilityModel> ()->GetPosition ();
    double minDistance = std::numeric_limits<double>::infinity ();
    double NowDistance = std::numeric_limits<double>::infinity ();

    Ptr<NetDevice> closestEnbDevice;
    for (NetDeviceContainer::Iterator i = enbDevices.Begin (); i != enbDevices.End (); ++i)
    {

        Vector enbpos = (*i)->GetNode ()->GetObject<MobilityModel> ()->GetPosition ();
        double distance = CalculateDistance (uepos, enbpos);
        if ((*i)->GetObject<LteEnbNetDevice>()->GetCellId() ==nowit->GetObject<LteEnbNetDevice>()->GetCellId()  ){
            NowDistance  = distance;
            continue;
        }
        
        if (distance < minDistance)
        {
            minDistance = distance;
            closestEnbDevice = *i;
        }
    }
    
    if(closestEnbDevice&& ho == true){
        ho = false;
        std::map<uint16_t, Ptr<UeManager> > md =  closestEnbDevice->GetObject<LteEnbNetDevice>()->GetRrc()->GetUeManagerAll();
        std::map<uint16_t, Ptr<UeManager> > ms =  nowit->GetObject<LteEnbNetDevice>()->GetRrc()->GetUeManagerAll();

        if (md.size() < ms.size()*1/3  && minDistance < NowDistance + 50){
            lteHelper->HandoverRequest(MilliSeconds(0), ueDevice, nowit ,closestEnbDevice );
        }
    }
}

void 
assosiation (NetDeviceContainer ueDevices, NetDeviceContainer enbDevices, Ptr<LteHelper> lteHelper){
    if (ho == false){
        return;
    }
    
    for (NetDeviceContainer::Iterator i = enbDevices.Begin (); i != enbDevices.End (); ++i)
    {
        std::map<uint16_t, Ptr<UeManager> > m =  (*i)->GetObject<LteEnbNetDevice>()->GetRrc()->GetUeManagerAll();
        if (m.size() > (ueDevices.GetN()/enbDevices.GetN() +1 )){
            int u1 = 0;
            int u2 = 0;
            
            for (uint32_t u = ueDevices.GetN ()/2; u < ueDevices.GetN (); ++u){
                uint16_t rnti = ueDevices.Get(u)->GetObject<LteUeNetDevice>()->GetRrc()->GetRnti();
                uint64_t imsi = ueDevices.Get(u)->GetObject<LteUeNetDevice>()->GetRrc()->GetImsi();
                std::map<uint16_t, Ptr<UeManager> >::iterator it = m.find (rnti);
                if(it != m.end() && it->second->GetImsi() == imsi ){
                    u2++;
                }         
            }
            for (uint32_t u = 0; u < ueDevices.GetN ()/2 +1; ++u){
                uint16_t rnti = ueDevices.Get(u)->GetObject<LteUeNetDevice>()->GetRrc()->GetRnti();
                uint64_t imsi = ueDevices.Get(u)->GetObject<LteUeNetDevice>()->GetRrc()->GetImsi();
                std::map<uint16_t, Ptr<UeManager> >::iterator it = m.find (rnti);
                if(it != m.end() && it->second->GetImsi() == imsi ){
                    u1++;
                }         
            }
            if (u1>u2){
                for (uint32_t u = 0; u < ueDevices.GetN ()/2 +1; ++u){
                    uint16_t rnti = ueDevices.Get(u)->GetObject<LteUeNetDevice>()->GetRrc()->GetRnti();
                    uint64_t imsi = ueDevices.Get(u)->GetObject<LteUeNetDevice>()->GetRrc()->GetImsi();
                    std::map<uint16_t, Ptr<UeManager> >::iterator it = m.find (rnti);
                    if(it != m.end() && it->second->GetImsi() == imsi ){
                        handover(ueDevices.Get(u),enbDevices,*i,lteHelper);
                        return;
                    }
                }
            }
        }
    }
    return;
}


result
runsim (uint32_t argc, int type)
{

    uint32_t numberOfNodes = argc;
    double simTime = 2;
    uint16_t at = type;


    Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
    Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
    epcHelper->SetAttribute("S1uLinkDataRate", DataRateValue (DataRate ("1000Mb/s")));
    lteHelper->SetEpcHelper (epcHelper);
    
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
    enbNodes.Create(5);
    ueNodes.Create(numberOfNodes);


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

    }
    MobilityHelper mobility1;
    mobility1.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility1.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                 "X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"),"Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
    mobility1.Install(enbNodes);
    Ptr<MobilityModel> mob = enbNodes.Get(1)->GetObject<MobilityModel>();

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
        
//        Vector pos = mob->GetPosition ();	
//        std::cout <<"for node "<<"  POS: x=" << pos.x << ", y=" << pos.y << std::endl;
      }

    // Attach one UE per eNodeB
    if(at == 0){
        lteHelper->AttachToClosestEnb (ueLteDevs,enbLteDevs );
    }
    if(at == 1){
        std::cout << "1" << std::endl;
    }

    if(at == 2){
        for (uint32_t b = 0; b < enbLteDevs.GetN() ; b++){
            enbLteDevs.Get(b)->GetObject<LteEnbNetDevice>()->GetRrc()->TraceConnectWithoutContext("HandoverEndOk", MakeCallback(&printsomething));

        }
        
        lteHelper->AddX2Interface (enbNodes);
        lteHelper->AttachToClosestEnb (ueLteDevs,enbLteDevs );
        int numit = 500;
        for (int i = 2 ; i< numit ; i += 30){
            Simulator::Schedule (MilliSeconds(i),*assosiation, ueLteDevs,enbLteDevs,lteHelper);
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
        c->SetAttribute("Interval",TimeValue(MilliSeconds (10)));
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
    for (uint32_t b = 0; b < enbLteDevs.GetN() ; b++){
        Ptr<EpcEnbApplication> p = enbNodes.Get(b)->GetApplication(0)->GetObject<EpcEnbApplication>();
        p->S1uMeasure.GetMeasurementName();
//        std::cout<< p->S1uMeasure.GetTotalTagedLatency() << " "<<p->S1uMeasure.GetReceivedTaged() << std::endl;
    }
    Simulator::Destroy();
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
    uint32_t seed = time(0);
    std::cout << seed << std::endl;
    uint32_t number_of_user_from = 20;
    uint32_t number_of_user_to = 30;
    uint32_t number_of_user_step = 2;
    uint32_t number_of_it = 1;
    for (uint32_t u = number_of_user_from; u < number_of_user_to; u = u + number_of_user_step){
        result res0;
        for (uint32_t i = 0; i < number_of_it; ++i){
            RngSeedManager::SetSeed(seed);    
            RngSeedManager::SetRun(1);  
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
        std::cout << 0 <<" End "<< u <<" "<< res0.t1_latency <<" "<< res0.t1_packet <<" "<< res0.t2_latency<<" " << res0.t2_packet
                <<" " 
                << " latency_1 " << res0.t1_latency/res0.t1_packet
                << " latency_2 " << res0.t2_latency/res0.t2_packet
                << std::endl;
        result res2;
        for (uint32_t i = 0; i < number_of_it; ++i){
            RngSeedManager::reset();
            RngSeedManager::SetSeed(seed);    
            RngSeedManager::SetRun(1);  

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
        std::cout << 2 <<" End "<< u <<" "<< res2.t1_latency <<" "<< res2.t1_packet <<" "<< res2.t2_latency<<" " << res2.t2_packet
                <<" " 
                << " latency_1 " << res2.t1_latency/res2.t1_packet
                << " latency_2 " << res2.t2_latency/res2.t2_packet
                << std::endl;
    }   
    return 0;
}
