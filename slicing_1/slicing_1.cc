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
#include <math.h>
#include <iostream>
#include <fstream>
#define PI 3.14159265354
using namespace ns3;
/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */

NS_LOG_COMPONENT_DEFINE ("latency");

struct data_input{
    double sim_time;
    int seed;
    int num_ue_1;
    int num_ue_2;
    int num_enb;
    int ss;
    int num_block;
    double area = 100;
};





void
runsim (data_input input)
{
    int seed = input.seed;
    int num_ue_1 = input.num_ue_1;
    int num_ue_2 = input.num_ue_2;
    int num_enb = input.num_enb;
    double sim_time = input.sim_time;
    int ss = input.ss;
    int num_block = input.num_block;
    double area = input.area;
    
    Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
    Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
    epcHelper->SetAttribute("S1uLinkDataRate", DataRateValue (DataRate ("1000Mb/s")));
    lteHelper->SetEpcHelper (epcHelper);

    std::ostringstream schname;
    if (ss == 4){
        schname << "SlPfFfMacScheduler";
        std::cout <<"ns3::SlPfFfMacScheduler"<< std::endl;
        lteHelper->SetSchedulerType ("ns3::SlPfFfMacScheduler"); 
        std::cout <<lteHelper->GetSchedulerType()<< std::endl;
    }   
    if (ss == 3){
        schname << "SpfRrFfMacScheduler";
        std::cout <<"ns3::SpfRrFfMacScheduler"<< std::endl;
        lteHelper->SetSchedulerType ("ns3::SpfRrFfMacScheduler"); 
        std::cout <<lteHelper->GetSchedulerType()<< std::endl;
    }   
    if (ss == 1){
        schname << "PfFfMacScheduler";
        std::cout <<"ns3::PfFfMacScheduler"<< std::endl;
        lteHelper->SetSchedulerType ("ns3::PfFfMacScheduler"); 
        std::cout <<lteHelper->GetSchedulerType()<< std::endl;
    }
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

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
    remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

    NodeContainer ueNodes_1;
    NodeContainer ueNodes_2;
    NodeContainer enbNodes;

    ueNodes_1.Create(num_ue_1);
    ueNodes_2.Create(num_ue_2);
    enbNodes.Create(num_enb); 
    
    std::cout <<"Mobility and Position  setup"<< std::endl;

    Ptr<UniformRandomVariable> uni_rand = CreateObject<UniformRandomVariable> ();

    Ptr<ListPositionAllocator> position_ue_1 = CreateObject<ListPositionAllocator> ();
    Ptr<ListPositionAllocator> position_ue_2 = CreateObject<ListPositionAllocator> ();
    Ptr<ListPositionAllocator> position_enb = CreateObject<ListPositionAllocator> ();
    
    for (uint16_t i = 0; i < num_ue_1; i++)
    {
        position_ue_1->Add (Vector(uni_rand->GetValue(-area,area), uni_rand->GetValue(-area,area), 0));
    }
    for (uint16_t i = 0; i < num_ue_2; i++)
    {
        position_ue_2->Add (Vector(uni_rand->GetValue(-area,area), uni_rand->GetValue(-area,area), 0));
    }
    for (uint16_t i = 0; i < num_enb; i++)
    {
        position_enb->Add (Vector(uni_rand->GetValue(-area,area), uni_rand->GetValue(-area,area), 0));
    }
    
    
    MobilityHelper  mob_ue_1;     
                    mob_ue_1.SetMobilityModel("ns3::ConstantPositionMobilityModel");
                    mob_ue_1.SetPositionAllocator(position_ue_1);

    MobilityHelper  mob_ue_2;         
                    mob_ue_2.SetMobilityModel("ns3::ConstantPositionMobilityModel");
                    mob_ue_2.SetPositionAllocator(position_ue_2);
    
    MobilityHelper  mob_enb; 
                    mob_enb.SetMobilityModel("ns3::ConstantPositionMobilityModel");
                    mob_enb.SetPositionAllocator(position_enb);

    std::cout <<"Mobility Install UE 1"<< std::endl;
    mob_ue_1.Install(ueNodes_1);
    std::cout <<"Mobility Install UE 2"<< std::endl;
    mob_ue_2.Install(ueNodes_2);
    std::cout <<"Mobility Install  enb"<< std::endl;
    mob_enb.Install(enbNodes);


    std::cout <<"Position check"<< std::endl;

    for (uint32_t u = 0; u < ueNodes_1.GetN (); ++u){
        uint32_t id = u;
        Ptr<MobilityModel> mob = ueNodes_1.Get(id)->GetObject<MobilityModel>();
        Vector pos = mob->GetPosition ();	
        std::cout <<"for ue_1 node " << u <<" POS: x=" << pos.x << ", y=" << pos.y << std::endl;
    }
    
    for (uint32_t u = 0; u < ueNodes_2.GetN (); ++u){
        uint32_t id = u;
        Ptr<MobilityModel> mob = ueNodes_2.Get(id)->GetObject<MobilityModel>();
        Vector pos = mob->GetPosition ();	
        std::cout <<"for ue_2 node " << u <<" POS: x=" << pos.x << ", y=" << pos.y << std::endl;
    }
    for (uint32_t u = 0; u < enbNodes.GetN (); ++u){
        uint32_t id = u;
        Ptr<MobilityModel> mob = enbNodes.Get(id)->GetObject<MobilityModel>();
        Vector pos = mob->GetPosition ();	
        std::cout <<"for enb node " << u <<" POS: x=" << pos.x << ", y=" << pos.y << std::endl;
    }
    
    std::cout <<"LTE setup and attach  setup"<< std::endl;

    internet.Install (ueNodes_1);
    internet.Install (ueNodes_2);
    
    NodeContainer ueNodes_total = NodeContainer(ueNodes_1,ueNodes_2);

    // Install LTE Devices to the nodes
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
    NetDeviceContainer ueLteDevs_1 = lteHelper->InstallUeDevice (ueNodes_1);
    NetDeviceContainer ueLteDevs_2 = lteHelper->InstallUeDevice (ueNodes_2);

    // Install the IP stack on the UEs
    Ipv4InterfaceContainer ueIpIface_1;
    Ipv4InterfaceContainer ueIpIface_2;

    ueIpIface_1 = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs_1));
    ueIpIface_2 = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs_2));

    // Assign IP address to UEs, and install applications
    for (uint32_t u = 0; u < ueNodes_total.GetN (); ++u)
      {
        Ptr<Node> ueNode = ueNodes_total.Get (u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
        ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
        
      }

    // Attach one UE per eNodeB
    lteHelper->AttachToClosestEnb (ueLteDevs_1,enbLteDevs );
    lteHelper->AttachToClosestEnb (ueLteDevs_2,enbLteDevs );

    

    uint16_t port = 3000;
    uint32_t np =10000;
    uint32_t inter =1500;
    std::cout<< inter << std::endl;
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;
    std::vector< Ptr<UdpServerLatency> > servers_1;
    for (uint32_t u = 0; u < ueNodes_1.GetN (); ++u)
    {
        Ptr<UdpClientLatency> c = CreateObject<UdpClientLatency> ();
        c->SetAttribute("MaxPackets",UintegerValue (np));
        c->SetAttribute("Interval",TimeValue(MicroSeconds (inter)));
        c->SetAttribute("PacketSize",UintegerValue (300));

        Ipv4Address addr = ueIpIface_1.GetAddress(u,0);
        c->SetRemote(addr,port);
        c->Setimsi( ueNodes_1.Get(u)->GetId());

        remoteHost->AddApplication(c);
        clientApps.Add(c);      

        Ptr<UdpServerLatency> s = CreateObject<UdpServerLatency> ();
        s->SetAttribute("Port",UintegerValue (port));
        s->Setimsi( ueNodes_1.Get(u)->GetId());

        serverApps.Add(s);
        ueNodes_1.Get(u)->AddApplication(s);   
        servers_1.push_back(s);
    }
    
    std::vector< Ptr<UdpServerLatency> > servers_2;
    for (uint32_t u = 0; u < ueNodes_2.GetN (); ++u)
    {
        Ptr<UdpClientLatency> c = CreateObject<UdpClientLatency> ();
        c->SetAttribute("MaxPackets",UintegerValue (np));
        c->SetAttribute("Interval",TimeValue(MicroSeconds (inter)));
        c->SetAttribute("PacketSize",UintegerValue (300));

        Ipv4Address addr = ueIpIface_2.GetAddress(u,0);
        c->SetRemote(addr,port);
        c->Setimsi( ueNodes_2.Get(u)->GetId());

        remoteHost->AddApplication(c);
        clientApps.Add(c);      

        Ptr<UdpServerLatency> s = CreateObject<UdpServerLatency> ();
        s->SetAttribute("Port",UintegerValue (port));
        s->Setimsi( ueNodes_2.Get(u)->GetId());

        serverApps.Add(s);
        ueNodes_2.Get(u)->AddApplication(s);   
        servers_2.push_back(s);
    }
    
    ////////////////a  ss
    if(ss == 4){
        
        std::cout <<"ns3::SlPfFfMacScheduler  settup"<< std::endl;
        std::cout <<"Number of UE 1 = " << ueNodes_1.GetN () << std::endl;
        std::cout <<"Number of UE 2 = " << ueNodes_2.GetN () << std::endl;


        std::map<uint64_t, uint8_t > sliceuemap;
        std::map<uint8_t, std::pair<uint8_t,uint8_t> > slicerbmap;
        
        for (uint32_t u = 0; u < ueNodes_1.GetN (); ++u){
            Ptr<LteUeNetDevice> dev = ueNodes_1.Get(u)->GetDevice(1)->GetObject<LteUeNetDevice>();
            std::cout <<"ns3::SlPfFfMacScheduler  settup slice 1  "  <<  dev->GetImsi() << std::endl;
            sliceuemap.insert(std::pair<uint64_t, uint8_t >(dev->GetImsi(), 1));
        }
        
            for (uint32_t u = 0; u < ueNodes_2.GetN (); ++u){
            Ptr<LteUeNetDevice> dev = ueNodes_2.Get(u)->GetDevice(1)->GetObject<LteUeNetDevice>();
            std::cout <<"ns3::SlPfFfMacScheduler  settup slice 2  "  <<  dev->GetImsi() << std::endl;
            sliceuemap.insert(std::pair<uint64_t, uint8_t >(dev->GetImsi(), 2));
        }  
        
        slicerbmap.insert(std::pair<uint8_t, std::pair<uint8_t,uint8_t> >(1 ,std::pair<uint8_t,uint8_t>(0,num_block-1) ));
        slicerbmap.insert(std::pair<uint8_t, std::pair<uint8_t,uint8_t> >(2 ,std::pair<uint8_t,uint8_t>(num_block,25) ));

        for (uint32_t u = 0; u < enbLteDevs.GetN (); ++u){
            uint16_t id = enbLteDevs.Get(u)->GetObject<LteEnbNetDevice>()->GetCellId();
            id = id  + 1;
            Ptr<SlPfFfMacScheduler> sched;
            PointerValue tmp;
            enbLteDevs.Get(u)->GetObject<LteEnbNetDevice>()->GetAttribute("FfMacScheduler",tmp);
            Ptr<Object> temp = tmp.GetObject();
            sched = temp->GetObject<SlPfFfMacScheduler>();
            Ptr<LteEnbRrc> rrc = enbLteDevs.Get(u)->GetObject<LteEnbNetDevice>()->GetRrc();
            sched->SetEnbRrc(rrc);
            sched->SetSliceUeMap(sliceuemap);
            sched->SetSliceRbMap(slicerbmap);
        }
    }
    
    serverApps.Start (Seconds (0.4));
    clientApps.Start (Seconds (0.6));

    Simulator::Stop(Seconds(sim_time));
    Simulator::Run();
    
    double t_1= 0;
    uint64_t p_1 = 0;
    for (uint32_t u = 0; u < ueNodes_1.GetN (); ++u){
        int ueimsi  = ueNodes_1.Get(u)->GetDevice(1)->GetObject<LteUeNetDevice>()->GetImsi();
        Ptr<UdpServerLatency> s = servers_1.at(u);
        double t = s->GetAverageLatency();
        uint64_t p = s->GetReceivedTaged();
        t_1 += t;
        p_1 += p;
        if(p != 0){
            std::cout << "Tenant 1 imsi " << ueimsi << " latency "<< t/p << " n_packet " << p << std::endl;
        }
        if(p == 0){
            std::cout << "Tenant 1 imsi " << ueimsi << " No Packet "<< std::endl;
        }
    }
    
    double t_2= 0;
    uint64_t p_2 = 0;
    for (uint32_t u = 0; u < ueNodes_2.GetN (); ++u){
        int ueimsi  = ueNodes_2.Get(u)->GetDevice(1)->GetObject<LteUeNetDevice>()->GetImsi();
        Ptr<UdpServerLatency> s = servers_2.at(u);
        double t = s->GetAverageLatency();
        uint64_t p = s->GetReceivedTaged();
        t_2 += t;
        p_2 += p;
        if(p != 0){
            std::cout << "Tenant 2 imsi " << ueimsi << " latency "<< t/p << " n_packet " << p << std::endl;
        }
        if(p == 0){
            std::cout << "Tenant 2 imsi " << ueimsi << " No Packet "<< std::endl;
        }
    }
    
    
    std::cout << "Tenant 1 " <<  " latency "<< t_1/p_1 << " n_packet " << p_1 << std::endl;
    std::cout << "Tenant 2 " <<  " latency "<< t_2/p_2 << " n_packet " << p_2 << std::endl;

    std::ofstream outfile;
    for (uint32_t u = 0; u < ueNodes_1.GetN (); ++u){
        std::ostringstream filename;
        int ueimsi  = ueNodes_1.Get(u)->GetDevice(1)->GetObject<LteUeNetDevice>()->GetImsi();
        filename <<"./share/" <<schname.str()<< "_" << seed  << "_" << ueimsi << "_" << num_block <<".csv";
        outfile.open(filename.str(), std::ofstream::out | std::ofstream::trunc);
        std::vector< std::pair<uint32_t, std::pair<double,double>> >::iterator it;
        Ptr<UdpServerLatency> s = servers_1.at(u);
        for(it = s->m_data.begin();it != s->m_data.end(); it++){
            outfile<< (*it).first <<"," <<(*it).second.first<<","<<(*it).second.second<<std::endl; 
        }
        outfile.close();
    }
    Simulator::Destroy();
    return;
}

int
main(int argc, char* argv[]) {
    int num_seed = 3;
    for(int seed = 2; seed < num_seed ; seed++){
        std::cout << "this loop's seed = "<< seed << std::endl;
        RngSeedManager::reset();
        RngSeedManager::SetSeed(10);    
        RngSeedManager::SetRun(3);  
        data_input input;
        input.num_block = 16;
        std::cout<< "simulation number of block is+++++ " << input.num_block << std::endl;

        input.num_enb = 4;
        input.num_ue_1 = 40;
        input.num_ue_2 = 40;
        
        input.seed = 4;
        input.sim_time = 4;
        input.ss = 4;
        input.area = 500;
            
        runsim(input);
    }

    std::cout<< "end of the simulation" << std::endl;
    return 0;
}
