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
  MyAppTag tag (Simulator::Now ());

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



static void Rx (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> packet, const Address &from)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << packet->GetSize()<< std::endl;
}

static void Sstresh (Ptr<OutputStreamWrapper> stream, uint32_t oldSstresh, uint32_t newSstresh)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldSstresh << "\t" << newSstresh << std::endl;
}



void 
PrintGnuplottableBuildingListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  uint32_t index = 0;
  for (BuildingList::Iterator it = BuildingList::Begin (); it != BuildingList::End (); ++it)
    {
      ++index;
      Box box = (*it)->GetBoundaries ();
      outFile << "set object " << index
              << " rect from " << box.xMin  << "," << box.yMin
              << " to "   << box.xMax  << "," << box.yMax
              << " front fs empty "
              << std::endl;
    }
}

void 
PrintGnuplottableUeListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<LteUeNetDevice> uedev = node->GetDevice (j)->GetObject <LteUeNetDevice> ();
          Ptr<MmWaveUeNetDevice> mmuedev = node->GetDevice (j)->GetObject <MmWaveUeNetDevice> ();
          Ptr<McUeNetDevice> mcuedev = node->GetDevice (j)->GetObject <McUeNetDevice> ();
          if (uedev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << uedev->GetImsi ()
                      << "\" at "<< pos.x << "," << pos.y << " left font \"Helvetica,8\" textcolor rgb \"black\" front point pt 1 ps 0.3 lc rgb \"black\" offset 0,0"
                      << std::endl;
            }
          else if (mmuedev)
           {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << mmuedev->GetImsi ()
                      << "\" at "<< pos.x << "," << pos.y << " left font \"Helvetica,8\" textcolor rgb \"black\" front point pt 1 ps 0.3 lc rgb \"black\" offset 0,0"
                      << std::endl;
            }
          else if (mcuedev)
           {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << mcuedev->GetImsi ()
                      << "\" at "<< pos.x << "," << pos.y << " left font \"Helvetica,8\" textcolor rgb \"black\" front point pt 1 ps 0.3 lc rgb \"black\" offset 0,0"
                      << std::endl;
            } 
        }
    }
}

void 
PrintGnuplottableEnbListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<LteEnbNetDevice> enbdev = node->GetDevice (j)->GetObject <LteEnbNetDevice> ();
          Ptr<MmWaveEnbNetDevice> mmdev = node->GetDevice (j)->GetObject <MmWaveEnbNetDevice> ();
          if (enbdev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << enbdev->GetCellId ()
                      << "\" at "<< pos.x << "," << pos.y
                      << " left font \"Helvetica,8\" textcolor rgb \"blue\" front  point pt 4 ps 0.3 lc rgb \"blue\" offset 0,0"
                      << std::endl;
            }
          else if (mmdev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << mmdev->GetCellId ()
                      << "\" at "<< pos.x << "," << pos.y
                      << " left font \"Helvetica,8\" textcolor rgb \"red\" front  point pt 4 ps 0.3 lc rgb \"red\" offset 0,0"
                      << std::endl;
            } 
        }
    }
}

void
ChangePosition(Ptr<Node> node, Vector vector)
{
  Ptr<MobilityModel> model = node->GetObject<MobilityModel> ();
  model->SetPosition(vector);
  NS_LOG_UNCOND("************************--------------------Change Position-------------------------------*****************");
}

void
ChangeSpeed(Ptr<Node> n, Vector speed)
{
  n->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (speed);
  NS_LOG_UNCOND("************************--------------------Change Speed-------------------------------*****************");
}

void
PrintPosition(Ptr<Node> node)
{
  Ptr<MobilityModel> model = node->GetObject<MobilityModel> ();
  NS_LOG_UNCOND("Position +****************************** " << model->GetPosition() << " at time " << Simulator::Now().GetSeconds());
}

void 
PrintLostUdpPackets(Ptr<UdpServer> app, std::string fileName)
{
  std::ofstream logFile(fileName.c_str(), std::ofstream::app);
  logFile << Simulator::Now().GetSeconds() << " " << app->GetLost() << std::endl;
  logFile.close();
  Simulator::Schedule(MilliSeconds(20), &PrintLostUdpPackets, app, fileName);
}


bool
AreOverlapping(Box a, Box b)
{
  return !((a.xMin > b.xMax) || (b.xMin > a.xMax) || (a.yMin > b.yMax) || (b.yMin > a.yMax) );
}


bool
OverlapWithAnyPrevious(Box box, std::list<Box> m_previousBlocks)
{
  for (std::list<Box>::iterator it = m_previousBlocks.begin(); it != m_previousBlocks.end(); ++it)
  {
    if (AreOverlapping(*it,box))
    {
      return true;
    }
  }
  return false;
}


