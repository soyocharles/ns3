#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/tap-bridge-module.h"
#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/csma-helper.h"


using namespace ns3;


NS_LOG_COMPONENT_DEFINE("docker_test");

int
main(int argc, char* argv[]) {
    CommandLine cmd;
    cmd.Parse (argc, argv);
    
    GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
    GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
    double simulation_time = 10000;
    
    Ptr<LteHelper> LteHelper_A = CreateObject<LteHelper> ();
    Ptr<PointToPointEpcHelper> PointToPointEpcHelper_A = CreateObject<PointToPointEpcHelper> ();
    LteHelper_A->SetEpcHelper(PointToPointEpcHelper_A);
    
    CsmaHelper PointToPointHelper_remotehost_to_docker;
    CsmaHelper PointToPointHelper_localhost_to_docker;
    PointToPointHelper PointToPointHelper_remotehost_to_pgw;
    PointToPointHelper PointToPointHelper_localhost_to_ue;
    
    
    NodeContainer ue_nodes;
    ue_nodes.Create(1);
    
    NodeContainer enb_nodes;
    enb_nodes.Create(1);

    
    NodeContainer remotehost_and_docker_nodes;
    remotehost_and_docker_nodes.Create(2);
    
    NodeContainer localhost_and_docker_nodes;
    localhost_and_docker_nodes.Create(2);
    
    Ptr<Node> Node_remotehost = remotehost_and_docker_nodes.Get(0);
    Ptr<Node> Node_pgw = PointToPointEpcHelper_A->GetPgwNode();
    Ptr<Node> Node_localhost = localhost_and_docker_nodes.Get(0);
    Ptr<Node> Node_ue = ue_nodes.Get(0);
    Ptr<Node> Node_localdocker = localhost_and_docker_nodes.Get(1);
    Ptr<Node> Node_remotedocker = remotehost_and_docker_nodes.Get(1);


    NodeContainer remotehost_and_pgw_nodes;
    NodeContainer localhost_and_ue_nodes;
    remotehost_and_pgw_nodes.Add(Node_pgw);
    remotehost_and_pgw_nodes.Add(Node_remotehost);
    localhost_and_ue_nodes.Add(Node_localhost);
    localhost_and_ue_nodes.Add(Node_ue);
    
    PointToPointHelper_remotehost_to_pgw.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    PointToPointHelper_localhost_to_ue.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
                       
    PointToPointHelper_remotehost_to_pgw.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    PointToPointHelper_remotehost_to_docker.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    PointToPointHelper_localhost_to_ue.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    PointToPointHelper_localhost_to_docker.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));

  
    

    
    InternetStackHelper InternetStackHelper_A;
    InternetStackHelper_A.SetIpv6StackInstall(false);
    InternetStackHelper_A.Install(localhost_and_docker_nodes);
    InternetStackHelper_A.Install(remotehost_and_docker_nodes);
    InternetStackHelper_A.Install(ue_nodes);
    
    NetDeviceContainer NDs_remotehost_to_pgw = PointToPointHelper_remotehost_to_pgw.Install(remotehost_and_pgw_nodes);
    NetDeviceContainer NDs_remotehost_to_docker = PointToPointHelper_remotehost_to_docker.Install(remotehost_and_docker_nodes);
    NetDeviceContainer NDs_localhost_to_ue = PointToPointHelper_localhost_to_ue.Install(localhost_and_ue_nodes);
    NetDeviceContainer NDs_localhost_to_docker = PointToPointHelper_localhost_to_docker.Install(localhost_and_docker_nodes);
     
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

    positionAlloc->Add (Vector(30, 10, 0));
    positionAlloc->Add (Vector(-30,-10, 0));

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(enb_nodes);
    mobility.Install(ue_nodes);

    NetDeviceContainer enbLteDevs = LteHelper_A->InstallEnbDevice (enb_nodes);
    NetDeviceContainer ueLteDevs = LteHelper_A->InstallUeDevice (ue_nodes);
    
    
    PointToPointEpcHelper_A->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));

    Ipv4AddressHelper Ipv4AddressHelper_10; 
    Ipv4AddressHelper Ipv4AddressHelper_11;
    Ipv4AddressHelper Ipv4AddressHelper_20;
    Ipv4AddressHelper Ipv4AddressHelper_21;

    Ipv4AddressHelper_10.SetBase("10.0.0.0","255.0.0.0");
    Ipv4AddressHelper_11.SetBase("11.0.0.0","255.0.0.0");
    Ipv4AddressHelper_20.SetBase("20.0.0.0","255.0.0.0");
    Ipv4AddressHelper_21.SetBase("21.0.0.0","255.0.0.0");

    Ipv4AddressHelper_11.Assign(NDs_localhost_to_ue);
    Ipv4AddressHelper_10.Assign(NDs_localhost_to_docker);
    Ipv4AddressHelper_21.Assign(NDs_remotehost_to_pgw);
    Ipv4AddressHelper_20.Assign(NDs_remotehost_to_docker);
    
