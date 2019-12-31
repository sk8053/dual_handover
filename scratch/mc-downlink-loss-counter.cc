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
#include "ns3/MyAppTag.h"
#include <ctime>
using namespace ns3;
using namespace std;
/**
 * Sample simulation script for MC device. It instantiates a LTE and a MmWave eNodeB,
 * attaches one MC UE to both and starts a flow for the UE to  and from a remote host.
 */

NS_LOG_COMPONENT_DEFINE ("McFirstExample");

class MyApp : public Application
{
	public:

		MyApp ();
		virtual ~MyApp();
		void ChangeDataRate (DataRate rate);
		void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);
		void SetSinkApp(Ptr<PacketSink>);
		Ptr<PacketSink> GetSinkApp();
		uint16_t appNumber;
		void SetStream( Ptr<OutputStreamWrapper> stream);
	private:
		virtual void StartApplication (void);
		virtual void StopApplication (void);

		void ScheduleTx (void);
		void SendPacket (void);
		void WaitNextData(void);//sjkang

		Ptr<Socket>     m_socket;
		Address         m_peer;
		uint32_t        m_packetSize;
		uint32_t        m_nPackets;
		DataRate        m_dataRate;
		EventId         m_sendEvent;
		bool            m_running;
		uint32_t        m_packetsSent;
		uint64_t 		m_sn;
		std::map<uint32_t,Time> startFileTransmit;
		bool 	firstTx;
		Ptr<PacketSink> sink;
		uint16_t WaitMessage[3000];
		Ptr<OutputStreamWrapper> fileDelay;
};

	MyApp::MyApp ()
: m_socket (0),
	m_peer (),
	m_packetSize (0),
	m_nPackets (0),
	m_dataRate (0),
	m_sendEvent (),
	m_running (false),
	m_packetsSent (0),
	m_sn(1),
	firstTx(true),
	sink(0)

{
	appNumber =0;
	startFileTransmit.clear();
	for (uint16_t i =0; i<3000;i++) WaitMessage[i] =0;
}