std::pair<Box, std::list<Box>>
GenerateBuildingBounds(double xArea, double yArea, double maxBuildSize, std::list<Box> m_previousBlocks )
{

  Ptr<UniformRandomVariable> xMinBuilding = CreateObject<UniformRandomVariable>();
  xMinBuilding->SetAttribute("Min",DoubleValue(0));
  xMinBuilding->SetAttribute("Max",DoubleValue(xArea));

  NS_LOG_UNCOND("min " << 0 << " max " << xArea);

  Ptr<UniformRandomVariable> yMinBuilding = CreateObject<UniformRandomVariable>();
  yMinBuilding->SetAttribute("Min",DoubleValue(0));
  yMinBuilding->SetAttribute("Max",DoubleValue(yArea));

  NS_LOG_UNCOND("min " << 0 << " max " << yArea);

  Box box;
  uint32_t attempt = 0;
  do
  {
    NS_ASSERT_MSG(attempt < 100, "Too many failed attempts to position non-overlapping buildings. Maybe area too small or too many buildings?");
    box.xMin = xMinBuilding->GetValue();

    Ptr<UniformRandomVariable> xMaxBuilding = CreateObject<UniformRandomVariable>();
    xMaxBuilding->SetAttribute("Min",DoubleValue(box.xMin));
    xMaxBuilding->SetAttribute("Max",DoubleValue(box.xMin + maxBuildSize));
    box.xMax = xMaxBuilding->GetValue();
    
    box.yMin = yMinBuilding->GetValue();

    Ptr<UniformRandomVariable> yMaxBuilding = CreateObject<UniformRandomVariable>();
    yMaxBuilding->SetAttribute("Min",DoubleValue(box.yMin));
    yMaxBuilding->SetAttribute("Max",DoubleValue(box.yMin + maxBuildSize));
    box.yMax = yMaxBuilding->GetValue();

    ++attempt;
  }
  while (OverlapWithAnyPrevious (box, m_previousBlocks));


  NS_LOG_UNCOND("Building in coordinates (" << box.xMin << " , " << box.yMin << ") and ("  << box.xMax << " , " << box.yMax <<
    ") accepted after " << attempt << " attempts");
  m_previousBlocks.push_back(box);
  std::pair<Box, std::list<Box>> pairReturn = std::make_pair(box,m_previousBlocks);
  return pairReturn;

}


static ns3::GlobalValue g_mmw1DistFromMainStreet("mmw1Dist", "Distance from the main street of the first MmWaveEnb",
    ns3::UintegerValue(50), ns3::MakeUintegerChecker<uint32_t>());
static ns3::GlobalValue g_mmw2DistFromMainStreet("mmw2Dist", "Distance from the main street of the second MmWaveEnb",
    ns3::UintegerValue(50), ns3::MakeUintegerChecker<uint32_t>());
static ns3::GlobalValue g_mmw3DistFromMainStreet("mmw3Dist", "Distance from the main street of the third MmWaveEnb",
    ns3::UintegerValue(110), ns3::MakeUintegerChecker<uint32_t>());
static ns3::GlobalValue g_mmWaveDistance("mmWaveDist", "Distance between MmWave eNB 1 and 2",
    ns3::UintegerValue(200), ns3::MakeUintegerChecker<uint32_t>());
static ns3::GlobalValue g_numBuildingsBetweenMmWaveEnb("numBlocks", "Number of buildings between MmWave eNB 1 and 2",
    ns3::UintegerValue(10), ns3::MakeUintegerChecker<uint32_t>());
static ns3::GlobalValue g_interPckInterval("interPckInterval", "Interarrival time of UDP packets (us)",
    ns3::UintegerValue(20), ns3::MakeUintegerChecker<uint32_t>());
static ns3::GlobalValue g_bufferSize("bufferSize", "RLC tx buffer size (MB)",
    ns3::UintegerValue(20), ns3::MakeUintegerChecker<uint32_t>());
static ns3::GlobalValue g_x2Latency("x2Latency", "Latency on X2 interface (us)",
    ns3::DoubleValue(0), ns3::MakeDoubleChecker<double>());
static ns3::GlobalValue g_mmeLatency("mmeLatency", "Latency on MME interface (us)",
    ns3::DoubleValue(10000), ns3::MakeDoubleChecker<double>());
static ns3::GlobalValue g_mobileUeSpeed("mobileSpeed", "The speed of the UE (m/s)",
    ns3::DoubleValue(2), ns3::MakeDoubleChecker<double>());
static ns3::GlobalValue g_fastSwitching("fastSwitching", "If true, use mc setup, else use hard handover",
    ns3::BooleanValue(true), ns3::MakeBooleanChecker());
static ns3::GlobalValue g_rlcAmEnabled("rlcAmEnabled", "If true, use RLC AM, else use RLC UM",
    ns3::BooleanValue(true), ns3::MakeBooleanChecker());
static ns3::GlobalValue g_runNumber ("runNumber", "Run number for rng",
    ns3::UintegerValue(1), ns3::MakeUintegerChecker<uint32_t>());
