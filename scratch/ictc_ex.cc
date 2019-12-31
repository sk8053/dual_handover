/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/* *
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
 * Author: Michele Polese <michele.polese@gmail.com>
 */

#include "ns3/mmwave-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"
//#include "ns3/gtk-config-store.h"
#include <ns3/buildings-helper.h>
#include <ns3/buildings-module.h>
#include <ns3/random-variable-stream.h>
#include <ns3/lte-ue-net-device.h>

#include <iostream>
#include <ctime>
#include <stdlib.h>
#include <list>


using namespace ns3;

/**
 * Sample simulation script for MC device. It instantiates a LTE and a MmWave eNodeB,
 * attaches one MC UE to both and starts a flow for the UE to  and from a remote host.
 */

NS_LOG_COMPONENT_DEFINE ("McFirstExample");

class MyAppTag : public Tag
{
public:
  MyAppTag ()
  {
  }

  MyAppTag (Time sendTs) : m_sendTs (sendTs)
  {
  }

  static TypeId GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::MyAppTag")
      .SetParent<Tag> ()
      .AddConstructor<MyAppTag> ();
    return tid;
  }

  virtual TypeId  GetInstanceTypeId (void) const
  {
    return GetTypeId ();
  }

  virtual void  Serialize (TagBuffer i) const
  {
    i.WriteU64 (m_sendTs.GetNanoSeconds());
  }

  virtual void  Deserialize (TagBuffer i)
  {
    m_sendTs = NanoSeconds (i.ReadU64 ());
  }

  virtual uint32_t  GetSerializedSize () const
  {
    return sizeof (m_sendTs);
  }

  virtual void Print (std::ostream &os) const
  {
    std::cout << m_sendTs;
  }

  Time m_sendTs;
};


class MyApp : public Application
{
public:

  MyApp ();
  virtual ~MyApp();
  void ChangeDataRate (DataRate rate);
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);



private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp()
{
  m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::ChangeDataRate (DataRate rate)
{
  m_dataRate = rate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
 // MyAppTag tag (Simulator::Now ());

  m_socket->Send (packet);
    if (++m_packetsSent < m_nPackets)
  {
      ScheduleTx ();
  }
}



void
MyApp::ScheduleTx (void)
{
  if (m_running)
  {
    Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
    m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
  }
}
/*
static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
}


static void
RttChange (Ptr<OutputStreamWrapper> stream, Time oldRtt, Time newRtt)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldRtt.GetSeconds () << "\t" << newRtt.GetSeconds () << std::endl;
}
*/



double instantPacketSize[100], packetRxTime[100], lastPacketRxTime[100];
double sumPacketSize[100];

static void
Rx (Ptr<OutputStreamWrapper> stream, uint16_t i, Ptr<const Packet> packet, const Address &from){
  packetRxTime[i] = Simulator::Now().GetSeconds();
  if (lastPacketRxTime[i] == packetRxTime[i]){
    instantPacketSize[i] += packet->GetSize();
    return;
  }
  else{
    sumPacketSize[i] += instantPacketSize[i];
    *stream->GetStream () << lastPacketRxTime[i] << "\t" << instantPacketSize[i] << "\t" << sumPacketSize[i]
    		<< std::endl;
    lastPacketRxTime[i] =  packetRxTime[i];
    instantPacketSize[i] = packet->GetSize();
  }
}

uint64_t lastTotalRx[100];
uint16_t c[10];
double t_total[10];
void
CalculateThroughput (Ptr<OutputStreamWrapper> stream, Ptr<PacketSink> sink, uint16_t i)
{
  Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
  double cur = (sink->GetTotalRx () - lastTotalRx[i]) * (double) 8 / 1e5;     /* Convert Application RX Packets to MBits. */
  c[i]++;
  t_total[i]+=cur;
  *stream->GetStream()  << now.GetSeconds () << "\t" << cur <<"\t"<<(double)(t_total[i]/c[i])<<std::endl;
  lastTotalRx[i] = sink->GetTotalRx ();
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput,stream,sink,i);
}
/*
static void Sstresh (Ptr<OutputStreamWrapper> stream, uint32_t oldSstresh, uint32_t newSstresh)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldSstresh << "\t" << newSstresh << std::endl;
}
*/