MyApp::~MyApp()
{
	m_socket = 0;
}
void
MyApp::SetStream( Ptr<OutputStreamWrapper> stream){
	fileDelay = stream;
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
MyApp::SetSinkApp(Ptr<PacketSink> sink){
	this->sink = sink;
}
Ptr<PacketSink>
MyApp::GetSinkApp(){
	return sink;
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
	m_sn = 0;
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


std::ofstream fileDelay("fileDelay.txt");
double SumOfFileDelay;
uint64_t file_delay_counter;


uint32_t packetNumber=7500;
uint16_t timeInterval =120;
bool isTcp;
uint64_t count_data=0;
	void
MyApp::SendPacket (void)
{
	uint32_t messageNumber = (m_sn - (m_sn % m_nPackets))/m_nPackets ;

	//if (m_sn % m_nPackets ==0 && (m_sn - m_sn % m_nPackets)/m_nPackets >= 1)
	if (m_sn % m_nPackets ==0 && !isTcp)
	{

		std::map <uint32_t, bool> SuccessIndi = sink->GetSuccess();
		std::map <uint32_t, Time> ArrivalTimes = sink->GetArrivalTime();
		std::map <uint32_t, Time>::iterator iter;

		for (iter = ArrivalTimes.begin(); iter != ArrivalTimes.end(); iter++){
			if (SuccessIndi[iter->first]){
				//	std::cout << (iter->second).GetSeconds() <<std::endl;

				double delay = (iter->second).GetSeconds() - startFileTransmit[iter->first].GetSeconds();
				SumOfFileDelay += delay;
				file_delay_counter++;
				*fileDelay->GetStream()<<Simulator::Now().GetSeconds()<< " Delay: "<<  delay <<" Avg Delay: "<< SumOfFileDelay/file_delay_counter<<
					"\t"<< iter->first<< std::endl;
			}else{
				*fileDelay->GetStream()<<Simulator::Now().GetSeconds()<< "\t"<<  200 <<"\t"<<200<<
					"\t"<< iter->first <<"\t"<<sink->GetMessageSize(iter->first)<< std::endl;
			}
			startFileTransmit.erase(iter->first);
			sink->eraseMaps();
		}

		for (iter = startFileTransmit.begin();iter !=startFileTransmit.end(); iter++){
			if (ArrivalTimes.find(iter->first) == ArrivalTimes.end()){
				WaitMessage[iter->first] ++;
				if (WaitMessage[iter->first] > 2){
					*fileDelay->GetStream()<<Simulator::Now().GetSeconds()<< "\t"<<  200 <<"\t"<<200<<
						"\t"<< iter->first <<"\t"<< sink->GetMessageSize(iter->first)<< std::endl;
					startFileTransmit.erase(iter->first);
				}
			}
		}
		startFileTransmit[messageNumber] = Simulator::Now();
	}
	Ptr<Packet> packet = Create<Packet> (m_packetSize);

	MyAppTag tag (Simulator::Now()) ;
	tag.SetTime (Simulator::Now());
	packet->AddByteTag(tag);

	SeqTsHeader seqHeader;
	seqHeader.SetSeq(m_sn);
	packet->AddHeader(seqHeader);
	m_sn ++;

	m_socket->Send (packet);

	if (++m_packetsSent < m_nPackets)
	{
		ScheduleTx ();
	}
	else if ( m_packetsSent == m_nPackets ) {

		//	m_sn = 1;
		m_packetsSent =0;
		//	sink->SetReceived(0);

		WaitNextData();
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
	void
MyApp::WaitNextData(void)
{


	if (m_running)
	{
		m_sendEvent = Simulator::Schedule (MilliSeconds(timeInterval), &MyApp::SendPacket, this);
	}
}


double SumOfDelay_value[50];
uint64_t delay_counter[50];

static void
LinkDelay(Ptr<OutputStreamWrapper> stream, uint16_t i,   double d){  //tcp linkDelay ??
	SumOfDelay_value[i] +=d;
	delay_counter[i] ++;
	*stream->GetStream() << Simulator::Now().GetSeconds()<< "\t"<<d <<"\t"
		<<SumOfDelay_value[i]/delay_counter[i]<< std::endl;


}

double instantPacketSize[100], packetRxTime[100], lastPacketRxTime[100];
double sumPacketSize[100];


static void
Rx (Ptr<OutputStreamWrapper> stream, uint16_t i, Ptr<const Packet> packet, const Address &from){
	count_data++;
	sumPacketSize[i] += packet->GetSize();
	*stream->GetStream()<<Simulator::Now().GetSeconds()<<"\t"<<packet->GetSize()<<"\t"<<sumPacketSize[i]
		<<"\t"<<count_data << std::endl;

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

static void
Loss (Ptr<OutputStreamWrapper> stream , uint64_t received, uint32_t lossAmount ){
	*stream->GetStream() << Simulator::Now().GetSeconds() << "\t"<< lossAmount<< "\t" << received << std::endl;
}
	void
ChangeSpeed(Ptr<Node>  n, Vector speed)
{
	n->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (speed);
}
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
	static void
Traces(uint16_t nodeNum)
{
	AsciiTraceHelper asciiTraceHelper;

	std::ostringstream pathCW;
	pathCW<<"/NodeList/"<<nodeNum+1<<"/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow";

	std::ostringstream fileCW;
	//fileCW<<"UE-"<<nodeNum+1<<"-TCP-CWND.txt";
	fileCW<<"tcp_cwnd_ue"<<nodeNum+1<<".txt";

	std::ostringstream pathRTT;
	pathRTT<<"/NodeList/"<<nodeNum+1<<"/$ns3::TcpL4Protocol/SocketList/0/RTT";

	std::ostringstream fileRTT;
	//fileRTT<<"UE-"<<nodeNum+1<<"-TCP-RTT.txt";
	fileRTT<<"tcp_rtt_ue"<<nodeNum+1<<".txt";


	std::ostringstream pathRCWnd;
	pathRCWnd<<"/NodeList/"<<nodeNum+1<<"/$ns3::TcpL4Protocol/SocketList/0/RWND";

	std::ostringstream fileRCWnd;
	//fileRCWnd<<"UE-"<<nodeNum+1<<"-TCP-RCWND.txt";
	fileRCWnd<<"tcp_rcwnd_ue"<<nodeNum+1<<".txt";

	Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream (fileCW.str ().c_str ());
	Config::ConnectWithoutContext (pathCW.str ().c_str (), MakeBoundCallback(&CwndChange, stream1));

	Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream (fileRTT.str ().c_str ());
	Config::ConnectWithoutContext (pathRTT.str ().c_str (), MakeBoundCallback(&RttChange, stream2));

	Ptr<OutputStreamWrapper> stream4 = asciiTraceHelper.CreateFileStream (fileRCWnd.str ().c_str ());
	Config::ConnectWithoutContext (pathRCWnd.str ().c_str (), MakeBoundCallback(&CwndChange, stream4));

}
void
TracePosition(Ptr<Node> node, Ptr<OutputStreamWrapper> stream){
	Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
	Vector current =mobility ->GetPosition();
	*stream->GetStream() << current.x << "\t" << current.y << "\t" << current.z  <<"\t"<<Simulator::Now().GetSeconds()<<std::endl;
	Simulator::Schedule(MilliSeconds(10),&TracePosition, node, stream);
}
	int
main (int argc, char *argv[])
{

	std::cout << "splitting number : " << std::endl;
	std::cout << "0,1 -> single path" << std::endl;
	std::cout << "2 -> alternative splitting " << std::endl;
	std::cout << "3 -> p-Splitting" <<std::endl;
	std::cout << "4 -> SDF " << std::endl;
	std::cout << "5 -> SQF " << std::endl;
	uint16_t numberOfNodes = 1;
	double simTime = 40.0;
	double interPacketInterval = 20;  // 500 microseconds
	bool harqEnabled = true;
	bool rlcAmEnabled = true;
	bool fixedTti = false;
	unsigned symPerSf = 24;
	double sfPeriod = 100.0;
	bool tcp = false, dl= true, ul=false;
	isTcp = tcp;
	double x2Latency = 1000.0, mmeLatency=0.0; //x2Latency should range from 0~20ms
	bool isEnablePdcpReordering;
	if (tcp) isEnablePdcpReordering = true;else isEnablePdcpReordering=false;
	bool isEnableLteMmWave = false;
	uint16_t typeOfSplitting ;
	string buildingTopology = "building_topology.txt";
	//std::string scenario ="UMa";

	if (isEnableLteMmWave)
		typeOfSplitting = 6; // 3 : p-split
	else
		typeOfSplitting = 0; // 3 : p-split 1 is 28G, 0 is 73G

	double Velocity =5.0;
	//int x2LinkDelay =0;
	std::string X2dataRate = "10Gb/s";
	std::string scheduler ="MmWaveFlexTtiMacScheduler";
	std::string pathLossModel = "BuildingsObstaclePropagationLossModel";
	// Command line arguments
	CommandLine cmd;
	cmd.AddValue("numberOfNodes", "Number of eNodeBs + UE pairs", numberOfNodes);
	cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
	cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
	cmd.AddValue("typeOfSplitting", "splitting algorithm type",typeOfSplitting);
	cmd.AddValue ("velocity" , "UE's velocity", Velocity);
	cmd.AddValue ("isTcp", "TCP or UDP", tcp);
	cmd.AddValue("x2LinkDataRate", "X2 link data rate " , X2dataRate);
	//	cmd.AddValue("x2LinkDelay" , "X2 link delay", x2LinkDelay);
	cmd.AddValue("pathLossModel", "path loss modles", pathLossModel);
	cmd.AddValue ("scheduler", "lte scheduler", scheduler);
	cmd.AddValue("rlcAmEnabled", "lte rlc avilability",rlcAmEnabled);
	cmd.AddValue("harqEnabled", "harq enable or not", harqEnabled);
	cmd.AddValue ("nPacket", "number of Packet", packetNumber);
	cmd.AddValue ("timeInterval", "transmission interval", timeInterval);
	cmd.AddValue ("buildingTopology","building topology", buildingTopology);
	cmd.Parse(argc, argv);
	// Config::SetDefault ("ns3::LteEnbRrc::EpsBearerToRlcMapping", EnumValue (ns3::LteEnbRrc::RLC_AM_ALWAYS));
	Config::SetDefault("ns3::McUePdcp::EnableReordering", BooleanValue(isEnablePdcpReordering));
	Config::SetDefault ("ns3::MmWaveHelper::RlcAmEnabled", BooleanValue(rlcAmEnabled));
	Config::SetDefault ("ns3::MmWaveHelper::HarqEnabled", BooleanValue(harqEnabled));
	Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::HarqEnabled", BooleanValue(harqEnabled));
	Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::FixedTti", BooleanValue(fixedTti));
	Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::SymPerSlot", UintegerValue(6));
	Config::SetDefault ("ns3::MmWavePhyMacCommon::ResourceBlockNum", UintegerValue(1));
	Config::SetDefault ("ns3::MmWavePhyMacCommon::ChunkPerRB", UintegerValue(72));
	Config::SetDefault ("ns3::MmWavePhyMacCommon::SymbolsPerSubframe", UintegerValue(symPerSf));
	Config::SetDefault ("ns3::MmWavePhyMacCommon::SubframePeriod", DoubleValue(sfPeriod));
	Config::SetDefault ("ns3::MmWavePhyMacCommon::TbDecodeLatency", UintegerValue(200.0));
	Config::SetDefault ("ns3::MmWavePhyMacCommon::NumHarqProcess", UintegerValue(100));
	Config::SetDefault ("ns3::MmWaveBeamforming::LongTermUpdatePeriod", TimeValue (MilliSeconds (100.0)));
	Config::SetDefault ("ns3::LteEnbRrc::SystemInformationPeriodicity", TimeValue (MilliSeconds (5.0)));
	//Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::ChannelCondition", StringValue ("n"));
	//	Config::SetDefault("ns3::MmWave3gppChannel::Blockage", BooleanValue(true));
	//	Config::SetDefault("ns3::MmWave3gppChannel::NumNonselfBlocking", IntegerValue(4));

	Config::SetDefault ("ns3::LteRlcAm::ReportBufferStatusTimer", TimeValue(MicroSeconds(100.0)));
	Config::SetDefault ("ns3::LteRlcUmLowLat::ReportBufferStatusTimer", TimeValue(MicroSeconds(100.0)));
	Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (320));
	Config::SetDefault ("ns3::LteEnbRrc::FirstSibTime", UintegerValue (2));
	Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::X2LinkDelay", TimeValue (MicroSeconds(x2Latency)));
	Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::X2LinkDataRate", DataRateValue(DataRate ("10Gb/s")));
	Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::X2LinkMtu",  UintegerValue(10000));
	Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::S1uLinkDelay", TimeValue (MicroSeconds(1000)));
	//Config::SetDefault("ns3::MmWavePointToPointEpcHelper::S1uLinkDataRate", DataRateValue(DataRate("1000Gb/s")));
	Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::S1apLinkDelay", TimeValue (MicroSeconds(mmeLatency)));
	//Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
	Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1024*1024*100));
	Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1024*1024*100));
	Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1400));

	Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (100 * 1024 * 1024));
	Config::SetDefault ("ns3::LteRlcUmLowLat::MaxTxBufferSize", UintegerValue (100 * 1024 * 1024));
	Config::SetDefault ("ns3::LteRlcAm::StatusProhibitTimer", TimeValue(MilliSeconds(1.0)));
	Config::SetDefault ("ns3::LteRlcAm::MaxTxBufferSize", UintegerValue (100 *1024 * 1024));
	Config::SetDefault("ns3::LteRlcAm::EnableAQM", BooleanValue(false));
	Config::SetDefault("ns3::LteEnbRrc::SecondaryCellHandoverMode", EnumValue(3));
	//Config::SetDefault ("ns3::PointToPointEpcHelper::X2LinkDataRate", DataRateValue (DataRate(X2dataRate)));
	//	Config::SetDefault ("ns3::PointToPointEpcHelper::X2LinkDelay", TimeValue (MilliSeconds(x2LinkDelay)));
	//  Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::ChannelCondition", StringValue("n"));
	//	Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::InCar", BooleanValue(true)); // enable or disable the shadowing effect
	//Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::OptionalNlos", BooleanValue(false));


	Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Scenario", StringValue("UMa"));
	Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Shadowing", BooleanValue(true)); // enable or disable the shadowing effect
	Config::SetDefault ("ns3::MmWave3gppChannel::UpdatePeriod", TimeValue (MilliSeconds (100))); // Set channel update period, 0 stands for no update.
	Config::SetDefault ("ns3::MmWave3gppChannel::CellScan", BooleanValue(true)); // Set true to use cell scanning method, false to use the default power method.
	//	Config::SetDefault ("ns3::MmWave3gppChannel::Blockage", BooleanValue(true)); // use blockage or not
	//	Config::SetDefault ("ns3::MmWave3gppChannel::PortraitMode", BooleanValue(true)); // use blockage model with UT in portrait mode
	//	Config::SetDefault ("ns3::MmWave3gppChannel::NumNonselfBlocking", IntegerValue(4)); // number of non-self blocking obstacles
	//	Config::SetDefault ("ns3::MmWave3gppChannel::BlockerSpeed", DoubleValue(1)); // speed of non-self blocking obstacles



	Config::SetDefault("ns3::McEnbPdcp::numberOfAlgorithm",UintegerValue(typeOfSplitting));
	Config::SetDefault("ns3::McEnbPdcp::enableLteMmWaveDC", BooleanValue(isEnableLteMmWave));
	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpCubic::GetTypeId ()));
	//Config::SetDefault("ns3::PacketSink::MessgeTransmission", BooleanValue(!tcp));
	//	Config::SetDefault("ns3::PacketSink::PacketNumber", UintegerValue(packetNumber));

	Ptr<MmWaveHelper> mmwaveHelper = CreateObject<MmWaveHelper> ();
	//mmwaveHelper->SetSchedulerType ("ns3::MmWaveFlexTtiMaxWeightMacScheduler");
	mmwaveHelper->SetSchedulerType ("ns3::"+scheduler);

	Ptr<MmWavePointToPointEpcHelper> epcHelper = CreateObject<MmWavePointToPointEpcHelper> ();
	mmwaveHelper->SetEpcHelper (epcHelper);
	mmwaveHelper->SetHarqEnabled (harqEnabled);
	//epcHelper->ActivateEpsBearer()
	mmwaveHelper->SetAttribute ("PathlossModel", StringValue ("ns3::BuildingsObstaclePropagationLossModel"));
	//mmwaveHelper->SetAttribute ("PathlossModel", StringValue ("ns3::"+pathLossModel));
	// mmwaveHelper->SetAttribute("LtePathlossModel",StringValue("ns3::ItuR1411LosPropagationLossModel"));
	//  mmwaveHelper->SetAttribute ("PathlossModel", StringValue ("ns3::MmWave3gppPropagationLossModel"));
	//	  mmwaveHelper->SetAttribute ("ChannelModel", StringValue ("ns3::MmWave3gppChannel"));
	//	  mmwaveHelper->SetAttribute ("PathlossModel", StringValue ("ns3::MmWave3gppBuildingsPropagationLossModel"));

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
	NodeContainer mmWaveEnbNodes_28G;
	NodeContainer mmWaveEnbNodes_73G;
	NodeContainer lteEnbNodes;
	NodeContainer allEnbNodes;

	mmWaveEnbNodes_73G.Create(3);
	mmWaveEnbNodes_28G.Create(3);
	lteEnbNodes.Create(1);
	ueNodes.Create(nodeNum);

	allEnbNodes.Add(lteEnbNodes);
	allEnbNodes.Add(mmWaveEnbNodes_73G);
	allEnbNodes.Add(mmWaveEnbNodes_28G);
	std::ofstream f ("enb_topology.txt");

	Vector mmw1Position = Vector(0.0,0.0, 25);  ///28Ghz //path 0
	Vector mmw2Position = Vector(0.0, 50.0, 25); //28Ghz // path 0
	Vector mmw3Position = Vector(0.0, 100.0, 25); //28Ghz // path 0
	Vector mmw4Position = Vector(100.0, 0.0, 25); //28Ghz // path 0

	//  uint16_t cellID = 2;
	Vector mmw5Position = Vector(100.0,50.0, 25);  ///73Ghz //path 1
	Vector mmw6Position = Vector(100.0, 100, 25); //73Ghz // path 1
	//Vector mmw7Position = Vector (100.0, 60,25);
	//Vector mmw8Position = Vector(100.0, 90, 25); //73Ghz // path 1
	// Vector mmw7Position = Vector(0.0, 40, 12); //73Ghz // path 1
	//uint16_t t= 1;
	f<<mmw1Position.x<<"\t"<<mmw1Position.y<<"\t"<<Simulator::Now().GetSeconds()<<std::endl;
	f<<mmw2Position.x<<"\t"<<mmw2Position.y<<"\t"<<Simulator::Now().GetSeconds()<<std::endl;
	f<<mmw3Position.x<<"\t"<<mmw3Position.y<<"\t"<<Simulator::Now().GetSeconds()<<std::endl;
	f<<mmw4Position.x<<"\t"<<mmw4Position.y<<"\t"<<Simulator::Now().GetSeconds()<<std::endl;

	f<<mmw5Position.x<<"\t"<<mmw5Position.y<<"\t"<<Simulator::Now().GetSeconds()<<std::endl;
	f<<mmw6Position.x<<"\t"<<mmw6Position.y<<"\t"<<Simulator::Now().GetSeconds()<<std::endl;
	//f<<mmw7Position.x<<"\t"<<mmw7Position.y<<"\t"<<Simulator::Now().GetSeconds()<<std::endl;
	//	f<<mmw8Position.x<<"\t"<<mmw8Position.y<<"\t"<<Simulator::Now().GetSeconds()<<std::endl;

	f<<30<<"\t"<<-5<<"\t"<<std::endl;
	f.close();
	// Install Mobility Model
	Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
	enbPositionAlloc->Add (Vector ((double)30.0, -5, 12));
	enbPositionAlloc->Add (mmw1Position);
	enbPositionAlloc->Add (mmw3Position);
	enbPositionAlloc->Add (mmw5Position);
	enbPositionAlloc->Add (mmw2Position);
	enbPositionAlloc->Add (mmw4Position);
	enbPositionAlloc->Add (mmw6Position);
	//	enbPositionAlloc->Add (mmw7Position);
	//enbPositionAlloc->Add (mmw8Position);

	MobilityHelper enbmobility;
	enbmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	enbmobility.SetPositionAllocator(enbPositionAlloc);
	enbmobility.Install (allEnbNodes);

	MobilityHelper uemobility;

	Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
	//for(uint16_t i =0 ; i<ueNodes.GetN(); i++){
	uePositionAlloc->Add(Vector(50 ,0,1.5));

	//	uePositionAlloc->Add(Vector(52 ,100,1.5));
	//	uePositionAlloc->Add(Vector(48 ,100,1.5));
	//	uePositionAlloc->Add(Vector(50 ,110,1.5));
	//	uePositionAlloc->Add(Vector(52 ,50,1.5));
	//	uePositionAlloc->Add(Vector(48 ,50,1.5));

	uemobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
	uemobility.SetPositionAllocator(uePositionAlloc);
	uemobility.Install (ueNodes);
	uemobility.AssignStreams(ueNodes,0);

	/*Ptr<Building> building1  ;
	  Ptr<Building> building2 ;
	  srand((unsigned int) time (NULL));
	  ofstream file( "building_topology.txt");

	  for (int i =0 ; i< 30;i++){
	  double d1 = rand() % 15+6;
	  double d2 = rand() % 15+6;
	  double d3 = rand() % 15+6;
	  double d4 = rand() % 15+6;
	  int y = rand() % 88 +6;
	  int y_2 = rand()% 88 +6;
	  int x_2 = rand() % 41 + 55;
	  int x = rand()% 41 +5;
	  building1 = Create <Building>();
	  building2 = Create <Building>();
	  building1->SetBoundaries(Box(x,x+d1/10, y, y+d2/10,0,33.5));
	  building2->SetBoundaries(Box(x_2,x_2+d3/10, y_2, y_2+d4/10,0,33.5));

	  file<< x<< "\t"<< x+d1/10 << "\t"<< y << "\t"<< y+d2/10 << endl;
	  file<< x_2<< "\t"<< x_2+d3/10 << "\t"<< y_2 << "\t"<< y_2+d4/10 << endl;

	  }*/
	Ptr<Building> building ;
	//      double d = 0.5;
	std::ifstream inFile;
	inFile.open(buildingTopology.c_str());
	if (inFile.fail())
		cout<< "NO file" <<endl;
	char inputString[100];
	while(!inFile.eof()){
		inFile.getline(inputString, 100);
		string a(inputString);
		string b = a.substr(a.find('\t')+1);
		string c = b.substr(b.find('\t')+1);
		string d = c.substr(c.find('\t')+1);
		//std::cout <<   << std::endl;
		uint16_t x = atoi(a.c_str());
		uint16_t x_d = atoi(b.c_str());
		uint16_t y= atoi(c.c_str());
		uint16_t y_d = atoi(d.c_str());
		building = Create<Building>();
		if (x!=0)
			building ->SetBoundaries(Box(x,x_d, y, y_d, 0, 35));
	}

	inFile.close();

	Simulator::Schedule(Seconds(0.5),&ChangeSpeed, ueNodes.Get(0),Vector(0,Velocity,0));
	//Simulator::Schedule(Seconds(10.5),&ChangeSpeed, ueNodes.Get(0),Vector(0,-Velocity,0));
	Simulator::Schedule(Seconds(20.5),&ChangeSpeed, ueNodes.Get(0),Vector(0,-Velocity,0));

	BuildingsHelper::Install (mmWaveEnbNodes_73G);
	BuildingsHelper::Install(mmWaveEnbNodes_28G);
	BuildingsHelper::Install(lteEnbNodes);
	BuildingsHelper::Install (ueNodes);
	BuildingsHelper::MakeMobilityModelConsistent();
	// Install mmWave, lte, mc Devices to the nodes
	NetDeviceContainer lteEnbDevs = mmwaveHelper->InstallLteEnbDevice (lteEnbNodes);
	NetDeviceContainer mmWaveEnbDevs_73GHZ = mmwaveHelper->InstallEnbDevice_73GHZ(mmWaveEnbNodes_73G);
	NetDeviceContainer mmWaveEnbDevs_28GHZ = mmwaveHelper->InstallEnbDevice_28GHZ(mmWaveEnbNodes_28G);

	NetDeviceContainer mcUeDevs;
	mcUeDevs = mmwaveHelper->InstallMcUeDevice (ueNodes);

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
	mmwaveHelper->AddX2Interface(lteEnbNodes, mmWaveEnbNodes_73G, mmWaveEnbNodes_28G);
	mmwaveHelper->AttachToClosestEnb (mcUeDevs, mmWaveEnbDevs_28GHZ, mmWaveEnbDevs_73GHZ, lteEnbDevs);


	uint16_t dlPort = 1234;
	uint16_t ulPort = 2000;
	ApplicationContainer clientApps;
	ApplicationContainer serverApps;


	if (tcp){

		for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
		{
			if (dl)
			{
				//NS_LOG_LOGIC ("installing TCP DL app for UE " << u);
				PacketSinkHelper dlPacketSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
				serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
				Ptr<MyApp> app = CreateObject<MyApp> ();
				Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (remoteHostContainer.Get (u), TcpSocketFactory::GetTypeId ());
				Address sinkAddress (InetSocketAddress (ueIpIface.GetAddress (u), dlPort));

				app->Setup (ns3TcpSocket, sinkAddress, 1400, 50000000, DataRate ("200Mbps"));

				remoteHostContainer.Get (u)->AddApplication (app);

				std::ostringstream fileName;
				//fileName<<"UE-"<<u+1<<"-TCP-DATA.txt";
				fileName<<"tcp_data_ue"<<u+1<<".txt";

				AsciiTraceHelper asciiTraceHelper;
				Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (fileName.str ().c_str ());
				serverApps.Get(u)->TraceConnectWithoutContext("Rx",MakeBoundCallback (&Rx, stream,u));

				std::ostringstream fileName_2;
				//fileName_2<<"UE-" << u+1 <<"-TCP-Throughput.txt";
				fileName_2<<"tcp_throughput_ue" << u+1 <<".txt";
				AsciiTraceHelper asciiTraceHelper_2;
				Ptr<OutputStreamWrapper> stream_2 = asciiTraceHelper_2.CreateFileStream(fileName_2.str().c_str());
				Simulator::Schedule (Seconds (0.1), &CalculateThroughput,stream_2,serverApps.Get(u)->GetObject<PacketSink>(),u);
				if(u==0){
					AsciiTraceHelper asciiTraceHelper;
					Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream ("tcp_cwnd_ue1.txt");
					ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream1));
					Ptr<OutputStreamWrapper> stream4 = asciiTraceHelper.CreateFileStream ("tcp_rtt_ue1.txt");
					ns3TcpSocket->TraceConnectWithoutContext ("RTT", MakeBoundCallback (&RttChange, stream4));

				}else
					Simulator::Schedule (Seconds (u*0.1+0.3), &Traces, u);

				app->SetStartTime (Seconds (0.1+u*0.2));
				app->SetStopTime (Seconds (simTime+0.1));
			}
			if (ul)
			{
				//NS_LOG_LOGIC ("installing TCP UL app for UE " << u);
				OnOffHelper ulClientHelper ("ns3::TcpSocketFactory",
						InetSocketAddress (remoteHostAddr[u], ulPort));
				ulClientHelper.SetConstantRate(DataRate ("100Mb/s"), 1500);
				clientApps.Add (ulClientHelper.Install (ueNodes.Get(u)));
				PacketSinkHelper ulPacketSinkHelper ("ns3::TcpSocketFactory",
						InetSocketAddress (Ipv4Address::GetAny (), ulPort));
				serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
			}
		}

	}
	else{
		for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
		{
			if(dl)
			{
				PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
				serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
				Ptr<MyApp> app = CreateObject<MyApp> ();
				Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (remoteHostContainer.Get (u), UdpSocketFactory::GetTypeId ());
				Address sinkAddress (InetSocketAddress (ueIpIface.GetAddress (u), dlPort));
				//app->Setup (ns3UdpSocket, sinkAddress, 1400, packetNumber, DataRate ("300Mbps"));
				//app->Setup (ns3UdpSocket, sinkAddress, 1400, 50000000, DataRate ("1000Mbps"));

				remoteHostContainer.Get (u)->AddApplication (app);

				std::ostringstream fileName;
				//fileName<<"UE-"<<u+1<<"-UDP-DATA_MmWaveDc.txt";
				fileName<<"udp_data_ue"<<u+1<<".txt";
				AsciiTraceHelper asciiTraceHelper;
				Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (fileName.str ().c_str ());
				serverApps.Get(u)->TraceConnectWithoutContext("Rx",MakeBoundCallback (&Rx, stream,u));

				std::ostringstream fileName_2;
				//fileName_2<<"UE-" << u+1 <<"-UDP-Throughput_MmWaveDc.txt";
				fileName_2<<"udp_throughput_ue" << u+1 <<".txt";
				AsciiTraceHelper asciiTraceHelper_2;
				Ptr<OutputStreamWrapper> stream_2 = asciiTraceHelper_2.CreateFileStream(fileName_2.str().c_str());
				Simulator::Schedule (Seconds (0.1), &CalculateThroughput,stream_2,serverApps.Get(u)->GetObject<PacketSink>(),u);

				std::ostringstream fileName_3;
				fileName_3 <<"app_delay"<<u+1<<".txt";
				Ptr<OutputStreamWrapper> stream_delay = asciiTraceHelper.CreateFileStream (fileName_3.str().c_str());
				serverApps.Get(u)->TraceConnectWithoutContext ("LinkDelay", MakeBoundCallback (&LinkDelay, stream_delay,u));

				std::ostringstream fileLoss;
				fileLoss<<"packet_loss_"<<u+1<<".txt";
				Ptr<OutputStreamWrapper> stream_loss = asciiTraceHelper.CreateFileStream (fileLoss.str().c_str());
				serverApps.Get(u)->TraceConnectWithoutContext ("Loss", MakeBoundCallback (&Loss, stream_loss));
	
				Ptr<PacketSink> sink0 = serverApps.Get(0)->GetObject<PacketSink>();
				sink0->SetAttribute("MessgeTransmission", BooleanValue(true));
				sink0->SetAttribute("PacketNumber",UintegerValue(packetNumber));
				app->Setup (ns3UdpSocket, sinkAddress, 32, packetNumber, DataRate ("100Mbps"));

				std::ostringstream fileName_4;
				fileName_4 <<"file_delay_"<<u+1<<".txt";
				Ptr<OutputStreamWrapper> stream_4 = asciiTraceHelper_2.CreateFileStream(fileName_4.str().c_str());
				app->SetStream(stream_4);
				
				app->SetStartTime (Seconds (0.3+u*0.1));
				app->SetStopTime (Seconds (simTime+0.1));
				app->SetSinkApp(serverApps.Get(u)->GetObject<PacketSink>());
				app->appNumber= u ;
			}
			if(ul)
			{
				++ulPort;
				PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
				serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
				UdpClientHelper ulClient (remoteHostAddr[u], ulPort);
				ulClient.SetAttribute ("Interval", TimeValue (MicroSeconds(interPacketInterval)));
				ulClient.SetAttribute ("MaxPackets", UintegerValue(1000000));
				clientApps.Add (ulClient.Install (ueNodes.Get(u)));
			}
		}
	}

	mmwaveHelper -> EnableTraces();

	//Ptr<MmWaveDrbActivator>
	//mmwaveHelper->MmWaveDrbActivator(mcUeDevs,bearer);

	// Start applications
	Config::Set ("/NodeList/*/DeviceList/*/TxQueue/MaxPackets", UintegerValue (1000*1000*1000));
	serverApps.Start (Seconds (0.1));
	std::ostringstream fileName_p;
	fileName_p<<"ue1_position.txt";
	AsciiTraceHelper asciiTraceHelper;
	Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (fileName_p.str ().c_str ());

	Simulator::Schedule(MilliSeconds(30),&TracePosition, ueNodes.Get(0), stream);

	Simulator::Stop(Seconds(simTime));
	Simulator::Run();
	Simulator::Destroy();
	return 0;
}