static ns3::GlobalValue g_maxXAxis("maxXAxis", "The maximum X coordinate for the area in which to deploy the buildings",
    ns3::DoubleValue(150), ns3::MakeDoubleChecker<double>());
static ns3::GlobalValue g_maxYAxis("maxYAxis", "The maximum Y coordinate for the area in which to deploy the buildings",
    ns3::DoubleValue(40), ns3::MakeDoubleChecker<double>());
static ns3::GlobalValue g_outPath("outPath",
    "The path of output log files",
    ns3::StringValue("./"), ns3::MakeStringChecker());
static ns3::GlobalValue g_noiseAndFilter("noiseAndFilter", "If true, use noisy SINR samples, filtered. If false, just use the SINR measure",
    ns3::BooleanValue(false), ns3::MakeBooleanChecker());
static ns3::GlobalValue g_handoverMode("handoverMode",
    "Handover mode",
    ns3::UintegerValue(3), ns3::MakeUintegerChecker<uint8_t>());
static ns3::GlobalValue g_reportTablePeriodicity("reportTablePeriodicity", "Periodicity of RTs",
    ns3::UintegerValue(1600), ns3::MakeUintegerChecker<uint32_t>());
static ns3::GlobalValue g_udp("udp", "If true, use UDP. If false, TCP",
    ns3::BooleanValue(true), ns3::MakeBooleanChecker());
static ns3::GlobalValue g_outageThreshold("outageTh", "Outage threshold",
    ns3::DoubleValue(-1000), ns3::MakeDoubleChecker<double>());
static ns3::GlobalValue g_lteUplink("lteUplink", "If true, always use LTE for uplink signalling",
    ns3::BooleanValue(false), ns3::MakeBooleanChecker());