int
main (int argc, char *argv[])
{

	std::cout << "splitting number : " << std::endl;
	std::cout << "0,1 -> single path" << std::endl;
	std::cout << "2 -> alternative splitting " << std::endl;
	std::cout << "3 -> p-Splitting" <<std::endl;
	std::cout << "4 -> SDF " << std::endl;
	std::cout << "5 -> SQF " << std::endl;
	 // uint16_t numberOfNodes = 1;
	// uint16_t nodeNum=1;
	  double simTime = 3.0;
	  double interPacketInterval = 20;  // 500 microseconds
	  bool harqEnabled = false;
	  bool rlcAmEnabled = false;
	  bool fixedTti = false;
	  unsigned symPerSf = 24;
	  double sfPeriod = 100.0;
	  bool tcp = false;
     double x2Latency = 0.0, mmeLatency=0.0;
     bool isEnablePdcpReordering = true;
	bool isEnableLteMmwave = false;
     uint16_t typeOfSplitting = 6; // 3 : p-split
      uint16_t Velocity = 1.5;
      uint16_t numberOfPacket = 500;
      std::string scheduler ="MmWaveFlexTtiMaxWeightMacScheduler";
      std::string pathLossModel = "BuildingsObstaclePropagationLossModel";
      std::string X2dataRate = "10Gb/s";
      int x2LinkDelay = 3;
	  // Command line arguments
	  CommandLine cmd;
	 // cmd.AddValue("numberOfNodes", "Number of eNodeBs + UE pairs", nodeNum);
	  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
	  cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
	  cmd.AddValue ("velocity" , "UE's velocity", Velocity);
	  cmd.AddValue ("isTcp", "TCP or UDP", tcp);
	  cmd.AddValue("x2LinkDataRate", "X2 link data rate " , X2dataRate);
	   cmd.AddValue("x2LinkDelay" , "X2 link delay", x2LinkDelay);
	   cmd.AddValue("pathLossModel", "path loss modles", pathLossModel);
	   cmd.AddValue ("scheduler", "lte scheduler", scheduler);
	   cmd.AddValue("rlcAmEnabled", "lte rlc avilability",rlcAmEnabled);
       cmd.AddValue("harqEnabled", "harq enable or not", harqEnabled);
	  cmd.AddValue("typeOfSplitting", "splitting algorithm type",typeOfSplitting);
	  cmd.AddValue("numberOfPacket", "number of packet", numberOfPacket);
	  cmd.Parse(argc, argv);
	 // Config::SetDefault ("ns3::LteEnbRrc::EpsBearerToRlcMapping", EnumValue (ns3::LteEnbRrc::RLC_AM_ALWAYS));
	  Config::SetDefault("ns3::McUePdcp::EnableReordering", BooleanValue(isEnablePdcpReordering));
	  Config::SetDefault ("ns3::MmWaveHelper::RlcAmEnabled", BooleanValue(rlcAmEnabled));
	  Config::SetDefault ("ns3::MmWaveHelper::HarqEnabled", BooleanValue(harqEnabled));
	  Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::HarqEnabled", BooleanValue(harqEnabled));
	  Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::HarqEnabled", BooleanValue(harqEnabled));
	  Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::FixedTti", BooleanValue(fixedTti));
	  Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::SymPerSlot", UintegerValue(6));
	  Config::SetDefault ("ns3::MmWavePhyMacCommon::ResourceBlockNum", UintegerValue(1));
	  //Config::SetDefault ("ns3::MmWavePhyMacCommon::ChunkPerRB", UintegerValue(72));
	  Config::SetDefault ("ns3::MmWavePhyMacCommon::SymbolsPerSubframe", UintegerValue(symPerSf));
	  Config::SetDefault ("ns3::MmWavePhyMacCommon::SubframePeriod", DoubleValue(sfPeriod));
	  Config::SetDefault ("ns3::MmWavePhyMacCommon::TbDecodeLatency", UintegerValue(200.0));
	  Config::SetDefault ("ns3::MmWavePhyMacCommon::NumHarqProcess", UintegerValue(100));
	  Config::SetDefault ("ns3::MmWaveBeamforming::LongTermUpdatePeriod", TimeValue (MilliSeconds (100.0)));
	  Config::SetDefault ("ns3::LteEnbRrc::SystemInformationPeriodicity", TimeValue (MilliSeconds (5.0)));
	 // Config::SetDefault ("ns3::MmWavePropagationLossModel::ChannelStates", StringValue ("n"));
	  Config::SetDefault ("ns3::LteRlcAm::ReportBufferStatusTimer", TimeValue(MicroSeconds(100.0)));
	  Config::SetDefault ("ns3::LteRlcUmLowLat::ReportBufferStatusTimer", TimeValue(MicroSeconds(100.0)));
	  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (320));
	  Config::SetDefault ("ns3::LteEnbRrc::FirstSibTime", UintegerValue (2));
	  Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::X2LinkDelay", TimeValue (MicroSeconds(x2Latency)));
	  Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::X2LinkDataRate", DataRateValue(DataRate ("1000Gb/s")));
	  Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::X2LinkMtu",  UintegerValue(10000));
	  Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::S1uLinkDelay", TimeValue (MicroSeconds(1000)));
	  Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::S1apLinkDelay", TimeValue (MicroSeconds(mmeLatency)));
	  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
	  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1024*1024*30));
	  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1024*1024*30));
	  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1400));

	  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (100 * 1024 * 1024));
	  Config::SetDefault ("ns3::LteRlcUmLowLat::MaxTxBufferSize", UintegerValue (100 * 1024 * 1024));
	  Config::SetDefault ("ns3::LteRlcAm::StatusProhibitTimer", TimeValue(MilliSeconds(1.0)));
	  Config::SetDefault ("ns3::LteRlcAm::MaxTxBufferSize", UintegerValue (300 *1024 * 1024));

	  Config::SetDefault ("ns3::PointToPointEpcHelper::X2LinkDelay", TimeValue (MilliSeconds(x2LinkDelay)));
	  Config::SetDefault ("ns3::PointToPointEpcHelper::X2LinkDataRate", DataRateValue (DataRate(X2dataRate)));


      Config::SetDefault("ns3::McEnbPdcp::numberOfAlgorithm",UintegerValue(typeOfSplitting));
      Config::SetDefault("ns3::McEnbPdcp::enableLteMmWaveDC", BooleanValue(isEnableLteMmwave));
	  	  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));

	  	  Ptr<MmWaveHelper> mmwaveHelper = CreateObject<MmWaveHelper> ();
	   mmwaveHelper->SetSchedulerType ("ns3::"+scheduler);

	   Ptr<MmWavePointToPointEpcHelper> epcHelper = CreateObject<MmWavePointToPointEpcHelper> ();
	   mmwaveHelper->SetEpcHelper (epcHelper);
	   mmwaveHelper->SetHarqEnabled (harqEnabled);

	  mmwaveHelper->SetAttribute ("PathlossModel", StringValue ("ns3::"+pathLossModel));
	 //  mmwaveHelper->SetAttribute ("PathlossModel", StringValue ("ns3::MmWave3gppPropagationLossModel"));
	   //mmwaveHelper->SetAttribute("ChannelModel", StringValue("ns3::MmWave3gppChannel"));
	  mmwaveHelper->Initialize();
	   cmd.Parse(argc, argv);

	   uint16_t nodeNum=1;

	   Ptr<Node> pgw = epcHelper->GetPgwNode ();
	     NodeContainer remoteHostContainer;
	     remoteHostContainer.Create (nodeNum);
	      InternetStackHelper internet;
	     internet.Install (remoteHostContainer);
	     Ipv4Address remoteHostAddr[nodeNum];
	     Ipv4StaticRoutingHelper ipv4RoutingHelper;
	     Ptr<Node> remoteHost ;
	     for (uint16_t i=0 ; i<nodeNum; i++)
	     {
	     // Create the Internet by connecting remoteHost to pgw. Setup routing too
	     remoteHost = remoteHostContainer.Get (i);
	     PointToPointHelper p2ph;
	     p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
	     p2ph.SetDeviceAttribute ("Mtu", UintegerValue (2500));
	     p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
	     NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);

	     Ipv4AddressHelper ipv4h;
		    std::ostringstream subnet;
		    subnet<<i+1<<".1.0.0";
	     ipv4h.SetBase (subnet.str ().c_str (), "255.255.0.0");
	     Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
	     // interface 0 is localhost, 1 is the p2p device
	     remoteHostAddr[i] = internetIpIfaces.GetAddress (1);
	     Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
	     remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.255.0.0"), 1);
	     }
	     // create LTE, mmWave eNB nodes and UE node
	     NodeContainer ueNodes;
	     NodeContainer mmWaveEnbNodes;
	     NodeContainer lteEnbNodes;
	     NodeContainer allEnbNodes;
	     mmWaveEnbNodes.Create(2);
	     lteEnbNodes.Create(1);
	     ueNodes.Create(nodeNum);
	     allEnbNodes.Add(lteEnbNodes);
	     allEnbNodes.Add(mmWaveEnbNodes);


	     Vector mmw1Position = Vector(200.0,0, 12);  ///73Ghz //path 1
	      Vector mmw2Position = Vector(0.0, 0.0, 12); //28Ghz // path 0


	      // Install Mobility Model
	        Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
	        enbPositionAlloc->Add (Vector ((double)0.0, 30, 10));
	        enbPositionAlloc->Add (mmw1Position);
	        enbPositionAlloc->Add (mmw2Position);
	        MobilityHelper enbmobility;
	        enbmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	        enbmobility.SetPositionAllocator(enbPositionAlloc);
	        enbmobility.Install (allEnbNodes);

	        MobilityHelper uemobility;
	          Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
	        for(uint16_t i =0 ; i<ueNodes.GetN(); i++){
	        	//if (i==0)
	        		uePositionAlloc->Add(Vector(100+i,0,1.5));

	        }

	          uemobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
	          uemobility.SetPositionAllocator(uePositionAlloc);
	          uemobility.Install (ueNodes);


	          ueNodes.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0, 0, 0));

	         BuildingsHelper::Install (mmWaveEnbNodes);
	          BuildingsHelper::Install(lteEnbNodes);
	      	BuildingsHelper::Install (ueNodes);

	          // Install mmWave, lte, mc Devices to the nodes
	           NetDeviceContainer lteEnbDevs = mmwaveHelper->InstallLteEnbDevice (lteEnbNodes);
	           NetDeviceContainer mmWaveEnbDevs_73GHZ = mmwaveHelper->InstallEnbDevice_73GHZ(mmWaveEnbNodes.Get(0));
	           NetDeviceContainer mmWaveEnbDevs_28GHZ = mmwaveHelper->InstallEnbDevice_28GHZ(mmWaveEnbNodes.Get(1));


	           NetDeviceContainer mcUeDevs;

	           mcUeDevs = mmwaveHelper->InstallMcUeDevice (ueNodes.Get(0));
	           //device = mmwaveHelper->InstallUeDevice(ueNodes.Get(1));
	           //mcUeDevs.Add(device);


	           // Install the IP stack on the UEs
	             internet.Install (ueNodes);
	             Ipv4InterfaceContainer ueIpIface;
	             ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (mcUeDevs));

	             // Assign IP address to UEs, and install applications
	             for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
	             {
	               Ptr<Node> ueNode = ueNodes.Get (u);
	               // Set the default gateway for the UE
	               Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
	               ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
	             }

	             // Add X2 interfaces
	             mmwaveHelper->AddX2Interface (lteEnbNodes, mmWaveEnbNodes);
	             mmwaveHelper->AttachToClosestEnb (mcUeDevs, mmWaveEnbDevs_28GHZ, mmWaveEnbDevs_73GHZ, lteEnbDevs);

	             uint16_t dlPort = 1234;
	             //uint16_t ulPort = 2000;
	             ApplicationContainer clientApps;
	             ApplicationContainer serverApps;



	 for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
	   {

			 PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
			 serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
			 Ptr<MyApp> app = CreateObject<MyApp> ();
			 Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (remoteHostContainer.Get (u), UdpSocketFactory::GetTypeId ());
			  Address sinkAddress (InetSocketAddress (ueIpIface.GetAddress (u), dlPort));
			  app->Setup (ns3UdpSocket, sinkAddress, 1400, numberOfPacket, DataRate ("500Mbps"));
			   remoteHostContainer.Get (u)->AddApplication (app);

			   std::ostringstream fileName;
			   	    fileName<<"udp_data_ue"<<u+1<<".txt";
			   		AsciiTraceHelper asciiTraceHelper;
			   		Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (fileName.str ().c_str ());
			   		serverApps.Get(u)->TraceConnectWithoutContext("Rx",MakeBoundCallback (&Rx, stream,u));

			    std::ostringstream fileName_2;
			    fileName_2<<"udp_throughput_ue" << u+1 <<".txt";
				AsciiTraceHelper asciiTraceHelper_2;
				Ptr<OutputStreamWrapper> stream_2 = asciiTraceHelper_2.CreateFileStream(fileName_2.str().c_str());
				Simulator::Schedule (Seconds (0.1), &CalculateThroughput,stream_2,serverApps.Get(u)->GetObject<PacketSink>(),u);

				app->SetStartTime (Seconds (0.2+u*0.7));
		        app->SetStopTime (Seconds (simTime+0.1));


	  }


mmwaveHelper -> EnableTraces();
 	 	 // Start applications
Config::Set ("/NodeList/*/DeviceList/*/TxQueue/MaxPackets", UintegerValue (1000*1000));
	               serverApps.Start (Seconds (0.001));


	               Simulator::Stop(Seconds(simTime));
	                Simulator::Run();
	                Simulator::Destroy();
	                 return 0;
}
