#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/applications-module.h"
#include "ns3/stats-module.h"
#include "ns3/callback.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/csma-module.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Offline");

class MyApp : public Application{
    public:
        Ptr<Socket> _socket;
        Address     _peer;
        uint32_t    _packetSize;
        DataRate    _dataRate;
        EventId     _sendEvent;
        bool        _isRunning;
        uint32_t    _numberOfPacketsSent;
        uint32_t    _simulationTime;

        MyApp(){
            _socket = 0;
            _packetSize = 0;
            _dataRate = 0;
            _isRunning = false;
            _numberOfPacketsSent = 0;
            _simulationTime = 0;
        }
        ~MyApp(){
            _socket=0;
        }

        static TypeId GetTypeId(void){
            static TypeId tid = TypeId("MyApp").SetParent<Application>().SetGroupName("Tutorial").AddConstructor<MyApp>();

            return tid;
        }

        void Setup(Ptr<Socket> socket,Address address, uint32_t packetSize, DataRate dataRate, uint32_t simulationTime){
            _socket = socket;
            _peer = address;
            _packetSize = packetSize;
            _dataRate = dataRate;
            _simulationTime = simulationTime;
        }
        void ScheduleTx(void){
            if(_isRunning){
                Time timeOfNext (Seconds(_packetSize * 8 / static_cast<double>(_dataRate.GetBitRate())));
                _sendEvent = Simulator::Schedule(timeOfNext, &MyApp::SendPacket, this);
            }
        }
        void SendPacket(void){
            Ptr<Packet> packet = Create<Packet> (_packetSize);
            _socket->Send(packet);

            if(Simulator::Now().GetSeconds() < _simulationTime) ScheduleTx();
        }
        virtual void StartApplication(void){
            _isRunning = true;
            _numberOfPacketsSent = 0;

            if(InetSocketAddress::IsMatchingType(_peer)){
                _socket->Bind();
            }else{
                _socket->Bind6();
            }
            _socket->Connect(_peer);
            SendPacket();
        }

        virtual void StopApplication(void){
            _isRunning = false;
            if(_sendEvent.IsRunning()){
                Simulator::Cancel(_sendEvent);
            }
            if(_socket){
                _socket->Close();
            }
        }

};

static void CwndChange(Ptr<OutputStreamWrapper> stream,uint32_t oldCwnd, uint32_t newCwnd){
            *stream->GetStream()<<Simulator::Now().GetSeconds()<<" "<<newCwnd<< std::endl;
 }