int
main (int argc, char *argv[])
{
  LogComponentEnable ("LteUeRrc", LOG_LEVEL_INFO);
  LogComponentEnable ("LteEnbRrc", LOG_LEVEL_INFO);
  //LogComponentEnable ("LteRlcTm", LOG_LEVEL_LOGIC);
  //LogComponentEnable("MmWavePointToPointEpcHelper",LOG_LEVEL_ALL);
  //LogComponentEnable("EpcUeNas",LOG_LEVEL_INFO);
  //LogComponentEnable ("MmWaveSpectrumPhy", LOG_LEVEL_INFO);
  //LogComponentEnable ("MmWaveUeMac", LOG_LEVEL_LOGIC);
  //LogComponentEnable ("MmWaveEnbMac", LOG_LEVEL_LOGIC);
  //LogComponentEnable ("LteUeMac", LOG_LEVEL_INFO);
  //LogComponentEnable ("LteEnbMac", LOG_LEVEL_INFO);
  //LogComponentEnable ("MmWaveEnbPhy", LOG_LEVEL_INFO);
  //LogComponentEnable ("MmWaveUePhy", LOG_LEVEL_INFO);
  //LogComponentEnable ("MmWaveEnbMac", LOG_LEVEL_INFO);
  //LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
  //LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
  //LogComponentEnable("PropagationLossModel",LOG_LEVEL_ALL);
  //LogComponentEnable("LteRrcProtocolIdeal", LOG_LEVEL_INFO);
  //LogComponentEnable ("mmWavePhyRxTrace", LOG_LEVEL_ALL);
  //LogComponentEnable ("MmWaveRrMacScheduler", LOG_LEVEL_ALL);
  //LogComponentEnable("McUeNetDevice", LOG_LEVEL_INFO);
  //LogComponentEnable("EpcSgwPgwApplication", LOG_LEVEL_FUNCTION);
  //LogComponentEnable("EpcEnbApplication", LOG_LEVEL_FUNCTION);
  //LogComponentEnable("MmWaveEnbMac", LOG_LEVEL_INFO);
  //LogComponentEnable("LteEnbMac", LOG_LEVEL_INFO);
  //LogComponentEnable("MmWavePointToPointEpcHelper", LOG_LEVEL_INFO);
  //LogComponentEnable("MmWaveHelper", LOG_LEVEL_LOGIC);
  //LogComponentEnable("EpcX2", LOG_LEVEL_ALL);
  //LogComponentEnable("EpcX2Header", LOG_LEVEL_ALL);
  LogComponentEnable("McEnbPdcp", LOG_LEVEL_INFO);
  LogComponentEnable("McUePdcp", LOG_LEVEL_INFO);
  //LogComponentEnable("LteRlcAm", LOG_LEVEL_INFO);
  //LogComponentEnable("LteRlcUmLowLat", LOG_LEVEL_INFO);
  //LogComponentEnable("EpcS1ap", LOG_LEVEL_LOGIC);
  //LogComponentEnable("EpcMmeApplication", LOG_LEVEL_LOGIC);
  //LogComponentEnable("LteRrcProtocolReal", LOG_LEVEL_LOGIC);
  //LogComponentEnable("MmWaveFlexTtiMacScheduler", LOG_LEVEL_INFO);
  //LogComponentEnable("AntennaArrayModel", LOG_LEVEL_ALL);
  //LogComponentEnable("UdpServer", LOG_LEVEL_INFO);
  //LogComponentEnable("UdpClient", LOG_LEVEL_INFO);

  uint16_t numberOfNodes = 1;
  //double simTime = 2.0;
  //double interPacketInterval = 50;  // 500 microseconds
  bool harqEnabled = true;
  bool fixedTti = false;
  unsigned symPerSf = 24;
  double sfPeriod = 100.0;

  std::list<Box>  m_previousBlocks;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("numberOfNodes", "Number of eNodeBs + UE pairs", numberOfNodes);
  //cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  //cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
  cmd.Parse(argc, argv);

  UintegerValue uintegerValue;
  BooleanValue booleanValue;
  StringValue stringValue;
  DoubleValue doubleValue;
  //EnumValue enumValue;
  GlobalValue::GetValueByName("numBlocks", uintegerValue);
  uint32_t numBlocks = uintegerValue.Get();
  // GlobalValue::GetValueByName("mmw1Dist", uintegerValue);
  // uint32_t mmw1Dist = uintegerValue.Get();
  // GlobalValue::GetValueByName("mmw2Dist", uintegerValue);
  // uint32_t mmw2Dist = uintegerValue.Get();
  // GlobalValue::GetValueByName("mmw3Dist", uintegerValue);
  // uint32_t mmw3Dist = uintegerValue.Get();
  GlobalValue::GetValueByName("maxXAxis", doubleValue);
  double maxXAxis = doubleValue.Get();
  GlobalValue::GetValueByName("maxYAxis", doubleValue);
  double maxYAxis = doubleValue.Get();
  // GlobalValue::GetValueByName("mmWaveDist", uintegerValue);
  // uint32_t mmWaveDist = uintegerValue.Get();
  // uint32_t mmWaveZ = 10;
  // uint32_t streetWidth = 15;
  // uint32_t minimumBuildingWidth = 10;
  // //uint32_t buildingZ = 200;

  // Ptr<UniformRandomVariable> userPosition = CreateObject<UniformRandomVariable>();
  // userPosition->SetAttribute("Min",DoubleValue(0));
  // userPosition->SetAttribute("Max",DoubleValue(400));
  // double ueInitialPosition = userPosition->GetValue();
  // NS_LOG_UNCOND("UE X coordinate is: " << ueInitialPosition);
  double ueInitialPosition = 0;
  double ueFinalPosition = 40;

  // Variables for the RT 
  int windowForTransient = 150; // number of samples for the vector to use in the filter
  GlobalValue::GetValueByName("reportTablePeriodicity", uintegerValue);
  int ReportTablePeriodicity = (int)uintegerValue.Get(); // in microseconds
  if(ReportTablePeriodicity == 1600)
  {
    windowForTransient = 150;
  }
  else if(ReportTablePeriodicity == 25600)
  {
    windowForTransient = 50;
  }
  else if(ReportTablePeriodicity == 12800)
  {
    windowForTransient = 100;
  }
  else
  {
    NS_ASSERT_MSG(false, "Unrecognized");
  }

  int vectorTransient = windowForTransient*ReportTablePeriodicity;
  GlobalValue::GetValueByName("fastSwitching", booleanValue);
  bool fastSwitching = booleanValue.Get();
  bool hardHandover = !fastSwitching;

  // params for RT, filter, HO mode
  GlobalValue::GetValueByName("noiseAndFilter", booleanValue);
  bool noiseAndFilter = booleanValue.Get();
  GlobalValue::GetValueByName("handoverMode", uintegerValue);
  uint8_t hoMode = uintegerValue.Get();
  GlobalValue::GetValueByName("udp", booleanValue);
  bool tcp = !(booleanValue.Get());
  GlobalValue::GetValueByName("outageTh", doubleValue);
  double outageTh = doubleValue.Get();

  GlobalValue::GetValueByName("rlcAmEnabled", booleanValue);
  bool rlcAmEnabled = booleanValue.Get();
  GlobalValue::GetValueByName("bufferSize", uintegerValue);
  uint32_t bufferSize = uintegerValue.Get();
  GlobalValue::GetValueByName("interPckInterval", uintegerValue);
  uint32_t interPacketInterval = uintegerValue.Get();
  GlobalValue::GetValueByName("x2Latency", doubleValue);
  double x2Latency = doubleValue.Get();
  GlobalValue::GetValueByName("mmeLatency", doubleValue);
  double mmeLatency = doubleValue.Get();
  GlobalValue::GetValueByName("mobileSpeed", doubleValue);
  double ueSpeed = doubleValue.Get();

  double transientDuration = double(vectorTransient)/1000000; 
  double simTime = transientDuration + ((double)ueFinalPosition - (double)ueInitialPosition)/ueSpeed + 1;

  NS_LOG_UNCOND("fastSwitching " << fastSwitching << " rlcAmEnabled " << rlcAmEnabled << " bufferSize " << bufferSize << " interPacketInterval " << 
      interPacketInterval << " x2Latency " << x2Latency << " mmeLatency " << mmeLatency << " mobileSpeed " << ueSpeed);

  // rng things
  GlobalValue::GetValueByName("runNumber", uintegerValue);
  uint32_t runSet = uintegerValue.Get();
  uint32_t seedSet = 5;
  RngSeedManager::SetSeed (seedSet);
  RngSeedManager::SetRun (runSet); 
  char seedSetStr[21];
  char runSetStr[21];
  sprintf(seedSetStr, "%d", seedSet);
  sprintf(runSetStr, "%d", runSet);

  GlobalValue::GetValueByName("outPath", stringValue);
  std::string path = stringValue.Get();
  std::string mmWaveOutName = "MmWaveSwitchStats";
  std::string lteOutName = "LteSwitchStats";
  std::string dlRlcOutName = "DlRlcStats";
  std::string dlPdcpOutName = "DlPdcpStats";
  std::string ulRlcOutName = "UlRlcStats";
  std::string ulPdcpOutName = "UlPdcpStats";
  std::string  ueHandoverStartOutName =  "UeHandoverStartStats";
  std::string enbHandoverStartOutName = "EnbHandoverStartStats";
  std::string  ueHandoverEndOutName =  "UeHandoverEndStats";
  std::string enbHandoverEndOutName = "EnbHandoverEndStats";
  std::string cellIdInTimeOutName = "CellIdStats";
  std::string cellIdInTimeHandoverOutName = "CellIdStatsHandover";
  std::string mmWaveSinrOutputFilename = "MmWaveSinrTime";
  std::string x2statOutputFilename = "X2Stats";
  std::string udpSentFilename = "UdpSent";
  std::string udpReceivedFilename = "UdpReceived";
  std::string extension = ".txt";
  std::string version;
  if(fastSwitching)
  {
    version = "mc";
  }
  else if(hardHandover)
  {
    version = "hh";
  }
  //get current time
  time_t rawtime;
  struct tm * timeinfo;
  char buffer[80];
  time (&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(buffer,80,"%d_%m_%Y_%I_%M_%S",timeinfo);
  std::string time_str(buffer);

  Config::SetDefault ("ns3::MmWaveHelper::RlcAmEnabled", BooleanValue(rlcAmEnabled));
  Config::SetDefault ("ns3::MmWaveHelper::HarqEnabled", BooleanValue(harqEnabled));
  Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::HarqEnabled", BooleanValue(harqEnabled));
  Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::HarqEnabled", BooleanValue(harqEnabled));
  Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::FixedTti", BooleanValue(fixedTti));
  Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::SymPerSlot", UintegerValue(6));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::ResourceBlockNum", UintegerValue(1));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::ChunkPerRB", UintegerValue(72));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::SymbolsPerSubframe", UintegerValue(symPerSf));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::SubframePeriod", DoubleValue(sfPeriod));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::TbDecodeLatency", UintegerValue(200.0));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::NumHarqProcess", UintegerValue(100));
  Config::SetDefault ("ns3::MmWaveBeamforming::LongTermUpdatePeriod", TimeValue (MilliSeconds (100.0)));
  Config::SetDefault ("ns3::LteEnbRrc::SystemInformationPeriodicity", TimeValue (MilliSeconds (5.0)));
  //Config::SetDefault ("ns3::MmWavePropagationLossModel::ChannelStates", StringValue ("n"));
  Config::SetDefault ("ns3::LteRlcAm::ReportBufferStatusTimer", TimeValue(MicroSeconds(100.0)));
  Config::SetDefault ("ns3::LteRlcUmLowLat::ReportBufferStatusTimer", TimeValue(MicroSeconds(100.0)));
  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (320));
  Config::SetDefault ("ns3::LteEnbRrc::FirstSibTime", UintegerValue (2));
  Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::X2LinkDelay", TimeValue (MicroSeconds(x2Latency)));
  Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::X2LinkDataRate", DataRateValue(DataRate ("1000Gb/s")));
  Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::X2LinkMtu",  UintegerValue(10000));
  Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::S1uLinkDelay", TimeValue (MicroSeconds(1000)));
  Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::S1apLinkDelay", TimeValue (MicroSeconds(mmeLatency)));
  Config::SetDefault ("ns3::McStatsCalculator::MmWaveOutputFilename", StringValue                 (path + version + mmWaveOutName + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::McStatsCalculator::LteOutputFilename", StringValue                    (path + version + lteOutName    + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::McStatsCalculator::CellIdInTimeOutputFilename", StringValue           (path + version + cellIdInTimeOutName    + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::MmWaveBearerStatsCalculator::DlRlcOutputFilename", StringValue        (path + version + dlRlcOutName   + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::MmWaveBearerStatsCalculator::UlRlcOutputFilename", StringValue        (path + version + ulRlcOutName   + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::MmWaveBearerStatsCalculator::DlPdcpOutputFilename", StringValue       (path + version + dlPdcpOutName + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::MmWaveBearerStatsCalculator::UlPdcpOutputFilename", StringValue       (path + version + ulPdcpOutName + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::MmWaveBearerStatsConnector::UeHandoverStartOutputFilename", StringValue    (path + version +  ueHandoverStartOutName + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::MmWaveBearerStatsConnector::EnbHandoverStartOutputFilename", StringValue   (path + version + enbHandoverStartOutName + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::MmWaveBearerStatsConnector::UeHandoverEndOutputFilename", StringValue    (path + version +  ueHandoverEndOutName + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::MmWaveBearerStatsConnector::EnbHandoverEndOutputFilename", StringValue   (path + version + enbHandoverEndOutName + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::MmWaveBearerStatsConnector::CellIdStatsHandoverOutputFilename", StringValue(path + version + cellIdInTimeHandoverOutName + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::MmWaveBearerStatsConnector::MmWaveSinrOutputFilename", StringValue(path + version + mmWaveSinrOutputFilename + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::CoreNetworkStatsCalculator::X2FileName", StringValue                  (path + version + x2statOutputFilename    + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  std::string lostFilename = path + version + "LostUdpPackets" +  "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension;
  //Config::SetDefault ("ns3::MmWaveHelper::ChannelModel", StringValue("ns3::MmWaveChannelMatrix"))
  Config::SetDefault ("ns3::UdpServer::ReceivedPacketsFilename", StringValue(path + version + "ReceivedUdp" +  "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::UdpClient::SentPacketsFilename", StringValue(path + version + "SentUdp" +  "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::UdpServer::ReceivedSnFilename", StringValue(path + version + "ReceivedSn" +  "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::LteRlcAm::BufferSizeFilename", StringValue(path + version + "RlcAmBufferSize" +  "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));

  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (bufferSize * 1024 * 1024));
  Config::SetDefault ("ns3::LteRlcUmLowLat::MaxTxBufferSize", UintegerValue (bufferSize * 1024 * 1024));
  Config::SetDefault ("ns3::LteRlcAm::StatusProhibitTimer", TimeValue(MilliSeconds(10.0)));
  Config::SetDefault ("ns3::LteRlcAm::MaxTxBufferSize", UintegerValue (bufferSize * 1024 * 1024));

   // handover and RT related params
  switch(hoMode)
  {
    case 1:
        Config::SetDefault ("ns3::LteEnbRrc::SecondaryCellHandoverMode", EnumValue(LteEnbRrc::THRESHOLD));
        break;
    case 2:
        Config::SetDefault ("ns3::LteEnbRrc::SecondaryCellHandoverMode", EnumValue(LteEnbRrc::FIXED_TTT));
        break;
    case 3:
        Config::SetDefault ("ns3::LteEnbRrc::SecondaryCellHandoverMode", EnumValue(LteEnbRrc::DYNAMIC_TTT));
        break;
  }
  
  Config::SetDefault ("ns3::LteEnbRrc::FixedTttValue", UintegerValue (150));
  Config::SetDefault ("ns3::LteEnbRrc::CrtPeriod", IntegerValue (ReportTablePeriodicity));
  Config::SetDefault ("ns3::LteEnbRrc::OutageThreshold", DoubleValue (outageTh));
  Config::SetDefault ("ns3::MmWaveEnbPhy::UpdateSinrEstimatePeriod", IntegerValue (ReportTablePeriodicity));
  Config::SetDefault ("ns3::MmWaveEnbPhy::Transient", IntegerValue (vectorTransient));
  Config::SetDefault ("ns3::MmWaveEnbPhy::NoiseAndFilter", BooleanValue(noiseAndFilter));

  // TCP settings
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (131072*40));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (131072*40));
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));

  GlobalValue::GetValueByName("lteUplink", booleanValue);
  bool lteUplink = booleanValue.Get();

  Config::SetDefault("ns3::McUePdcp::LteUplink", BooleanValue(lteUplink));
  std::cout << "Lte uplink " << lteUplink << "\n";


  Ptr<MmWaveHelper> mmwaveHelper = CreateObject<MmWaveHelper> ();
  //mmwaveHelper->SetSchedulerType ("ns3::MmWaveFlexTtiMaxWeightMacScheduler");
  Ptr<MmWavePointToPointEpcHelper> epcHelper = CreateObject<MmWavePointToPointEpcHelper> ();
  mmwaveHelper->SetEpcHelper (epcHelper);
  mmwaveHelper->SetHarqEnabled (harqEnabled);
  //mmwaveHelper->SetAttribute ("PathlossModel", StringValue ("ns3::BuildingsObstaclePropagationLossModel"));

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

   // Get SGW/PGW and create a single RemoteHost 
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet by connecting remoteHost to pgw. Setup routing too
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (2500));
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

  // create LTE, mmWave eNB nodes and UE node
  NodeContainer ueNodes;
  NodeContainer mmWaveEnbNodes;
  NodeContainer lteEnbNodes;
  NodeContainer allEnbNodes;
  mmWaveEnbNodes.Create(1);
  lteEnbNodes.Create(1);
  ueNodes.Create(1);
  allEnbNodes.Add(lteEnbNodes);
  allEnbNodes.Add(mmWaveEnbNodes);

  // Positions
  Vector mmw1Position = Vector(-1, 20, 3);

  std::vector<Ptr<Building> > buildingVector;

  double maxBuildingSize = 5;

  for(uint32_t buildingIndex = 0; buildingIndex < numBlocks; buildingIndex++)
  {
    Ptr < Building > building;
      building = Create<Building> ();
      /* returns a vecotr where:
      * position [0]: coordinates for x min
      * position [1]: coordinates for x max
      * position [2]: coordinates for y min
      * position [3]: coordinates for y max
      */
      std::pair<Box, std::list<Box>> pairBuildings = GenerateBuildingBounds(maxXAxis, maxYAxis, maxBuildingSize, m_previousBlocks);
      m_previousBlocks = std::get<1>(pairBuildings);
      Box box = std::get<0>(pairBuildings);
      Ptr<UniformRandomVariable> randomBuildingZ = CreateObject<UniformRandomVariable>();
      randomBuildingZ->SetAttribute("Min",DoubleValue(1.6));
      randomBuildingZ->SetAttribute("Max",DoubleValue(40));
      double buildingHeight = randomBuildingZ->GetValue();

      building->SetBoundaries (Box(box.xMin, box.xMax,
                                    box.yMin,  box.yMax,
                                    0.0, buildingHeight));
      buildingVector.push_back(building);
  }


  // Install Mobility Model
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  //enbPositionAlloc->Add (Vector ((double)mmWaveDist/2 + streetWidth, mmw1Dist + 2*streetWidth, mmWaveZ));
  enbPositionAlloc->Add (mmw1Position); // LTE BS, out of area where buildings are deployed
  enbPositionAlloc->Add (mmw1Position);
  MobilityHelper enbmobility;
  enbmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbmobility.SetPositionAllocator(enbPositionAlloc);
  enbmobility.Install (allEnbNodes);
  BuildingsHelper::Install (allEnbNodes);

  MobilityHelper uemobility;
  Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
  //uePositionAlloc->Add (Vector (ueInitialPosition, -5, 0));
  uePositionAlloc->Add (Vector (ueInitialPosition, 151, 0));
  uemobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  uemobility.SetPositionAllocator(uePositionAlloc);
  uemobility.Install (ueNodes);
  BuildingsHelper::Install (ueNodes);

  //ueNodes.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (ueInitialPosition, -5, 0));
  ueNodes.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (ueInitialPosition, 151, 0));
  ueNodes.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0, 0, 0));

  // Install mmWave, lte, mc Devices to the nodes
  NetDeviceContainer lteEnbDevs = mmwaveHelper->InstallLteEnbDevice (lteEnbNodes);
  NetDeviceContainer mmWaveEnbDevs = mmwaveHelper->InstallEnbDevice (mmWaveEnbNodes);
  NetDeviceContainer mcUeDevs;
  if(fastSwitching)
  {
    mcUeDevs = mmwaveHelper->InstallMcUeDevice (ueNodes);
  } 
  else if(hardHandover)
  {
    mcUeDevs = mmwaveHelper->InstallInterRatHoCapableUeDevice (ueNodes);
  }
  else
  {
    NS_FATAL_ERROR("Invalid option");
  }

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

  // Manual attachment
  if(fastSwitching)
  {
    mmwaveHelper->AttachToClosestEnb (mcUeDevs, mmWaveEnbDevs, lteEnbDevs);  
  }
  else if(hardHandover)
  {
    mmwaveHelper->AttachIrToClosestEnb (mcUeDevs, mmWaveEnbDevs, lteEnbDevs);
  }
  

  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  bool dl = 1;
  bool ul = 0;

  if(tcp)
  {
    // Install and start applications on UEs and remote host
    Address sinkAddress (InetSocketAddress (ueIpIface.GetAddress (0), dlPort));
    PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
    serverApps.Add(packetSinkHelper.Install (ueNodes.Get (0)));

    Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (remoteHost, TcpSocketFactory::GetTypeId ());
    Ptr<MyApp> app = CreateObject<MyApp> ();
    app->Setup (ns3TcpSocket, sinkAddress, 1000, 50000000, DataRate ("1000Mb/s"));
    clientApps.Add(app);
	remoteHost->AddApplication (app);
    
    AsciiTraceHelper asciiTraceHelper;
     
    Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream (path + version + "mmWave-tcp-window-newreno" + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension);
    ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream1));
    Ptr<OutputStreamWrapper> stream4 = asciiTraceHelper.CreateFileStream (path + version + "mmWave-tcp-rtt-newreno" + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension);
    ns3TcpSocket->TraceConnectWithoutContext ("RTT", MakeBoundCallback (&RttChange, stream4));
	  Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream (path + version + "mmWave-tcp-data-newreno" + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension);
    serverApps.Get(0)->TraceConnectWithoutContext("Rx",MakeBoundCallback (&Rx, stream2));

    // for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    // {
    //   if (dl)
    //     {
    //       //NS_LOG_LOGIC ("installing TCP DL app for UE " << u);
    //       OnOffHelper dlClientHelper ("ns3::TcpSocketFactory",
    //                                      InetSocketAddress (ueIpIface.GetAddress (u), dlPort));
    //       dlClientHelper.SetConstantRate(DataRate ("1000Mb/s"), 1500);
    //       clientApps.Add (dlClientHelper.Install (remoteHost));
    //       PacketSinkHelper dlPacketSinkHelper ("ns3::TcpSocketFactory", 
    //                                            InetSocketAddress (Ipv4Address::GetAny (), dlPort));
    //       serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
    //     }
    //   if (ul)
    //     {
    //       //NS_LOG_LOGIC ("installing TCP UL app for UE " << u);
    //       OnOffHelper ulClientHelper ("ns3::TcpSocketFactory",
    //                                      InetSocketAddress (remoteHostAddr, ulPort));
    //       ulClientHelper.SetConstantRate(DataRate ("1000Mb/s"), 1500);
    //       clientApps.Add (ulClientHelper.Install (ueNodes.Get(u)));
    //       PacketSinkHelper ulPacketSinkHelper ("ns3::TcpSocketFactory", 
    //                                            InetSocketAddress (Ipv4Address::GetAny (), ulPort));
    //       serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
    //     }
    // }
    Ptr<OutputStreamWrapper> stream3 = asciiTraceHelper.CreateFileStream ("mmWave-tcp-sstresh-newreno.txt");
    ns3TcpSocket->TraceConnectWithoutContext("SlowStartThreshold",MakeBoundCallback (&Sstresh, stream3));
  }
  else // use UDP
  {
    for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
        if(dl)
        {
          UdpServerHelper dlPacketSinkHelper (dlPort);
          dlPacketSinkHelper.SetAttribute ("PacketWindowSize", UintegerValue(256));
          serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));

          Simulator::Schedule(MilliSeconds(20), &PrintLostUdpPackets, DynamicCast<UdpServer>(serverApps.Get(serverApps.GetN()-1)), lostFilename);

          UdpClientHelper dlClient (ueIpIface.GetAddress (u), dlPort);
          dlClient.SetAttribute ("Interval", TimeValue (MicroSeconds(interPacketInterval)));
          dlClient.SetAttribute ("MaxPackets", UintegerValue(0xFFFFFFFF));
          clientApps.Add (dlClient.Install (remoteHost));

        }
        if(ul)
        {
          ++ulPort;
          PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
          ulPacketSinkHelper.SetAttribute ("PacketWindowSize", UintegerValue(256));
          serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
          UdpClientHelper ulClient (remoteHostAddr, ulPort);
          ulClient.SetAttribute ("Interval", TimeValue (MicroSeconds(interPacketInterval)));
          ulClient.SetAttribute ("MaxPackets", UintegerValue(0xFFFFFFFF));
          clientApps.Add (ulClient.Install (ueNodes.Get(u)));
        }
    }
  }

  // Start applications
  NS_LOG_UNCOND("transientDuration " << transientDuration << " simTime " << simTime);
  serverApps.Start (Seconds(transientDuration));
  clientApps.Start (Seconds(transientDuration));
  clientApps.Stop (Seconds(simTime - 1));


  Simulator::Schedule(Seconds(transientDuration), &ChangeSpeed, ueNodes.Get(0), Vector(0, ueSpeed, 0)); // start UE movement after Seconds(0.5)
  Simulator::Schedule(Seconds(simTime - 1), &ChangeSpeed, ueNodes.Get(0), Vector(0, 0, 0)); // start UE movement after Seconds(0.5)

  double numPrints = 0;
  for(int i = 0; i < numPrints; i++)
  {
   Simulator::Schedule(Seconds(i*simTime/numPrints), &PrintPosition, ueNodes.Get(0)); 
  }

  // Uncomment to enable PCAP tracing
  // p2ph.EnablePcapAll("mmwave-epc-simple");
  BuildingsHelper::MakeMobilityModelConsistent ();

  mmwaveHelper->EnableTraces ();

  bool print = false;
  if(print)
  {
    PrintGnuplottableBuildingListToFile("buildings.txt");
    PrintGnuplottableEnbListToFile("enbs.txt");
    PrintGnuplottableUeListToFile("ues.txt");
  }
  else
  {
    Simulator::Stop(Seconds(simTime)); //--------------------------------------------> CHANGE
    //Simulator::Stop(Seconds(5)); // use this simulation time just to check the code
    Simulator::Run();    
  }
  
  Simulator::Destroy();
  return 0;
}