//    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  
    Ipv4StaticRoutingHelper Ipv4StaticRoutingHelper_A;
    Ptr<Ipv4StaticRouting> Ipv4StaticRouting_ue = Ipv4StaticRoutingHelper_A.GetStaticRouting(Node_ue->GetObject<Ipv4> ());
    Ptr<Ipv4StaticRouting> Ipv4StaticRouting_pgw = Ipv4StaticRoutingHelper_A.GetStaticRouting(Node_pgw->GetObject<Ipv4> ());

    Ptr<Ipv4StaticRouting> Ipv4StaticRouting_remotehost = Ipv4StaticRoutingHelper_A.GetStaticRouting(Node_remotehost->GetObject<Ipv4> ());
    Ptr<Ipv4StaticRouting> Ipv4StaticRouting_remotedocker = Ipv4StaticRoutingHelper_A.GetStaticRouting(Node_remotedocker->GetObject<Ipv4> ());
    Ptr<Ipv4StaticRouting> Ipv4StaticRouting_localhost = Ipv4StaticRoutingHelper_A.GetStaticRouting(Node_localhost->GetObject<Ipv4> ());
    Ptr<Ipv4StaticRouting> Ipv4StaticRouting_localdocker = Ipv4StaticRoutingHelper_A.GetStaticRouting(Node_localdocker->GetObject<Ipv4> ());

    Ipv4StaticRouting_remotehost->AddNetworkRouteTo(Ipv4Address("10.0.0.0"), Ipv4Mask("255.0.0.0"),1);
    Ipv4StaticRouting_remotehost->AddNetworkRouteTo(Ipv4Address("11.0.0.0"), Ipv4Mask("255.0.0.0"),1);
    Ipv4StaticRouting_remotehost->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"),1);
    
    Ipv4StaticRouting_remotehost->AddNetworkRouteTo(Ipv4Address("20.0.0.0"), Ipv4Mask("255.0.0.0"),2);

    
    Ipv4StaticRouting_localhost->AddNetworkRouteTo(Ipv4Address("20.0.0.0"), Ipv4Mask("255.0.0.0"),1);
    Ipv4StaticRouting_localhost->AddNetworkRouteTo(Ipv4Address("21.0.0.0"), Ipv4Mask("255.0.0.0"),1);
    Ipv4StaticRouting_localhost->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"),1);
    
    Ipv4StaticRouting_localhost->AddNetworkRouteTo(Ipv4Address("10.0.0.0"), Ipv4Mask("255.0.0.0"),2);

    
    Ipv4StaticRouting_remotedocker->AddNetworkRouteTo(Ipv4Address("21.0.0.0"), Ipv4Mask("255.0.0.0"),1);
    Ipv4StaticRouting_remotedocker->AddNetworkRouteTo(Ipv4Address("11.0.0.0"), Ipv4Mask("255.0.0.0"),1);
    Ipv4StaticRouting_remotedocker->AddNetworkRouteTo(Ipv4Address("10.0.0.0"), Ipv4Mask("255.0.0.0"),1);
    Ipv4StaticRouting_remotedocker->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"),1);

    Ipv4StaticRouting_localdocker->AddNetworkRouteTo(Ipv4Address("11.0.0.0"), Ipv4Mask("255.0.0.0"),1);     
    Ipv4StaticRouting_localdocker->AddNetworkRouteTo(Ipv4Address("21.0.0.0"), Ipv4Mask("255.0.0.0"),1);      
    Ipv4StaticRouting_localdocker->AddNetworkRouteTo(Ipv4Address("20.0.0.0"), Ipv4Mask("255.0.0.0"),1);     
    Ipv4StaticRouting_localdocker->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"),1);
//
    Ipv4StaticRouting_ue->AddNetworkRouteTo(Ipv4Address("20.0.0.0"), Ipv4Mask("255.0.0.0"),2);
    Ipv4StaticRouting_ue->AddNetworkRouteTo(Ipv4Address("10.0.0.0"), Ipv4Mask("255.0.0.0"),2);
    Ipv4StaticRouting_ue->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"),2);

    Ipv4StaticRouting_pgw->AddNetworkRouteTo(Ipv4Address("20.0.0.0"), Ipv4Mask("255.0.0.0"),3);

    LteHelper_A->Attach(ueLteDevs.Get(0), enbLteDevs.Get(0));

    PointToPointEpcHelper_A->GetUeDefaultGatewayAddress().Print(std::cout);
    std::cout << "pgw" << std::endl;
    
    std::cout << Node_remotedocker->GetNDevices();
    std::cout << "hello" << std::endl;
    
    std::cout << Node_ue->GetNDevices();
    std::cout << "ue" << std::endl;
    
    std::string mode = "ConfigureLocal";
    std::string tap_left = "tap-left";
    std::string tap_right = "tap-right";
  
    TapBridgeHelper tapBridge;
    tapBridge.SetAttribute ("Mode", StringValue (mode));
    tapBridge.SetAttribute ("DeviceName", StringValue (tap_right));
    tapBridge.Install (Node_remotedocker, NDs_remotehost_to_docker.Get (1));
    
    tapBridge.SetAttribute ("DeviceName", StringValue (tap_left));
    tapBridge.Install (Node_localdocker, NDs_localhost_to_docker.Get (1));


    Simulator::Stop(Seconds(simulation_time));
    Simulator::Run();
    
    Simulator::Destroy();

    
    
    return 0;
}