int main(int argc, char *argv[]){
    uint32_t payload = 102;
    std::string tcpTypeOne = "ns3::TcpNewReno";
    //std::string tcpTypeTwo = "ns3::TcpWestwoodPlus";
    std::string tcpTypeTwo = "ns3::TcpAdaptiveReno";

    int numberOfLeaf = 2;
    int numberOfFlow = 2;
    std::string sender_dataRate = "1Gbps";
    std::string sender_delay = "1ms";
    std::string outputDir = "scratch/tcphs";
    int simulationTimeInSec = 6;
    int cleanUpTime = 2;
    int bottleNekRate = 50;
    int bottleNekDelay = 100;
    int packetLossExp = 6;
    int exp = 1;

    CommandLine cmd (__FILE__);
    cmd.AddValue ("tcp2","Name of TCP variant 2", tcpTypeTwo);
    cmd.AddValue ("nLeaf","Number of left and right side leaf nodes", numberOfLeaf);
    cmd.AddValue ("bttlnkRate","Max Packets allowed in the device queue", bottleNekRate);
    cmd.AddValue ("plossRate", "Packet loss rate", packetLossExp);
    cmd.AddValue ("output_folder","Folder to store dara", outputDir);
    cmd.AddValue ("exp","1 for bttlnck, 2 for packet loss rate", exp);
    cmd.Parse (argc,argv);

    std::string file = outputDir + "/Data_" + std::to_string(exp)+".txt";
    numberOfFlow = numberOfLeaf;
    double packetLossRate = (1.0/std::pow(10,packetLossExp));
    std::string bottleNeckDataRate = std::to_string(bottleNekRate) + "Mbps";
    std::string bottleNeckDelay = std::to_string(bottleNekDelay)+ "ms";

    NS_LOG_UNCOND("USING TCP 1 = "<<tcpTypeOne<<" ; TCP 2 ="<<tcpTypeTwo<<" ; nLeaf ="<<numberOfLeaf<<" ; bottleNeckRate = "<<bottleNeckDataRate<<" ; packet loss rate = "<<packetLossRate<<" ; sender date rate = "<<sender_dataRate);
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payload));

    PointToPointHelper bottleNeckLine;

    bottleNeckLine.SetDeviceAttribute("DataRate",StringValue(bottleNeckDataRate));
    bottleNeckLine.SetChannelAttribute("Delay", StringValue(bottleNeckDelay));

    PointToPointHelper p2pleaf;
    p2pleaf.SetDeviceAttribute("DataRate",StringValue(sender_dataRate));
    p2pleaf.SetChannelAttribute("Delay",StringValue(sender_delay));
    p2pleaf.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue (std::to_string (bottleNekDelay * bottleNekRate * 125 / payload) + "p"));

    PointToPointDumbbellHelper p2pdumb(numberOfLeaf,p2pleaf,numberOfLeaf,p2pleaf,bottleNeckLine);

    Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
    em->SetAttribute("ErrorRate",DoubleValue(packetLossRate));
    p2pdumb.m_routerDevices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

    
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue (tcpTypeOne));
    InternetStackHelper stackOne;

    

    for(uint32_t i=0;i < p2pdumb.LeftCount(); i=i+2){
        stackOne.Install(p2pdumb.GetLeft(i));
    }

    for(uint32_t i=0; i<p2pdumb.RightCount(); i=i+2){
        stackOne.Install(p2pdumb.GetRight(i));
    }

    stackOne.Install(p2pdumb.GetLeft());
    stackOne.Install(p2pdumb.GetRight());

    //NS_LOG_UNCOND("here");
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue (tcpTypeTwo));
    InternetStackHelper stackTwo;
    
    for(uint32_t i=1 ;i < p2pdumb.LeftCount(); i=i+2){
        stackTwo.Install(p2pdumb.GetLeft(i));
    }

    for(uint32_t i=1 ; i<p2pdumb.RightCount(); i=i+2){
        stackTwo.Install(p2pdumb.GetRight(i));
    }

    p2pdumb.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"), // left nodes
                          Ipv4AddressHelper ("10.2.1.0", "255.255.255.0"),  // right nodes
                          Ipv4AddressHelper ("10.3.1.0", "255.255.255.0")); // routers 
    Ipv4GlobalRoutingHelper::PopulateRoutingTables (); // populate routing table
    
    
    FlowMonitorHelper flow;
    flow.SetMonitorAttribute("MaxPerHopDelay", TimeValue(Seconds(cleanUpTime)));
    Ptr<FlowMonitor> monitor = flow.InstallAll();

    

    for(int i=0;i<numberOfFlow;i++){
        Address sinkAdd (InetSocketAddress(p2pdumb.GetRightIpv4Address(i), 8080));
        PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny(), 8080));
        ApplicationContainer sinkApps = packetSinkHelper.Install(p2pdumb.GetRight(i));
        sinkApps.Start(Seconds(0));
        sinkApps.Stop(Seconds(simulationTimeInSec + cleanUpTime));
        Ptr<Socket> tcpSocket = Socket::CreateSocket (p2pdumb.GetLeft(i), TcpSocketFactory::GetTypeId());
        Ptr<MyApp> app = CreateObject<MyApp>();

        app->Setup(tcpSocket, sinkAdd, payload,DataRate(sender_dataRate),simulationTimeInSec);
        p2pdumb.GetLeft(i)->AddApplication(app);
        app->SetStartTime(Seconds(1));
        app->SetStopTime(Seconds(simulationTimeInSec));

        std::ostringstream oss;
        oss<< outputDir<<"/flow"<<i+1<<".cwnd";
        AsciiTraceHelper asciiTraceHelper;
        Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (oss.str());
        tcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));
    }

    Simulator::Stop(Seconds(simulationTimeInSec + cleanUpTime));
    Simulator::Run();

    int j=0;
    float CurrentThroughputArr[]={0,0};

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flow.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    for(auto itr= stats.begin(); itr!= stats.end();itr++){
        //Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(itr->first);
        if(j%2==0) {
            CurrentThroughputArr[0] += itr->second.rxBytes * 8;
        }
        if(j%2 == 1){
            CurrentThroughputArr[1] += itr->second.rxBytes * 8;
        }
        j++;

    }
    CurrentThroughputArr[0] /= ((simulationTimeInSec + cleanUpTime));
    CurrentThroughputArr[1] /= ((simulationTimeInSec + cleanUpTime));

    //NS_LOG_UNCOND("Throughput of "<<tcpTypeOne<<" = "<<CurrentThroughputArr[0]);
    //NS_LOG_UNCOND("Throughput of "<<tcpTypeTwo<<" = "<<CurrentThroughputArr[1]);
   // NS_LOG_UNCOND("Elapsed Time: "<<elapsed)

   std::string output_file = outputDir+"/data.txt";
    std::ofstream out(output_file, std::ios::app);
    out << bottleNekRate << "\t" << exp << "\t" << CurrentThroughputArr[0] << "\t" << CurrentThroughputArr[0] <<std::endl;

    Simulator::Destroy();
    return 0;
}